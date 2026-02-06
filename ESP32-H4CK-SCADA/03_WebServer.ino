// ============================================================
// 03_WebServer.ino — HTTP Routes + WebSocket + Static Files
// ============================================================

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// WebSocket event handler
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    debugLogf("WS", "Client connected: #%u from %s", client->id(),
              client->remoteIP().toString().c_str());
  } else if (type == WS_EVT_DISCONNECT) {
    debugLogf("WS", "Client disconnected: #%u", client->id());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String msg = "";
      for (size_t i = 0; i < len; i++) msg += (char)data[i];
      // Handle incoming WS messages (e.g., subscribe to specific data)
      debugLogf("WS", "Message from #%u: %s", client->id(), msg.c_str());
    }
  }
}

// Broadcast JSON to all WS clients
void wsBroadcast(const String& json) {
  ws.textAll(json);
}

// Client IP helper
String getClientIP(AsyncWebServerRequest* request) {
  return request->client()->remoteIP().toString();
}

// ===== CORS headers =====
void addCorsHeaders(AsyncWebServerResponse* response) {
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Session");
}

// ===== Send JSON response =====
void sendJson(AsyncWebServerRequest* request, int code, const String& json) {
  AsyncWebServerResponse* response = request->beginResponse(code, "application/json", json);
  addCorsHeaders(response);
  request->send(response);
}

// ===== Defense check middleware =====
bool checkDefense(AsyncWebServerRequest* request) {
  String ip = getClientIP(request);
  if (isIPBlocked(ip)) {
    sendJson(request, 403, jsonError("Access denied - IP blocked", 403));
    return false;
  }
  if (RATE_LIMIT_ACTIVE && !checkRateLimit(ip)) {
    sendJson(request, 429, jsonError("Rate limit exceeded", 429));
    return false;
  }
  return true;
}

// ===== Role check middleware =====
bool requireRole(AsyncWebServerRequest* request, UserRole minRole) {
  UserRole role = getRequestRole(request);
  if (role == ROLE_NONE) {
    sendJson(request, 401, jsonError("Authentication required", 401));
    return false;
  }
  if (role > minRole) {  // ADMIN=0, OPERATOR=1, etc.
    sendJson(request, 403, jsonError("Insufficient permissions", 403));
    return false;
  }
  return true;
}

