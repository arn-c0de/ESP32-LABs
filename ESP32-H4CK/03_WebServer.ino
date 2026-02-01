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
    // Connection limiting
    if (totalRequests > 1000 && ESP.getFreeHeap() < 20000) {
      request->send(503, "text/plain", "Service Temporarily Unavailable - Low Memory");
      return;
    }
    Serial.printf("[HTTP] GET / from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    if (fileExists("/index.html")) {
      request->send(LittleFS, "/index.html", "text/html");
    } else {
      String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
      html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
      html += "<title>SecureNet Solutions - IoT Device Management</title>";
      html += "<style>";
      html += "body{font-family:Arial,sans-serif;margin:0;padding:0;background:#f4f4f4;color:#333}";
      html += "header{background:#0066cc;color:#fff;padding:20px;text-align:center}";
      html += "nav{background:#004d99;padding:10px;text-align:center}";
      html += "nav a{color:#fff;margin:0 15px;text-decoration:none;font-weight:bold}";
      html += ".container{max-width:1200px;margin:20px auto;padding:20px;background:#fff;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
      html += "h1{margin:0;font-size:2em}h2{color:#0066cc;border-bottom:2px solid #0066cc;padding-bottom:10px}";
      html += ".features{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin:20px 0}";
      html += ".feature{padding:20px;background:#f9f9f9;border-left:4px solid #0066cc;border-radius:4px}";
      html += "footer{background:#333;color:#fff;text-align:center;padding:15px;margin-top:40px}";
      html += ".cta{background:#0066cc;color:#fff;padding:12px 30px;border:none;border-radius:5px;cursor:pointer;font-size:1em;margin:10px 5px}";
      html += ".cta:hover{background:#004d99}";
      html += "</style></head><body>";
      html += "<header><h1>SecureNet Solutions</h1><p>Enterprise IoT Device Management Platform</p></header>";
      html += "<nav><a href='/'>Home</a><a href='/about'>About</a><a href='/products'>Products</a><a href='/support'>Support</a><a href='/login'>Login</a></nav>";
      html += "<div class='container'>";
      html += "<h2>Welcome to SecureNet Solutions</h2>";
      html += "<p>Leading provider of industrial IoT device management and monitoring solutions. Our ESP32-based platform offers real-time control, secure remote access, and comprehensive analytics for your connected devices.</p>";
      html += "<div class='features'>";
      html += "<div class='feature'><h3>🔒 Secure Access</h3><p>Industry-standard encryption and authentication protocols protect your devices.</p></div>";
      html += "<div class='feature'><h3>📊 Real-time Monitoring</h3><p>Monitor device status, performance metrics, and diagnostics in real-time.</p></div>";
      html += "<div class='feature'><h3>🌐 Remote Management</h3><p>Control and configure your devices from anywhere with our web-based dashboard.</p></div>";
      html += "<div class='feature'><h3>⚡ High Performance</h3><p>Optimized for low-latency operations and efficient resource utilization.</p></div>";
      html += "</div>";
      html += "<h2>Get Started</h2>";
      html += "<p>Access your device management dashboard by logging in with your credentials.</p>";
      html += "<button class='cta' onclick='location.href=\"/login\"'>Access Dashboard</button>";
      html += "<button class='cta' onclick='location.href=\"/support\"'>Contact Support</button>";
      html += "</div>";
      html += "<footer><p>&copy; 2026 SecureNet Solutions. All rights reserved. | <a href='/privacy' style='color:#fff'>Privacy Policy</a> | <a href='/terms' style='color:#fff'>Terms of Service</a></p></footer>";
      html += "</body></html>";
      request->send(200, "text/html", html);
    }
  });
  
  // Login page
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] GET /login from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    if (fileExists("/login.html")) {
      request->send(LittleFS, "/login.html", "text/html");
    } else {
      String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
      html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
      html += "<title>Login - SecureNet Solutions</title>";
      html += "<style>";
      html += "body{font-family:Arial,sans-serif;margin:0;padding:0;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;align-items:center;justify-content:center}";
      html += ".login-box{background:#fff;padding:40px;border-radius:10px;box-shadow:0 10px 25px rgba(0,0,0,0.2);width:100%;max-width:400px}";
      html += ".logo{text-align:center;margin-bottom:30px;color:#0066cc;font-size:1.8em;font-weight:bold}";
      html += "h2{text-align:center;color:#333;margin-bottom:30px}";
      html += ".form-group{margin-bottom:20px}";
      html += "label{display:block;margin-bottom:5px;color:#555;font-weight:bold}";
      html += "input[type=text],input[type=password]{width:100%;padding:12px;border:1px solid #ddd;border-radius:5px;font-size:1em;box-sizing:border-box}";
      html += "input[type=text]:focus,input[type=password]:focus{outline:none;border-color:#0066cc}";
      html += ".btn{width:100%;background:#0066cc;color:#fff;padding:12px;border:none;border-radius:5px;font-size:1.1em;cursor:pointer;font-weight:bold}";
      html += ".btn:hover{background:#004d99}";
      html += ".footer-text{text-align:center;margin-top:20px;color:#888;font-size:0.9em}";
      html += ".footer-text a{color:#0066cc;text-decoration:none}";
      html += "</style></head><body>";
      html += "<div class='login-box'>";
      html += "<div class='logo'>SecureNet</div>";
      html += "<h2>Device Dashboard Login</h2>";
      html += "<div id='msg' style='display:none;padding:10px;margin-bottom:15px;border-radius:5px'></div>";
      html += "<form id='lf'>";
      html += "<div class='form-group'><label>Username</label><input type='text' id='u' name='username' required placeholder='Enter your username'></div>";
      html += "<div class='form-group'><label>Password</label><input type='password' id='p' name='password' required placeholder='Enter your password'></div>";
      html += "<button type='submit' class='btn'>Sign In</button>";
      html += "</form>";
      html += "<div class='footer-text'>Forgot password? <a href='/support'>Contact Support</a></div>";
      html += "<div class='footer-text'><a href='/'>← Back to Home</a></div>";
      html += "</div>";
      html += "<script>";
      html += "document.getElementById('lf').onsubmit=function(e){e.preventDefault();";
      html += "var fd=new FormData();fd.append('username',document.getElementById('u').value);";
      html += "fd.append('password',document.getElementById('p').value);";
      html += "fetch('/api/login',{method:'POST',body:fd}).then(function(r){return r.json()}).then(function(d){";
      html += "if(d.success){localStorage.setItem('token',d.token);localStorage.setItem('username',d.username);";
      html += "localStorage.setItem('role',d.role);window.location.href=d.role==='admin'?'/admin':'/';}";
      html += "else{var m=document.getElementById('msg');m.style.display='block';m.style.background='#f8d7da';";
      html += "m.style.color='#721c24';m.textContent=d.error||'Login failed';}";
      html += "}).catch(function(e){var m=document.getElementById('msg');m.style.display='block';";
      html += "m.style.background='#f8d7da';m.style.color='#721c24';m.textContent='Error: '+e.message;});};";
      html += "</script></body></html>";
      request->send(200, "text/html", html);
    }
  });
  
  // Admin panel (requires authentication)
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] GET /admin from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    if (!isAuthenticated(request)) {
      Serial.printf("[HTTP] /admin access denied - not authenticated\n");
      request->redirect("/login");
      return;
    }
    Serial.printf("[HTTP] /admin access granted\n");
    
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
  
  // About page
  server.on("/about", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] GET /about from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>About - SecureNet Solutions</title>";
    html += "<style>body{font-family:Arial,sans-serif;margin:0;padding:0;background:#f4f4f4}header{background:#0066cc;color:#fff;padding:20px;text-align:center}nav{background:#004d99;padding:10px;text-align:center}nav a{color:#fff;margin:0 15px;text-decoration:none}.container{max-width:1200px;margin:20px auto;padding:20px;background:#fff;border-radius:8px}h2{color:#0066cc}.team{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin:20px 0}.member{padding:15px;background:#f9f9f9;border-radius:5px}footer{background:#333;color:#fff;text-align:center;padding:15px;margin-top:40px}</style></head><body>";
    html += "<header><h1>About SecureNet Solutions</h1></header>";
    html += "<nav><a href='/'>Home</a><a href='/about'>About</a><a href='/products'>Products</a><a href='/support'>Support</a><a href='/login'>Login</a></nav>";
    html += "<div class='container'><h2>Our Company</h2><p>Founded in 2020, SecureNet Solutions is a leading provider of enterprise-grade IoT device management solutions. We specialize in secure remote access and real-time monitoring for industrial applications.</p>";
    html += "<h2>Leadership Team</h2><div class='team'>";
    html += "<div class='member'><h3>Dr. Sarah Chen</h3><p><strong>CEO & Founder</strong></p><p>Email: s.chen@securenet-solutions.local</p></div>";
    html += "<div class='member'><h3>Michael Rodriguez</h3><p><strong>CTO</strong></p><p>Email: m.rodriguez@securenet-solutions.local</p></div>";
    html += "<div class='member'><h3>Administrator Account</h3><p><strong>System Admin</strong></p><p>Email: admin@securenet-solutions.local</p><p style='color:#888;font-size:0.85em'>Contact for technical access issues</p></div>";
    html += "<div class='member'><h3>John Smith</h3><p><strong>Head of Operations</strong></p><p>Email: operator@securenet-solutions.local</p></div>";
    html += "</div></div><footer><p>&copy; 2026 SecureNet Solutions</p></footer></body></html>";
    request->send(200, "text/html", html);
  });

  // Products page
  server.on("/products", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] GET /products from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Products - SecureNet Solutions</title>";
    html += "<style>body{font-family:Arial,sans-serif;margin:0;padding:0;background:#f4f4f4}header{background:#0066cc;color:#fff;padding:20px;text-align:center}nav{background:#004d99;padding:10px;text-align:center}nav a{color:#fff;margin:0 15px;text-decoration:none}.container{max-width:1200px;margin:20px auto;padding:20px;background:#fff;border-radius:8px}h2{color:#0066cc}.products{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin:20px 0}.product{padding:20px;background:#f9f9f9;border-left:4px solid #0066cc;border-radius:5px}footer{background:#333;color:#fff;text-align:center;padding:15px;margin-top:40px}</style></head><body>";
    html += "<header><h1>Our Products</h1></header>";
    html += "<nav><a href='/'>Home</a><a href='/about'>About</a><a href='/products'>Products</a><a href='/support'>Support</a><a href='/login'>Login</a></nav>";
    html += "<div class='container'><h2>Enterprise Solutions</h2><div class='products'>";
    html += "<div class='product'><h3>SecureNet IoT Gateway</h3><p>Industrial-grade gateway for secure device connectivity</p><ul><li>256-bit encryption</li><li>Real-time monitoring</li><li>Remote firmware updates</li></ul></div>";
    html += "<div class='product'><h3>Device Management Platform</h3><p>Cloud-based dashboard for fleet management</p><ul><li>Multi-device support</li><li>Analytics & reporting</li><li>API integration</li></ul></div>";
    html += "<div class='product'><h3>Security Suite</h3><p>Comprehensive security toolset</p><ul><li>Vulnerability scanning</li><li>Access control</li><li>Audit logging</li></ul></div>";
    html += "</div><p style='margin-top:30px;padding:15px;background:#fffbf0;border-left:4px solid #ffa500'>📥 Download product documentation: <a href='/docs/manual.pdf'>manual.pdf</a> | <a href='/api/info'>API Reference</a></p>";
    html += "</div><footer><p>&copy; 2026 SecureNet Solutions</p></footer></body></html>";
    request->send(200, "text/html", html);
  });

  // Support/Contact page
  server.on("/support", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] GET /support from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Support - SecureNet Solutions</title>";
    html += "<style>body{font-family:Arial,sans-serif;margin:0;padding:0;background:#f4f4f4}header{background:#0066cc;color:#fff;padding:20px;text-align:center}nav{background:#004d99;padding:10px;text-align:center}nav a{color:#fff;margin:0 15px;text-decoration:none}.container{max-width:1200px;margin:20px auto;padding:20px;background:#fff;border-radius:8px}h2{color:#0066cc}.contact-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin:20px 0}.contact-box{padding:20px;background:#f9f9f9;border-radius:5px}.note{background:#fff3cd;border-left:4px solid #ffa500;padding:15px;margin:20px 0}footer{background:#333;color:#fff;text-align:center;padding:15px;margin-top:40px}</style></head><body>";
    html += "<header><h1>Contact & Support</h1></header>";
    html += "<nav><a href='/'>Home</a><a href='/about'>About</a><a href='/products'>Products</a><a href='/support'>Support</a><a href='/login'>Login</a></nav>";
    html += "<div class='container'><h2>Get in Touch</h2>";
    html += "<div class='note'><strong>⚠️ Important:</strong> For urgent technical issues or password resets, contact our admin team directly.</div>";
    html += "<div class='contact-grid'>";
    html += "<div class='contact-box'><h3>Technical Support</h3><p><strong>Email:</strong> support@securenet-solutions.local</p><p><strong>Phone:</strong> +1 (555) 0123-456</p><p>Response time: 2-4 hours</p></div>";
    html += "<div class='contact-box'><h3>System Administration</h3><p><strong>Username:</strong> admin</p><p><strong>Email:</strong> admin@securenet-solutions.local</p><p style='color:#888;font-size:0.9em'>Note: Admin credentials are managed internally. Default enterprise password policy applies.</p></div>";
    html += "<div class='contact-box'><h3>Operations Team</h3><p><strong>Contact:</strong> John Smith</p><p><strong>Username:</strong> operator</p><p><strong>Email:</strong> operator@securenet-solutions.local</p></div>";
    html += "<div class='contact-box'><h3>Guest Access</h3><p><strong>Username:</strong> guest</p><p><strong>Purpose:</strong> Read-only demo access</p><p style='color:#888;font-size:0.9em'>For evaluation and testing purposes</p></div>";
    html += "</div>";
    html += "<h2>Quick Links</h2><ul><li><a href='/login'>Access Dashboard</a></li><li><a href='/api/info'>System Information</a></li><li><a href='/docs'>Documentation</a></li></ul>";
    html += "<p style='color:#888;margin-top:30px'><!-- Internal Note: Test users (admin/admin, guest/guest, operator/operator123) for demo --></p>";
    html += "</div><footer><p>&copy; 2026 SecureNet Solutions</p></footer></body></html>";
    request->send(200, "text/html", html);
  });

  // Privacy page
  server.on("/privacy", HTTP_GET, [](AsyncWebServerRequest *request) {
    totalRequests++;
    String html = "<!DOCTYPE html><html><head><title>Privacy Policy</title><style>body{font-family:Arial,sans-serif;max-width:800px;margin:40px auto;padding:20px;line-height:1.6}h1{color:#0066cc}</style></head><body>";
    html += "<h1>Privacy Policy</h1><p>Last updated: February 2026</p><p>SecureNet Solutions respects your privacy. This policy outlines how we collect and use information...</p>";
    html += "<p><a href='/'>← Back to Home</a></p></body></html>";
    request->send(200, "text/html", html);
  });

  // Terms page
  server.on("/terms", HTTP_GET, [](AsyncWebServerRequest *request) {
    totalRequests++;
    String html = "<!DOCTYPE html><html><head><title>Terms of Service</title><style>body{font-family:Arial,sans-serif;max-width:800px;margin:40px auto;padding:20px;line-height:1.6}h1{color:#0066cc}</style></head><body>";
    html += "<h1>Terms of Service</h1><p>By using SecureNet Solutions services, you agree to the following terms...</p>";
    html += "<p><a href='/'>← Back to Home</a></p></body></html>";
    request->send(200, "text/html", html);
  });

  // Version control discovery - .git/config (intentional exposure)
  server.on("/.git/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] ⚠️  .git/config access from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    if (!ENABLE_VULNERABILITIES) {
      request->send(404, "text/plain", "Not Found");
      return;
    }
    String gitConfig = "[core]\n\trepositoryformatversion = 0\n\tfilemode = false\n";
    gitConfig += "\tbare = false\n\tlogallrefupdates = true\n";
    gitConfig += "[remote \"origin\"]\n\turl = git@github.com:securenet/esp32-device-manager.git\n";
    gitConfig += "\tfetch = +refs/heads/*:refs/remotes/origin/*\n";
    gitConfig += "[branch \"main\"]\n\tremote = origin\n\tmerge = refs/heads/main\n";
    gitConfig += "[user]\n\tname = admin\n\temail = admin@securenet-solutions.local\n";
    request->send(200, "text/plain", gitConfig);
  });

  // .env file exposure (critical vulnerability)
  server.on("/.env", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] ⚠️  CRITICAL: .env file access from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    if (!ENABLE_VULNERABILITIES) {
      request->send(404, "text/plain", "Not Found");
      return;
    }
    String envContent = "# SecureNet ESP32 Configuration\n";
    envContent += "WIFI_SSID=" + WIFI_SSID_STR + "\n";
    envContent += "WIFI_PASSWORD=" + WIFI_PASSWORD_STR + "\n";
    envContent += "JWT_SECRET=" + JWT_SECRET_STR + "\n";
    envContent += "DATABASE_PASSWORD=admin123\n";
    envContent += "DEBUG_TOKEN=dbg_" + JWT_SECRET_STR + "\n";
    envContent += "ADMIN_EMAIL=admin@securenet-solutions.local\n";
    envContent += "TELNET_PORT=23\n";
    envContent += "API_KEY=sk-securenet-2024-prod-key-xyz\n";
    request->send(200, "text/plain", envContent);
  });

  // Backup directory listing
  server.on("/backup", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] /backup directory access from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    String html = "<!DOCTYPE html><html><head><title>Backup Directory</title>";
    html += "<style>body{font-family:monospace;padding:20px;background:#f4f4f4}h1{color:#333}ul{list-style:none;padding:0}li{padding:8px;background:#fff;margin:5px 0;border-left:4px solid #0066cc}a{text-decoration:none;color:#0066cc}</style></head><body>";
    html += "<h1>Index of /backup</h1><hr><ul>";
    html += "<li>📁 <a href='/backup/'>../</a></li>";
    html += "<li>📄 <a href='/backup/database_backup_2024_11_15.sql'>database_backup_2024_11_15.sql</a> (45.2 KB)</li>";
    html += "<li>📄 <a href='/backup/config_old.json'>config_old.json</a> (3.1 KB)</li>";
    html += "<li>📄 <a href='/backup/users_export.csv'>users_export.csv</a> (12.8 KB)</li>";
    html += "<li>🔐 <a href='/backup/private_keys.zip'>private_keys.zip</a> (8.4 KB)</li>";
    html += "<li>📄 <a href='/backup/.env.backup'>.env.backup</a> (892 B)</li>";
    html += "</ul><hr><p style='color:#888'>Last modified: 2024-11-15 14:23:17</p></body></html>";
    request->send(200, "text/html", html);
  });

  // Backup file downloads (simulated)
  server.on("/backup/database_backup_2024_11_15.sql", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] ⚠️  Database backup download from %s\n", request->client()->remoteIP().toString().c_str());
    String sql = "-- SecureNet Database Backup\n-- Date: 2024-11-15\n\n";
    sql += "INSERT INTO users VALUES (1, 'admin', 'admin', 'admin');\n";
    sql += "INSERT INTO users VALUES (2, 'guest', 'guest', 'guest');\n";
    sql += "INSERT INTO users VALUES (3, 'operator', 'operator123', 'guest');\n";
    sql += "INSERT INTO api_keys VALUES ('sk-securenet-2024-prod-key-xyz', 'admin', '2024-01-01');\n";
    request->send(200, "text/plain", sql);
  });

  server.on("/backup/.env.backup", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] .env.backup download from %s\n", request->client()->remoteIP().toString().c_str());
    request->send(200, "text/plain", "JWT_SECRET=old_secret_key_456\nADMIN_PASS=admin\n");
  });

  // robots.txt with hints
  server.on("/robots.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    String robots = "User-agent: *\n";
    robots += "Disallow: /admin\n";
    robots += "Disallow: /backup\n";
    robots += "Disallow: /.git\n";
    robots += "Disallow: /.env\n";
    robots += "Disallow: /debug\n";
    robots += "Disallow: /api/config\n";
    robots += "# Internal note: Check /api/endpoints for full list\n";
    request->send(200, "text/plain", robots);
  });

  // Debug endpoint (intentionally exposed - vulnerability)
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] GET /debug from %s\n", request->client()->remoteIP().toString().c_str());
    totalRequests++;
    addCORSHeaders(request);
    if (!ENABLE_VULNERABILITIES) {
      request->send(403, "text/plain", "Forbidden");
      return;
    }
    
    String debug = "=== DEBUG INFO ===\n";
    debug += "Free Heap: " + String(ESP.getFreeHeap()) + "\n";
    debug += "WiFi SSID: " + WIFI_SSID_STR + "\n";
    debug += "WiFi Password: " + WIFI_PASSWORD_STR + "\n";  // Intentional info disclosure
    debug += "JWT Secret: " + JWT_SECRET_STR + "\n";  // Intentional info disclosure
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
  
  // Rate limit 404 responses to prevent DoS from gobuster/dirb
  static unsigned long last404Time = 0;
  static int count404 = 0;
  unsigned long now = millis();
  
  if (now - last404Time < 1000) {
    count404++;
    if (count404 > 50) { // Max 50 requests per second
      request->send(429, "text/plain", "Too Many Requests");
      return;
    }
  } else {
    count404 = 0;
    last404Time = now;
  }
  
  // Simplified response to reduce memory usage
  String message = "404 - Not Found\n";
  message += "URI: " + request->url() + "\n";
  
  // Add vulnerable HTTP headers with version information
  AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", message);
  response->addHeader("Server", "ESP32-WebServer/1.0.2 (ESP-IDF v4.4.5)");
  response->addHeader("X-Powered-By", "Arduino/ESP32, LittleFS");
  response->addHeader("X-Framework", "ESPAsyncWebServer/3.9.6");
  response->addHeader("X-Device-Model", ESP.getChipModel());
  response->addHeader("X-Firmware-Version", "1.0.0-beta");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
  
  // Only log every 10th 404 to reduce FS writes
  if (count404 % 10 == 0) {
    logDebug("404 Not Found: " + request->url());
  }
}

void addCORSHeaders(AsyncWebServerRequest *request) {
  // This function is deprecated - CORS headers are now added directly in route handlers
  // Keeping for backward compatibility but it does nothing to prevent memory leaks
  // Original implementation created orphaned response objects
}
