/*
 * Web Server Module - ESP32-H4CK-SCADA
 *
 * Serves HTML pages from LittleFS and sets up basic page routes.
 * All pages are stored in data/html/ directory.
 * No inline HTML - everything served from filesystem.
 */

void initWebServer() {
  // Enable reuse of connections and set timeouts
  DefaultHeaders::Instance().addHeader("Connection", "close");
  
  setupRoutes();
  serveStaticFiles();
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.printf("[WEBSERVER] Server started on port %d\n", HTTP_PORT);
  Serial.printf("[WEBSERVER] Max concurrent connections: %d\n", MAX_HTTP_CONNECTIONS);
  Serial.printf("[WEBSERVER] Access at: http://%s/\n", getLocalIP().c_str());
}

void setupRoutes() {
  // Home / Index page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET / from " + clientIP);
    request->send(LittleFS, "/html/index.html", "text/html");
  });

  // Login page (login form is on index page)
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /login from " + clientIP);
    request->send(LittleFS, "/html/index.html", "text/html");
  });

  // Dashboard page
  server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /dashboard from " + clientIP);
    request->send(LittleFS, "/html/dashboard.html", "text/html");
  });

  // Sensors page
  server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /sensors from " + clientIP);
    request->send(LittleFS, "/html/sensors.html", "text/html");
  });

  // Actuators page
  server.on("/actuators", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /actuators from " + clientIP);
    request->send(LittleFS, "/html/actuators.html", "text/html");
  });

  // Alarms page
  server.on("/alarms", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /alarms from " + clientIP);
    request->send(LittleFS, "/html/alarms.html", "text/html");
  });

  // Vulnerabilities page
  server.on("/vulnerabilities", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /vulnerabilities from " + clientIP);
    request->send(LittleFS, "/html/vulnerabilities.html", "text/html");
  });

  // Incidents page
  server.on("/incidents", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /incidents from " + clientIP);
    request->send(LittleFS, "/html/incidents.html", "text/html");
  });

  // Admin page (requires admin role)
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /admin from " + clientIP);

    if (!requireAdmin(request)) {
      return;
    }
    request->send(LittleFS, "/html/admin.html", "text/html");
  });

  // Defense page
  server.on("/defense", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /defense from " + clientIP);
    request->send(LittleFS, "/html/defense.html", "text/html");
  });

  // Incidents page
  server.on("/incidents", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      sendRateLimited(request, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /incidents from " + clientIP);
    request->send(LittleFS, "/html/incidents.html", "text/html");
  });

  // Flags page
  server.on("/flags", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /flags from " + clientIP);
    request->send(LittleFS, "/html/flags.html", "text/html");
  });

  // Leaderboard page
  server.on("/leaderboard", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    rateLimitedLog("[HTTP] GET /leaderboard from " + clientIP);
    request->send(LittleFS, "/html/leaderboard.html", "text/html");
  });

  // Mobile page
  server.on("/mobile", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    request->send(LittleFS, "/html/mobile.html", "text/html");
  });

  // robots.txt with SCADA-specific hints
  server.on("/robots.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (rejectIfLowHeap(request)) {
      return;
    }
    if (!tryReserveConnection(clientIP)) {
      request->send(503, "text/plain", "Server busy");
      return;
    }
    ConnectionGuard guard(true);
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    String robots = "User-agent: *\n";
    robots += "Disallow: /admin\n";
    robots += "Disallow: /api/admin/\n";
    robots += "Disallow: /debug\n";
    robots += "Disallow: /.env\n";
    robots += "# SCADA HMI v" + String(LAB_VERSION) + "\n";
    robots += "# Modbus TCP on port 502 (disabled)\n";
    robots += "# Internal note: Check /api/endpoints for full API map\n";
    request->send(200, "text/plain", robots);
  });

  Serial.println("[WEBSERVER] Routes configured");
}

void serveStaticFiles() {
  server.serveStatic("/css/", LittleFS, "/css/");
  server.serveStatic("/js/", LittleFS, "/js/");

  Serial.println("[WEBSERVER] Static file serving enabled");
}

void handleNotFound(AsyncWebServerRequest *request) {
  totalRequests++;
  String clientIP = request->client()->remoteIP().toString();
  if (rejectIfLowHeap(request)) {
    return;
  }
  if (!tryReserveConnection(clientIP)) {
    request->send(503, "text/plain", "Server busy");
    return;
  }
  ConnectionGuard guard(true);
  if (isIpBlocked(clientIP)) {
    request->send(403, "text/plain", "Access Denied");
    return;
  }
  if (!checkRateLimit(clientIP)) {
    sendRateLimited(request, "text/plain", "Too Many Requests");
    return;
  }
  if (!checkNotFoundBackoff(clientIP)) {
    sendRateLimited(request, "text/plain", "Too Many Requests");
    return;
  }

  String message = "404 - Not Found\nURI: " + request->url() + "\n";
  AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", message);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
  rateLimitedLog("[HTTP] 404 " + request->url() + " from " + clientIP);
}
