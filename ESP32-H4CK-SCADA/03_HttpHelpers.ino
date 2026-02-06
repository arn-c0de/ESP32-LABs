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

// Custom handler for sensor readings
class SensorReadingsHandler : public AsyncWebHandler {
public:
  SensorReadingsHandler() {}
  virtual ~SensorReadingsHandler() {}

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

// Custom handler for alarm acknowledgement
class AlarmAckHandler : public AsyncWebHandler {
public:
  AlarmAckHandler() {}
  virtual ~AlarmAckHandler() {}

  bool canHandle(AsyncWebServerRequest *request) override {
    if (request->method() != HTTP_POST) return false;
    String url = request->url();
    return url.startsWith("/api/alarms/") && url.endsWith("/ack");
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
    
    String role = getRequestRole(request);
    if (role != "admin" && role != "operator" && role != "maintenance") {
      request->send(403, "application/json", "{\"error\":\"Insufficient permissions\"}");
      return;
    }
    
    String sensorId = extractIdFromUrl(request->url(), "/api/alarms/", "/ack");
    unsigned long timestamp = 0;
    
    if (request->hasParam("timestamp")) {
      timestamp = request->getParam("timestamp")->value().toInt();
    }
    
    Serial.printf("[API] POST /api/alarms/%s/ack (ts=%lu) from %s (%s)\n",
                  sensorId.c_str(), timestamp, clientIP.c_str(), role.c_str());
    
    bool found = false;
    int total = (alarmCount < MAX_ALARMS) ? alarmCount : MAX_ALARMS;
    
    for (int i = 0; i < total; i++) {
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
};

// Custom handler for actuator control
class ActuatorControlHandler : public AsyncWebHandler {
private:
  String _bodyData;
  
public:
  ActuatorControlHandler() {}
  virtual ~ActuatorControlHandler() {}

  bool canHandle(AsyncWebServerRequest *request) override {
    if (request->method() != HTTP_POST) return false;
    String url = request->url();
    return url.startsWith("/api/actuators/") && url.endsWith("/control");
  }

  void handleRequest(AsyncWebServerRequest *request) override {
    // This will be called by handleBody after body is received
  }
  
  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override {
    String clientIP = request->client()->remoteIP().toString();
    
    // Accumulate body data
    if (index == 0) {
      _bodyData = "";
    }
    for (size_t i = 0; i < len; i++) {
      _bodyData += (char)data[i];
    }
    
    // Only process when all data received
    if (index + len != total) {
      return;
    }
    
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
    
    String actuatorId = extractIdFromUrl(request->url(), "/api/actuators/", "/control");
    
    Serial.printf("[API] POST /api/actuators/%s/control from %s (%s)\n",
                  actuatorId.c_str(), clientIP.c_str(), role.c_str());
    
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, _bodyData);
    
    if (error) {
      Serial.printf("[API] JSON parse error: %s\n", error.c_str());
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    String cmd = doc["command"].as<String>();
    float param = doc["param"] | 0.0f;
    
    Serial.printf("[API] Command: %s, param: %.1f\n", cmd.c_str(), param);
    
    String result = executeActuatorCommand(actuatorId.c_str(), cmd.c_str(), param);
    request->send(200, "application/json", result);
  }
};