// ============================================================
// Route Setup
// ============================================================
void setupAPIRoutes() {
  // ===== CORS preflight =====
  server.onNotFound([](AsyncWebServerRequest* request) {
    if (request->method() == HTTP_OPTIONS) {
      AsyncWebServerResponse* response = request->beginResponse(200);
      addCorsHeaders(response);
      request->send(response);
    } else {
      sendJson(request, 404, jsonError("Not found", 404));
    }
  });

  // ===== AUTH ROUTES =====
  server.on("/api/auth/login", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!checkDefense(request)) return;

    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];

    JsonDocument doc;
    if (deserializeJson(doc, body)) {
      sendJson(request, 400, jsonError("Invalid JSON", 400));
      return;
    }

    String username = doc["username"] | "";
    String password = doc["password"] | "";
    UserRole role = authenticateUser(username, password);

    if (role == ROLE_NONE) {
      logDefenseEvent("AUTH_FAIL", getClientIP(request),
        "Failed login: " + username);
      sendJson(request, 401, jsonError("Invalid credentials", 401));
      return;
    }

    String sessId = createSession(username, role, getClientIP(request));
    String jwt = generateJWT(username, role);

    // Register player in gameplay
    gameplayRegisterPlayer(sessId, username, role);

    JsonDocument resp;
    resp["success"]    = true;
    resp["session_id"] = sessId;
    resp["token"]      = jwt;
    resp["username"]   = username;
    resp["role"]       = roleToString(role);

    String out;
    serializeJson(resp, out);

    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", out);
    addCorsHeaders(response);
    response->addHeader("Set-Cookie", "session=" + sessId + "; Path=/; HttpOnly");
    request->send(response);
  });

  server.on("/api/auth/logout", HTTP_POST, [](AsyncWebServerRequest* request) {
    Session* sess = getRequestSession(request);
    if (sess) destroySession(sess->sessionId);
    sendJson(request, 200, jsonSuccess("Logged out"));
  });

  server.on("/api/auth/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    Session* sess = getRequestSession(request);
    if (!sess) {
      sendJson(request, 200, "{\"authenticated\":false}");
      return;
    }
    JsonDocument doc;
    doc["authenticated"] = true;
    doc["username"] = sess->username;
    doc["role"]     = roleToString(sess->role);
    doc["session"]  = sess->sessionId;
    String out;
    serializeJson(doc, out);
    sendJson(request, 200, out);
  });

  // ===== SENSOR ROUTES =====
  server.on("/api/sensors/list", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;
    String data = dbGetSensors();
    if (data.isEmpty()) data = "{\"sensors\":[]}";
    sendJson(request, 200, data);
  });

  server.on("/api/sensor/reading", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;

    String sensorId = request->arg("sensor_id");
    int limit = request->hasArg("limit") ? request->arg("limit").toInt() : 50;
    if (limit <= 0) limit = 50;
    if (limit > 500) limit = 500;

    if (sensorId.isEmpty()) {
      sendJson(request, 400, jsonError("Missing sensor_id parameter", 400));
      return;
    }

    // IDOR Vulnerability: no cross-line check when VULN_IDOR_SENSORS is true
    UserRole role = getRequestRole(request);
    Session* sess = getRequestSession(request);

    if (!VULN_IDOR_SENSORS && sess) {
      // Proper access check: operator can only see their assigned line
      // For the lab, we skip this check when VULN_IDOR_SENSORS is true
      // This IS the vulnerability students should find
    }

    // Log potential IDOR attempt
    if (sess && sensorId.length() > 8) {
      String sensorLine = sensorId.substring(7, 8);
      logDefenseEvent("SENSOR_ACCESS", getClientIP(request),
        "Sensor read: " + sensorId);
    }

    String readings = getSensorReadings(sensorId, limit);
    sendJson(request, 200, readings);
  });

  // ===== ACTUATOR ROUTES =====
  server.on("/api/actuators/list", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;
    String data = dbGetActuators();
    if (data.isEmpty()) data = "{\"actuators\":[]}";
    sendJson(request, 200, data);
  });

  server.on("/api/actuators/control", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!checkDefense(request)) return;
    if (!requireRole(request, ROLE_OPERATOR)) return;

    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];

    JsonDocument doc;
    if (deserializeJson(doc, body)) {
      sendJson(request, 400, jsonError("Invalid JSON", 400));
      return;
    }

    String actuatorId = doc["id"] | "";
    String cmd        = doc["cmd"] | "";

    // Command Injection vulnerability
    String result = executeActuatorCommand(actuatorId, cmd, doc["params"]);
    sendJson(request, 200, result);
  });

  // ===== ALARM ROUTES =====
  server.on("/api/alarms/history", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;

    String line = request->arg("line");
    int limit = request->hasArg("limit") ? request->arg("limit").toInt() : 50;

    // IDOR on alarm history (VULN_IDOR_ALARMS)
    String data = getAlarmHistory(line, limit);
    sendJson(request, 200, data);
  });

  server.on("/api/alarms/active", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;
    sendJson(request, 200, getActiveAlarms());
  });

  // ===== INCIDENT ROUTES =====
  server.on("/api/incidents/list", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;
    sendJson(request, 200, getActiveIncidents());
  });

  server.on("/api/incidents/report", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!checkDefense(request)) return;
    if (!requireRole(request, ROLE_OPERATOR)) return;

    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    String result = submitIncidentReport(body);
    sendJson(request, 200, result);
  });

  // ===== HINT ROUTES =====
  server.on("/api/hints", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;
    String endpoint = request->arg("endpoint");
    int level = request->hasArg("level") ? request->arg("level").toInt() : 1;
    Session* sess = getRequestSession(request);
    String sessId = sess ? sess->sessionId : "";
    sendJson(request, 200, getHint(endpoint, level, sessId));
  });

  // ===== FLAG ROUTES =====
  server.on("/api/flags/submit", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!checkDefense(request)) return;

    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];

    Session* sess = getRequestSession(request);
    String sessId = sess ? sess->sessionId : "";
    String result = submitFlag(body, sessId, getClientIP(request));
    sendJson(request, 200, result);
  });

  server.on("/api/flags/progress", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;
    Session* sess = getRequestSession(request);
    String sessId = sess ? sess->sessionId : "";
    sendJson(request, 200, getPlayerProgress(sessId));
  });

  // ===== RACE CONDITION TEST =====
  server.on("/api/test/race", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;

    String actuator = request->arg("actuator");
    int count = request->hasArg("count") ? request->arg("count").toInt() : 10;

    if (!VULN_RACE_ACTUATORS) {
      sendJson(request, 403, jsonError("Race condition testing disabled", 403));
      return;
    }

    String result = triggerRaceCondition(actuator, count);
    sendJson(request, 200, result);
  });

  // ===== DASHBOARD STATUS =====
  server.on("/api/dashboard/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;
    sendJson(request, 200, getDashboardStatus());
  });

  // ===== DEFENSE ROUTES =====
  server.on("/api/defense/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!DEFENSE_ENABLED) {
      sendJson(request, 200, "{\"defense_enabled\":false}");
      return;
    }
    sendJson(request, 200, getDefenseStatus());
  });

  server.on("/api/defense/alerts", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!IDS_ACTIVE) {
      sendJson(request, 200, "{\"ids_active\":false,\"alerts\":[]}");
      return;
    }
    sendJson(request, 200, getDefenseAlerts());
  });

  server.on("/api/defense/block-ip", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!requireRole(request, ROLE_ADMIN)) return;

    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    JsonDocument doc;
    if (deserializeJson(doc, body)) {
      sendJson(request, 400, jsonError("Invalid JSON", 400));
      return;
    }
    String ip = doc["ip"] | "";
    int duration = doc["duration"] | BLOCK_DURATION_SEC;
    blockIP(ip, duration);
    sendJson(request, 200, jsonSuccess("IP blocked: " + ip));
  });

  // ===== ADMIN ROUTES =====
  server.on("/api/admin/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!requireRole(request, ROLE_ADMIN)) return;
    sendJson(request, 200, configGetJson());
  });

  server.on("/api/admin/config", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!requireRole(request, ROLE_ADMIN)) return;
    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    if (configUpdateFromJson(body)) {
      sendJson(request, 200, jsonSuccess("Configuration updated"));
    } else {
      sendJson(request, 400, jsonError("Invalid configuration", 400));
    }
  });

  server.on("/api/admin/leaderboard", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!LEADERBOARD_ENABLED) {
      sendJson(request, 200, "{\"enabled\":false}");
      return;
    }
    String sort = request->arg("sort");
    int limit = request->hasArg("limit") ? request->arg("limit").toInt() : 10;
    sendJson(request, 200, getLeaderboard(sort, limit));
  });

  server.on("/api/admin/incidents/create", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!requireRole(request, ROLE_ADMIN)) return;
    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    String result = createManualIncident(body);
    sendJson(request, 200, result);
  });

  server.on("/api/admin/sessions", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!requireRole(request, ROLE_ADMIN)) return;
    sendJson(request, 200, authStatusJson());
  });

  server.on("/api/admin/flag", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!requireRole(request, ROLE_ADMIN)) return;
    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];
    String result = submitRootFlag(body);
    sendJson(request, 200, result);
  });

  // ===== EQUIPMENT =====
  server.on("/api/equipment/list", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;
    String data = dbGetEquipment();
    if (data.isEmpty()) data = "{\"lines\":[]}";
    sendJson(request, 200, data);
  });

  // ===== MAINTENANCE LOGS =====
  server.on("/api/maintenance/logs", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;
    if (!requireRole(request, ROLE_MAINTENANCE)) return;
    sendJson(request, 200, dbGetMaintenance());
  });

  // ===== WIFI STATUS =====
  server.on("/api/system/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
    sendJson(request, 200, wifiStatusJson());
  });

  server.on("/api/system/info", HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["heap_free"]  = ESP.getFreeHeap();
    doc["heap_total"] = ESP.getHeapSize();
    doc["uptime_sec"] = millis() / 1000;
    doc["chip_model"] = ESP.getChipModel();
    doc["cpu_freq"]   = ESP.getCpuFreqMHz();
    doc["flash_size"] = ESP.getFlashChipSize();
    String out;
    serializeJson(doc, out);
    sendJson(request, 200, out);
  });

  // ===== VULNERABILITY ENDPOINTS (Lab-only) =====
  setupVulnRoutes();
}

