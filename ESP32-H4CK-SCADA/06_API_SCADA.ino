/*
 * SCADA API Module
 *
 * REST API endpoints for sensor monitoring, actuator control,
 * and alarm management. Contains intentional vulnerabilities.
 */

void setupSCADARoutes() {
  // ===== AUTHENTICATION APIs =====
  
  // Login with JSON body handler
  server.on("/api/login", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      handleLoginJSON(request, data, len);
    });
  
  server.on("/api/logout", HTTP_GET, handleLogout);
  server.on("/api/logout", HTTP_POST, handleLogout);

  // ===== DASHBOARD API =====
  
  server.on("/api/dashboard/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    Serial.printf("[API] GET /api/dashboard/status from %s\n", clientIP.c_str());
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    String json = getDashboardStatusJSON();
    request->send(200, "application/json", json);
  });

  // ===== SENSOR APIs =====
  
  // Get all sensors
  server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    Serial.printf("[API] GET /api/sensors from %s\n", clientIP.c_str());
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    String json = getSensorListJSON();
    request->send(200, "application/json", json);
  });
  
  // Get sensor reading history (VULN: IDOR - no access control on sensor ID)
  // Handled by custom SensorReadingsHandler
  server.addHandler(new SensorReadingsHandler());
  
  // ===== ACTUATOR APIs =====
  
  // Get all actuators
  server.on("/api/actuators", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    Serial.printf("[API] GET /api/actuators from %s\n", clientIP.c_str());
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    String json = getActuatorListJSON();
    request->send(200, "application/json", json);
  });
  
  // Control actuator (requires operator role minimum)
  // Handled by custom ActuatorControlHandler
  server.addHandler(new ActuatorControlHandler());
  
  // ===== ALARM APIs =====
  
  // Get alarm history (optional line filter)
  server.on("/api/alarms", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    Serial.printf("[API] GET /api/alarms from %s\n", clientIP.c_str());
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    int line = 0;  // 0 = all lines
    if (request->hasParam("line")) {
      line = request->getParam("line")->value().toInt();
    }
    
    int limit = 50;
    if (request->hasParam("limit")) {
      limit = request->getParam("limit")->value().toInt();
      if (limit < 1) limit = 1;
      if (limit > 200) limit = 200;
    }
    
    String json = getAlarmHistoryJSON(line, limit);
    request->send(200, "application/json", json);
  });
  
  // Acknowledge alarm (requires operator role minimum)
  // Handled by custom AlarmAckHandler
  server.addHandler(new AlarmAckHandler());
  
  // ===== SYSTEM INFO API =====
  
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    
    totalRequests++;
    Serial.printf("[API] GET /api/info from %s\n", clientIP.c_str());
    
    DynamicJsonDocument doc(1024);
    doc["version"] = LAB_VERSION;
    doc["codename"] = CODENAME;
    doc["build_date"] = BUILD_DATE;
    doc["lab_mode"] = LAB_MODE_STR;
    doc["uptime"] = (millis() - systemStartTime) / 1000;
    doc["total_requests"] = totalRequests;
    doc["active_sessions"] = activeSessions.size();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["chip_model"] = ESP.getChipModel();
    doc["chip_cores"] = ESP.getChipCores();
    doc["cpu_freq"] = ESP.getCpuFreqMHz();
    doc["flash_size"] = ESP.getFlashChipSize();
    doc["ip"] = getLocalIP();
    
    // SCADA specific
    doc["num_lines"] = NUM_LINES;
    doc["total_sensors"] = TOTAL_SENSORS;
    doc["total_actuators"] = TOTAL_ACTUATORS;
    doc["alarm_count"] = alarmCount;
    doc["difficulty"] = DIFFICULTY == EASY ? "EASY" : (DIFFICULTY == NORMAL ? "NORMAL" : "HARD");
    
    // Vulnerabilities enabled
    JsonObject vulns = doc.createNestedObject("vulnerabilities");
    vulns["idor_sensors"] = VULN_IDOR_SENSORS;
    vulns["command_inject"] = VULN_COMMAND_INJECT;
    vulns["race_actuators"] = VULN_RACE_ACTUATORS;
    vulns["weak_auth"] = VULN_WEAK_AUTH;
    vulns["hardcoded_secrets"] = VULN_HARDCODED_SECRETS;
    vulns["logic_flaws"] = VULN_LOGIC_FLAWS;
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  // ===== USER MANAGEMENT APIs (admin only) =====
  
  server.on("/api/users", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    totalRequests++;
    Serial.printf("[API] GET /api/users from %s\n", clientIP.c_str());
    
    if (!requireAdmin(request)) {
      return;
    }
    
    String json = getAllUsers();
    request->send(200, "application/json", json);
  });
  
  // ===== CONFIG API (admin only) =====
  
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    totalRequests++;
    Serial.printf("[API] GET /api/config from %s\n", clientIP.c_str());
    
    if (!requireAdmin(request)) {
      return;
    }
    
    DynamicJsonDocument doc(512);
    doc["wifi_ssid"] = WIFI_SSID_STR;
    doc["lab_mode"] = LAB_MODE_STR;
    doc["difficulty"] = DIFFICULTY;
    doc["hints_level"] = HINTS_LEVEL;
    doc["enable_vulnerabilities"] = ENABLE_VULNERABILITIES;
    doc["debug_mode"] = DEBUG_MODE;
    doc["protect_admin"] = PROTECT_ADMIN_ENDPOINTS;
    
    // VULN: Info disclosure
    if (VULN_HARDCODED_SECRETS) {
      doc["wifi_password"] = WIFI_PASSWORD_STR;
      doc["jwt_secret"] = JWT_SECRET_STR;
    }
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  // ===== DEFENSE API =====
  
  server.on("/api/defense/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    totalRequests++;
    Serial.printf("[API] GET /api/defense/status from %s\n", clientIP.c_str());
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    DynamicJsonDocument doc(2048);
    
    // Resources
    JsonObject resources = doc.createNestedObject("resources");
    resources["max_dp"] = defenseConfig.max_dp;
    resources["max_ap"] = defenseConfig.max_ap;
    resources["max_stability"] = defenseConfig.max_stability;
    resources["current_dp"] = defenseConfig.current_dp;
    resources["current_ap"] = defenseConfig.current_ap;
    resources["current_stability"] = defenseConfig.current_stability;
    
    // Active rules
    JsonArray rulesArr = doc.createNestedArray("rules");
    for (int i = 0; i < activeRuleCount && i < 32; i++) {
      if (activeRules[i].active) {
        JsonObject ruleObj = rulesArr.createNestedObject();
        ruleObj["id"] = activeRules[i].id;
        ruleObj["type"] = (int)activeRules[i].type;
        ruleObj["target"] = activeRules[i].target;
        ruleObj["createdAt"] = activeRules[i].createdAt;
        ruleObj["expiresAt"] = activeRules[i].expiresAt;
        ruleObj["cost_dp"] = activeRules[i].cost_dp;
        ruleObj["cost_ap"] = activeRules[i].cost_ap;
      }
    }
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  server.on("/api/defense/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    totalRequests++;
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    // Return empty logs for now
    request->send(200, "application/json", "[]");
  });
  
  Serial.println("[API] SCADA routes configured");
}
