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
    totalRequests++;
    addCORSHeaders(request);
    
    if (!ENABLE_VULNERABILITIES) {
      request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
      return;
    }
    
    DynamicJsonDocument doc(512);
    doc["wifi_ssid"] = WIFI_SSID;
    doc["wifi_password"] = WIFI_PASSWORD;  // Intentional exposure
    doc["jwt_secret"] = JWT_SECRET;  // Intentional exposure
    doc["enable_vulnerabilities"] = ENABLE_VULNERABILITIES;
    doc["debug_mode"] = DEBUG_MODE;
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  Serial.println("[API] REST routes configured");
}

void handleGetSystemInfo(AsyncWebServerRequest *request) {
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
  totalRequests++;
  addCORSHeaders(request);
  
  // Intentional vulnerability: No authentication check in vulnerable mode
  if (!VULN_WEAK_AUTH && !isAuthenticated(request)) {
    sendJSONResponse(request, 401, "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  // Check for search parameter (vulnerable to SQL injection)
  if (request->hasParam("search")) {
    String searchTerm = request->getParam("search")->value();
    String result = selectQuery(searchTerm);  // Vulnerable query
    sendJSONResponse(request, 200, result);
    return;
  }
  
  String users = getAllUsers();
  sendJSONResponse(request, 200, users);
}

void handlePostUser(AsyncWebServerRequest *request) {
  totalRequests++;
  addCORSHeaders(request);
  
  // Check authentication
  if (!isAuthenticated(request)) {
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