// ============================================================
// Static file serving
// ============================================================
void setupStaticFiles() {
  // Debug endpoint to check LittleFS
  server.on("/debug/fs", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = "<html><body><h1>LittleFS Debug</h1><ul>";
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
      html += "<li>" + String(file.name()) + " (" + String(file.size()) + " bytes)</li>";
      file = root.openNextFile();
    }
    html += "</ul></body></html>";
    request->send(200, "text/html", html);
  });

  // Root redirect
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!LittleFS.exists("/html/index.html")) {
      String msg = "{\"error\":\"LittleFS not mounted or empty\"}";
      request->send(500, "application/json", msg);
      return;
    }
    request->send(LittleFS, "/html/index.html", "text/html");
  });

  // Serve HTML pages
  server.serveStatic("/html/", LittleFS, "/html/");
  server.serveStatic("/css/", LittleFS, "/css/");
  server.serveStatic("/js/", LittleFS, "/js/");

  // Explicit page routes
  server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/dashboard.html", "text/html");
  });
  server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/sensors.html", "text/html");
  });
  server.on("/actuators", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/actuators.html", "text/html");
  });
  server.on("/alarms", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/alarms.html", "text/html");
  });
  server.on("/incidents", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/incidents.html", "text/html");
  });
  server.on("/defense", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/defense.html", "text/html");
  });
  server.on("/flags", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/flags.html", "text/html");
  });
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/admin.html", "text/html");
  });
  server.on("/leaderboard", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/leaderboard.html", "text/html");
  });
  server.on("/mobile", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/mobile.html", "text/html");
  });
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/html/index.html", "text/html");
  });
}

// ============================================================
// webServerInit
// ============================================================
void webServerInit() {
  Serial.println("[WEB] Initializing web server...");

  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // API routes
  setupAPIRoutes();

  // Static files
  setupStaticFiles();

  // Start server
  server.begin();
  Serial.printf("[WEB] Server started on port %d\n", WEB_PORT);
  Serial.printf("[WEB] Dashboard: http://%s/dashboard\n", wifiGetIP().c_str());
}
