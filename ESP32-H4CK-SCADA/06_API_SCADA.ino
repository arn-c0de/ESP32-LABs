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
      handleLoginJSON(request, data, len, total);
    });
  
  server.on("/api/logout", HTTP_GET, handleLogout);
  server.on("/api/logout", HTTP_POST, handleLogout);

  // ===== DASHBOARD API =====
  
  server.on("/api/dashboard/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    rateLimitedLog("[API] GET /api/dashboard/status from " + clientIP);
    
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
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    rateLimitedLog("[API] GET /api/sensors from " + clientIP);
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    String json = getSensorListJSON();
    request->send(200, "application/json", json);
  });
  
  // Get sensor reading history (VULN: IDOR - no access control on sensor ID)
  server.addHandler(new SensorReadingsHandler());

  // Sensor control (enable/disable) - flat URL, ID in JSON body
  server.on("/api/sensors/control", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      handleSensorControlBody(request, data, len, index, total);
    });

  // Sensor reset (clears 'faulted' flag) - accessible to Operator+Admin
  server.on("/api/sensors/reset", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      handleSensorResetBody(request, data, len, index, total);
    });
  
  // ===== ACTUATOR APIs =====
  
  // Get all actuators
  server.on("/api/actuators", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    rateLimitedLog("[API] GET /api/actuators from " + clientIP);
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    String json = getActuatorListJSON();
    request->send(200, "application/json", json);
  });
  
  // Control actuator (requires operator role minimum) - flat URL, ID in JSON body
  server.on("/api/actuators/control", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      handleActuatorControlBody(request, data, len, index, total);
    });
  
  // ===== ALARM APIs =====
  
  // Get alarm history (optional line filter)
  server.on("/api/alarms", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    rateLimitedLog("[API] GET /api/alarms from " + clientIP);
    
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
  
  // Acknowledge alarm (requires operator role minimum) - flat URL, ID in JSON body
  server.on("/api/alarms/ack", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      handleAlarmAckBody(request, data, len, index, total);
    });
  
  // ===== SYSTEM INFO API =====
  
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    
    totalRequests++;
    rateLimitedLog("[API] GET /api/info from " + clientIP);
    
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
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    totalRequests++;
    rateLimitedLog("[API] GET /api/defense/status from " + clientIP);
    
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
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    totalRequests++;
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    // Return empty logs for now
    request->send(200, "application/json", "[]");
  });

  server.on("/api/defense/alerts", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    if (!requireRole(request, "operator")) {
      return;
    }
    String output = getDefenseAlertsJSON();
    request->send(200, "application/json", output);
  });

  // ===== ADMIN DEFENSE API (Operator/Admin) =====
  server.on("/api/admin/defense/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    if (!requireRole(request, "operator")) {
      return;
    }
    String output = handleAdminDefenseStatus();
    request->send(200, "application/json", output);
  });

  server.on("/api/admin/defense/block", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (rejectIfBodyTooLarge(request, total)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    if (!requireRole(request, "operator")) {
      return;
    }

    if (ESP.getFreeHeap() < MIN_FREE_HEAP) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, (char*)data, len);
    if (error) {
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    String targetIp = doc["ip"] | "";
    bool permanent = doc["permanent"] | false;
    unsigned long duration = doc["duration"] | 300;
    if (targetIp == "") {
      request->send(400, "application/json", "{\"error\":\"Missing ip\"}");
      return;
    }
    String by = getRequestUsername(request);
    if (by == "") by = "operator";
    addBlock(targetIp, duration, permanent, by);
    request->send(200, "application/json", "{\"success\":true}");
  });

  server.on("/api/admin/defense/unblock", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (rejectIfBodyTooLarge(request, total)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    if (!requireRole(request, "operator")) {
      return;
    }

    if (ESP.getFreeHeap() < MIN_FREE_HEAP) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, (char*)data, len);
    if (error) {
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    String targetIp = doc["ip"] | "";
    if (targetIp == "") {
      request->send(400, "application/json", "{\"error\":\"Missing ip\"}");
      return;
    }
    removeBlock(targetIp);
    request->send(200, "application/json", "{\"success\":true}");
  });

  // ===== WIFI CLIENT HISTORY (AP Mode) =====
  server.on("/api/wifi/clients", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    rateLimitedLog("[API] GET /api/wifi/clients from " + clientIP);
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    String json = getWiFiClientsJSON();
    request->send(200, "application/json", json);
  });

  // ===== INCIDENT SPAWNING =====
  server.on("/api/incidents/spawn", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (rejectIfBodyTooLarge(request, total)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
      return;
    }
    
    totalRequests++;
    rateLimitedLog("[API] POST /api/incidents/spawn from " + clientIP);
    
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    // Parse JSON body
    if (ESP.getFreeHeap() < MIN_FREE_HEAP) {
      request->send(503, "application/json", "{\"error\":\"Server busy\"}");
      return;
    }
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) {
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    String type = doc["type"] | "unknown";
    String details = doc["details"] | "";
    
    // Trigger incident based on type
    bool success = triggerIncident(type, details);
    
    if (success) {
      DynamicJsonDocument responseDoc(256);
      responseDoc["success"] = true;
      responseDoc["type"] = type;
      responseDoc["message"] = "Incident spawned successfully";
      
      String response;
      serializeJson(responseDoc, response);
      request->send(200, "application/json", response);
      Serial.printf("[INCIDENT] Spawned: %s - %s\n", type.c_str(), details.c_str());
    } else {
      request->send(500, "application/json", "{\"error\":\"Failed to spawn incident\"}");
    }
  });
  
  Serial.println("[API] SCADA routes configured");
}
