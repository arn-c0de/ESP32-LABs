/*
 * HTTP Helper Functions
 */

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
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
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

  if (isIpBlocked(clientIP)) {
    request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
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

  if (isIpBlocked(clientIP)) {
    request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
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

  sensors[idx].enabled = enabled;

  String response = "{\"success\":true,\"sensor_id\":\"" + sensorId +
                   "\",\"enabled\":" + String(enabled ? "true" : "false") + "}";
  request->send(200, "application/json", response);
}

void handleActuatorControlBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  String clientIP = request->client()->remoteIP().toString();

  if (isIpBlocked(clientIP)) {
    request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
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

  String result = executeActuatorCommand(actuatorId.c_str(), cmd.c_str(), param);
  request->send(200, "application/json", result);
}
