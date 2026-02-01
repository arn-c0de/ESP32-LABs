/*
 * Web Server Module
 * 
 * Handles HTTP/HTTPS server initialization, routing, and static file serving.
 * Provides CORS support and SSL configuration (when enabled).
 */

void initWebServer() {
  // Setup all routes
  setupRoutes();
  
  // Setup static file serving
  serveStaticFiles();
  
  // Handle 404 errors
  server.onNotFound(handleNotFound);
  
  // Setup SSL if enabled
  if (SSL_ENABLED) {
    setupSSL();
  }
  
  // Start server
  server.begin();
  
  Serial.printf("[WEBSERVER] Server started on port %d\n", HTTP_PORT);
  Serial.printf("[WEBSERVER] Access at: http://%s/\n", getLocalIP().c_str());
}

void setupRoutes() {
  // Home page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    addCORSHeaders(request);
    if (fileExists("/index.html")) {
      request->send(LittleFS, "/index.html", "text/html");
    } else {
      String html = "<html><body>";
      html += "<h1>ESP32-H4CK Vulnerable Lab</h1>";
      html += "<p>Welcome to the intentionally vulnerable web application training platform.</p>";
      html += "<h2>Available Endpoints:</h2>";
      html += "<ul>";
      html += "<li><a href='/login'>/login</a> - Login page</li>";
      html += "<li><a href='/admin'>/admin</a> - Admin panel (requires auth)</li>";
      html += "<li><a href='/api/info'>/api/info</a> - System information</li>";
      html += "<li><a href='/api/users'>/api/users</a> - User list (requires auth)</li>";
      html += "<li><a href='/shell.html'>/shell.html</a> - WebSocket shell</li>";
      html += "<li>Telnet: " + getLocalIP() + ":23</li>";
      html += "</ul>";
      html += "<h3>Warning:</h3>";
      html += "<p style='color:red;'>This system contains intentional vulnerabilities for educational purposes.</p>";
      html += "</body></html>";
      request->send(200, "text/html", html);
    }
  });
  
  // Login page
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    addCORSHeaders(request);
    if (fileExists("/login.html")) {
      request->send(LittleFS, "/login.html", "text/html");
    } else {
      String html = "<html><body>";
      html += "<h1>Login</h1>";
      html += "<form method='POST' action='/api/login'>";
      html += "Username: <input type='text' name='username'><br>";
      html += "Password: <input type='password' name='password'><br>";
      html += "<input type='submit' value='Login'>";
      html += "</form>";
      html += "<p>Default credentials: admin/admin, guest/guest</p>";
      html += "</body></html>";
      request->send(200, "text/html", html);
    }
  });
  
  // Admin panel (requires authentication)
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
    addCORSHeaders(request);
    if (!isAuthenticated(request)) {
      request->redirect("/login");
      return;
    }
    
    if (fileExists("/admin.html")) {
      request->send(LittleFS, "/admin.html", "text/html");
    } else {
      String html = "<html><body>";
      html += "<h1>Admin Panel</h1>";
      html += "<p>Welcome to the admin dashboard.</p>";
      html += "<h2>System Status:</h2>";
      html += "<pre>" + String(ESP.getFreeHeap()) + " bytes free heap</pre>";
      html += "<a href='/api/logout'>Logout</a>";
      html += "</body></html>";
      request->send(200, "text/html", html);
    }
  });
  
  // Debug endpoint (intentionally exposed - vulnerability)
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    addCORSHeaders(request);
    if (!ENABLE_VULNERABILITIES) {
      request->send(403, "text/plain", "Forbidden");
      return;
    }
    
    String debug = "=== DEBUG INFO ===\n";
    debug += "Free Heap: " + String(ESP.getFreeHeap()) + "\n";
    debug += "WiFi SSID: " + WIFI_SSID + "\n";
    debug += "WiFi Password: " + WIFI_PASSWORD + "\n";  // Intentional info disclosure
    debug += "JWT Secret: " + JWT_SECRET + "\n";  // Intentional info disclosure
    debug += "Active Sessions: " + String(activeSessions.size()) + "\n";
    debug += "Total Requests: " + String(totalRequests) + "\n";
    debug += "Uptime: " + String(millis() / 1000) + " seconds\n";
    request->send(200, "text/plain", debug);
  });
  
  Serial.println("[WEBSERVER] Routes configured");
}

void serveStaticFiles() {
  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  
  // Serve common file types
  server.serveStatic("/css/", LittleFS, "/css/");
  server.serveStatic("/js/", LittleFS, "/js/");
  server.serveStatic("/images/", LittleFS, "/images/");
  
  Serial.println("[WEBSERVER] Static file serving enabled");
}

void setupSSL() {
  // SSL setup would go here
  // Requires certificate and key files in data folder
  Serial.println("[WEBSERVER] SSL/TLS not implemented (requires certificates)");
}

void handleNotFound(AsyncWebServerRequest *request) {
  totalRequests++;
  addCORSHeaders(request);
  
  String message = "404 - Not Found\n\n";
  message += "URI: " + request->url() + "\n";
  message += "Method: " + String(request->method()) + "\n";
  message += "Arguments: " + String(request->args()) + "\n";
  
  for (uint8_t i = 0; i < request->args(); i++) {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }
  
  request->send(404, "text/plain", message);
  
  logDebug("404 Not Found: " + request->url());
}

void addCORSHeaders(AsyncWebServerRequest *request) {
  // Add CORS headers (intentionally permissive for vulnerability)
  AsyncWebServerResponse *response = request->beginResponse(200);
  response->addHeader("Access-Control-Allow-Origin", "*");  // Intentionally permissive
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  response->addHeader("Access-Control-Allow-Credentials", "true");
}
