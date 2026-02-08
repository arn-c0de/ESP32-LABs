/*
 * HTTP Helper Functions
 */

extern SemaphoreHandle_t actuatorMutex;

static int parseLineFromSensorId(const String &sensorId) {
  if (!sensorId.startsWith("SENSOR-L")) return 0;
  int start = 8;
  int dash = sensorId.indexOf('-', start);
  if (dash < 0) return 0;
  String lineStr = sensorId.substring(start, dash);
  return lineStr.toInt();
}

static bool isLineAllowedForRole(int line, const String &role) {
  if (line <= 0 || line > NUM_LINES) return false;
  if (role == "admin" || role == "operator") return true;
  if (role == "maintenance") return line <= 2;
  if (role == "viewer" || role == "guest") return line == 1;
  return false;
}

bool rejectIfLowHeap(AsyncWebServerRequest *request) {
  if (ESP.getFreeHeap() < MIN_FREE_HEAP) {
    rateLimitedLog("[HTTP] Rejected - low memory");
    request->send(503, "text/plain", "Server busy - low memory");
    return true;
  }
  return false;
}

bool rejectIfBodyTooLarge(AsyncWebServerRequest *request, size_t total) {
  if (total > MAX_BODY_BYTES) {
    request->send(413, "application/json", "{\"error\":\"Request body too large\"}");
    return true;
  }
  return false;
}

void sendRateLimited(AsyncWebServerRequest *request, const char* contentType, const String &body) {
  AsyncWebServerResponse *response = request->beginResponse(429, contentType, body);
  response->addHeader("Retry-After", "1");
  request->send(response);
}

void addCORSHeaders(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(200);
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  request->send(response);
}

// Extract ID from URL like /api/sensors/SENSOR-L1-01/readings
String extractIdFromUrl(String url, String prefix, String suffix) {
  int prefixPos = url.indexOf(prefix);
  if (prefixPos < 0) return "";

  int startPos = prefixPos + prefix.length();
  int endPos = url.indexOf(suffix, startPos);
  if (endPos < 0) endPos = url.length();

  return url.substring(startPos, endPos);
}

// Custom handler for sensor readings (GET - works fine as custom handler)
class SensorReadingsHandler : public AsyncWebHandler {
public:
  SensorReadingsHandler() {}
  virtual ~SensorReadingsHandler() {}

  bool isRequestHandlerTrivial() override { return false; }

  bool canHandle(AsyncWebServerRequest *request) override {
    if (request->method() != HTTP_GET) return false;
    String url = request->url();
    return url.startsWith("/api/sensors/") && url.endsWith("/readings");
  }

  void handleRequest(AsyncWebServerRequest *request) override {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true, clientIP);
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }

    totalRequests++;

    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }

    String sensorId = extractIdFromUrl(request->url(), "/api/sensors/", "/readings");
    int limit = 60;
    if (request->hasParam("limit")) {
      limit = request->getParam("limit")->value().toInt();
      if (limit < 1) limit = 1;
      if (limit > 1000) limit = 1000;
    }
    if (!VULN_PHYSICS_ANALYSIS && limit > 10) {
      limit = 10;
    }
    if (!VULN_IDOR_SENSORS) {
      String role = getRequestRole(request);
      int line = parseLineFromSensorId(sensorId);
      if (line > 0 && !isLineAllowedForRole(line, role)) {
        request->send(403, "application/json", "{\"error\":\"Insufficient permissions\"}");
        return;
      }
    }

    Serial.printf("[API] GET /api/sensors/%s/readings (limit=%d) from %s\n",
                  sensorId.c_str(), limit, clientIP.c_str());

    String json = getSensorReadingJSON(sensorId.c_str(), limit);
    if (json == "null") {
      request->send(404, "application/json", "{\"error\":\"Sensor not found\"}");
    } else {
      request->send(200, "application/json", json);
    }
  }
};

// ===== POST handlers using server.on() body callback pattern =====
// (Custom AsyncWebHandler subclasses have issues with POST in ESPAsyncWebServer)

void handleAlarmAckBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  String clientIP = request->client()->remoteIP().toString();
  if (rejectIfLowHeap(request)) {
    return;
  }
  if (rejectIfBodyTooLarge(request, total)) {
    return;
  }
  if (isIpBlocked(clientIP)) {
    request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
    return;
  }
  if (!tryReserveConnection(clientIP)) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  ConnectionGuard guard(true, clientIP);
  if (!checkRateLimit(clientIP)) {
    sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
    return;
  }

  totalRequests++;

  if (!isAuthenticated(request)) {
    Serial.printf("[API] Alarm ack UNAUTHORIZED from %s (hasAuth=%d)\n",
                  clientIP.c_str(), request->hasHeader("Authorization") ? 1 : 0);
    request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }

  String role = getRequestRole(request);
  if (role != "admin" && role != "operator" && role != "maintenance") {
    request->send(403, "application/json", "{\"error\":\"Insufficient permissions\"}");
    return;
  }

  // Parse JSON body
  if (ESP.getFreeHeap() < 50000) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, (char*)data, len);
  if (error) {
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String sensorId = doc["sensor_id"].as<String>();
  unsigned long timestamp = doc["timestamp"] | (unsigned long)0;

  Serial.printf("[API] POST /api/alarms/ack sensor=%s ts=%lu from %s (%s)\n",
                sensorId.c_str(), timestamp, clientIP.c_str(), role.c_str());

  bool found = false;
  int total_alarms = (alarmCount < MAX_ALARMS) ? alarmCount : MAX_ALARMS;

  for (int i = 0; i < total_alarms; i++) {
    AlarmEntry &a = alarms[i];
    if (strcmp(a.sensorId, sensorId.c_str()) == 0 && !a.acknowledged) {
      if (timestamp == 0 || a.timestamp == timestamp) {
        a.acknowledged = true;
        found = true;
        Serial.printf("[API] Acknowledged alarm: %s at %lu\n", a.sensorId, a.timestamp);
        break;
      }
    }
  }

  if (!found) {
    request->send(404, "application/json", "{\"error\":\"Alarm not found or already acknowledged\"}");
    return;
  }

  request->send(200, "application/json", "{\"success\":true,\"message\":\"Alarm acknowledged\"}");
}

void handleSensorControlBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  String clientIP = request->client()->remoteIP().toString();
  if (rejectIfLowHeap(request)) {
    return;
  }
  if (rejectIfBodyTooLarge(request, total)) {
    return;
  }
  if (isIpBlocked(clientIP)) {
    request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
    return;
  }
  if (!tryReserveConnection(clientIP)) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  ConnectionGuard guard(true, clientIP);
  if (!checkRateLimit(clientIP)) {
    sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
    return;
  }

  totalRequests++;

  if (!isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }

  String role = getRequestRole(request);
  if (role != "admin" && role != "operator" && role != "maintenance") {
    request->send(403, "application/json", "{\"error\":\"Insufficient permissions\"}");
    return;
  }

  if (ESP.getFreeHeap() < 50000) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, (char*)data, len);
  if (error) {
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String sensorId = doc["sensor_id"].as<String>();
  bool enabled = doc["enabled"] | true;

  Serial.printf("[API] POST /api/sensors/control sensor=%s enabled=%d from %s (%s)\n",
                sensorId.c_str(), enabled ? 1 : 0, clientIP.c_str(), role.c_str());

  int idx = -1;
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    if (strcmp(sensors[i].id, sensorId.c_str()) == 0) {
      idx = i;
      break;
    }
  }

  if (idx < 0) {
    request->send(404, "application/json", "{\"error\":\"Sensor not found\"}");
    return;
  }

  // Disallow enabling a sensor that is currently faulted unless the requester is an admin.
  if (enabled && sensors[idx].faulted) {
    if (role == "admin") {
      // Admin-override: activate the sensor and clear the fault flag
      sensors[idx].faulted = false;
      sensors[idx].enabled = true;
      Serial.printf("[API] Admin override: enabled faulted sensor %s from %s\n", sensorId.c_str(), clientIP.c_str());

      String response = "{\"success\":true,\"sensor_id\":\"" + sensorId +
                       "\",\"enabled\":true,\"faulted\":false,\"override\":true}";
      request->send(200, "application/json", response);
      return;
    } else {
      sensors[idx].enabled = false; // ensure it's disabled
      Serial.printf("[API] Rejected enabling faulted sensor %s from %s (role=%s)\n", sensorId.c_str(), clientIP.c_str(), role.c_str());
      request->send(409, "application/json", "{\"error\":\"Sensor is faulted and cannot be enabled\"}");
      return;
    }
  }

  sensors[idx].enabled = enabled;

  String response = "{\"success\":true,\"sensor_id\":\"" + sensorId +
                   "\",\"enabled\":" + String(enabled ? "true" : "false") + "}";
  request->send(200, "application/json", response);
}

void handleActuatorControlBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  String clientIP = request->client()->remoteIP().toString();
  if (rejectIfLowHeap(request)) {
    return;
  }
  if (rejectIfBodyTooLarge(request, total)) {
    return;
  }
  if (isIpBlocked(clientIP)) {
    request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
    return;
  }
  if (!tryReserveConnection(clientIP)) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  ConnectionGuard guard(true, clientIP);
  if (!checkRateLimit(clientIP)) {
    sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
    return;
  }

  totalRequests++;

  if (!isAuthenticated(request)) {
    Serial.printf("[API] Actuator control UNAUTHORIZED from %s (hasAuth=%d)\n",
                  clientIP.c_str(), request->hasHeader("Authorization") ? 1 : 0);
    request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }

  String role = getRequestRole(request);
  if (role != "admin" && role != "operator" && role != "maintenance") {
    request->send(403, "application/json", "{\"error\":\"Insufficient permissions\"}");
    return;
  }

  if (ESP.getFreeHeap() < 50000) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, (char*)data, len);
  if (error) {
    Serial.printf("[API] Actuator JSON parse error: %s\n", error.c_str());
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String actuatorId = doc["actuator_id"].as<String>();
  String cmd = doc["command"].as<String>();
  float param = doc["param"] | 0.0f;

  Serial.printf("[API] POST /api/actuators/control id=%s cmd=%s param=%.1f from %s (%s)\n",
                actuatorId.c_str(), cmd.c_str(), param, clientIP.c_str(), role.c_str());

  if (!VULN_RACE_ACTUATORS) {
    if (actuatorMutex == nullptr || xSemaphoreTake(actuatorMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
  }
  String result = executeActuatorCommand(actuatorId.c_str(), cmd.c_str(), param);
  if (!VULN_RACE_ACTUATORS && actuatorMutex != nullptr) {
    xSemaphoreGive(actuatorMutex);
  }
  request->send(200, "application/json", result);
}

// Reset a sensor's fault flag (accessible to admin and operators). This does NOT enable the sensor.
void handleSensorResetBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  String clientIP = request->client()->remoteIP().toString();
  if (rejectIfLowHeap(request)) {
    return;
  }
  if (rejectIfBodyTooLarge(request, total)) {
    return;
  }
  if (isIpBlocked(clientIP)) {
    request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
    return;
  }
  if (!tryReserveConnection(clientIP)) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  ConnectionGuard guard(true, clientIP);
  if (!checkRateLimit(clientIP)) {
    sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
    return;
  }

  totalRequests++;

  if (!isAuthenticated(request)) {
    Serial.printf("[API] Sensor reset UNAUTHORIZED from %s (hasAuth=%d)\n",
                  clientIP.c_str(), request->hasHeader("Authorization") ? 1 : 0);
    request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }

  String role = getRequestRole(request);
  if (role != "admin" && role != "operator") {
    request->send(403, "application/json", "{\"error\":\"Insufficient permissions\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, (char*)data, len);
  if (error) {
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String sensorId = doc["sensor_id"].as<String>();

  Serial.printf("[API] POST /api/sensors/reset sensor=%s from %s (%s)\n",
                sensorId.c_str(), clientIP.c_str(), role.c_str());

  int idx = -1;
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    if (strcmp(sensors[i].id, sensorId.c_str()) == 0) {
      idx = i;
      break;
    }
  }

  if (idx < 0) {
    request->send(404, "application/json", "{\"error\":\"Sensor not found\"}");
    return;
  }

  sensors[idx].faulted = false;
  // Keep sensor disabled after reset; operator/admin can enable explicitly via control endpoint
  sensors[idx].enabled = false;

  String response = "{\"success\":true,\"sensor_id\":\"" + sensorId +
                   "\",\"faulted\":false,\"enabled\":false}";
  request->send(200, "application/json", response);
}
