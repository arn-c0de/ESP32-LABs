/*
 * Web Server Module - ESP32-H4CK-SCADA
 *
 * Serves HTML pages from LittleFS and sets up basic page routes.
 * All pages are stored in data/html/ directory.
 * No inline HTML - everything served from filesystem.
 */

void initWebServer() {
  setupRoutes();
  serveStaticFiles();
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.printf("[WEBSERVER] Server started on port %d\n", HTTP_PORT);
  Serial.printf("[WEBSERVER] Access at: http://%s/\n", getLocalIP().c_str());
}

void setupRoutes() {
  // Home / Index page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET / from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/index.html", "text/html");
  });

  // Login page (login form is on index page)
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /login from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/index.html", "text/html");
  });

  // Dashboard page
  server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /dashboard from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/dashboard.html", "text/html");
  });

  // Sensors page
  server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /sensors from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/sensors.html", "text/html");
  });

  // Actuators page
  server.on("/actuators", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /actuators from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/actuators.html", "text/html");
  });

  // Alarms page
  server.on("/alarms", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /alarms from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/alarms.html", "text/html");
  });

  // Vulnerabilities page
  server.on("/vulnerabilities", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /vulnerabilities from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/vulnerabilities.html", "text/html");
  });

  // Incidents page
  server.on("/incidents", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /incidents from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/incidents.html", "text/html");
  });

  // Admin page (requires admin role)
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /admin from %s\n", clientIP.c_str());

    if (!requireAdmin(request)) {
      return;
    }
    request->send(LittleFS, "/html/admin.html", "text/html");
  });

  // Defense page
  server.on("/defense", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /defense from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/defense.html", "text/html");
  });

  // Incidents page
  server.on("/incidents", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /incidents from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/incidents.html", "text/html");
  });

  // Flags page
  server.on("/flags", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /flags from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/flags.html", "text/html");
  });

  // Leaderboard page
  server.on("/leaderboard", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    if (isIpBlocked(clientIP)) {
      request->send(403, "text/plain", "Access Denied");
      return;
    }
    if (!checkRateLimit(clientIP)) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
    totalRequests++;
    Serial.printf("[HTTP] GET /leaderboard from %s\n", clientIP.c_str());
    request->send(LittleFS, "/html/leaderboard.html", "text/html");
  });

  // Mobile page
  server.on("/mobile", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
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

  // Rate limit 404 responses to slow down directory brute-forcing
  static unsigned long last404Time = 0;
  static int count404 = 0;
  unsigned long now = millis();

  if (now - last404Time < 1000) {
    count404++;
    if (count404 > 50) {
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
  } else {
    count404 = 0;
    last404Time = now;
  }

  String message = "404 - Not Found\nURI: " + request->url() + "\n";

  // Intentionally verbose headers - info disclosure vulnerability
  AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", message);
  response->addHeader("Server", "ESP32-SCADA-HMI/" + String(LAB_VERSION));
  response->addHeader("X-Powered-By", "Arduino/ESP32, LittleFS, AsyncWebServer");
  response->addHeader("X-Device-Model", ESP.getChipModel());
  response->addHeader("X-Firmware-Version", LAB_VERSION);
  response->addHeader("X-SCADA-Protocol", "Modbus-TCP-Sim");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);

  if (count404 % 10 == 0) {
    logDebug("404 Not Found: " + request->url());
  }
}
