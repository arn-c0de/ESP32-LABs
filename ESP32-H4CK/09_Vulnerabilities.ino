/*
 * Vulnerabilities Module
 * 
 * Implements intentional security vulnerabilities for educational purposes.
 * Each vulnerability is documented and can be toggled via feature flags.
 * 
 * OWASP TOP 10 Coverage:
 * - A01: Broken Access Control
 * - A02: Cryptographic Failures
 * - A03: Injection (SQL, Command, XSS)
 * - A04: Insecure Design
 * - A05: Security Misconfiguration
 * - A07: Identification and Authentication Failures
 * - A08: Software and Data Integrity Failures
 * - A09: Security Logging and Monitoring Failures
 */

void setupVulnerableEndpoints() {
  // SQL Injection vulnerable endpoint
  server.on("/vuln/search", HTTP_GET, handleSQLInjection);
  
  // IDOR - User profile access (new endpoint)
  server.on("/vuln/user-profile", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("user_id")) {
      request->send(400, "application/json", "{\"error\":\"Missing user_id parameter\"}");
      return;
    }
    
    String userId = request->getParam("user_id")->value();
    Serial.printf("[VULN] ⚠️  IDOR: Accessing user_id=%s without authorization check\n", userId.c_str());
    
    // No authorization check - can access any user's profile
    DynamicJsonDocument doc(512);
    doc["user_id"] = userId;
    doc["username"] = "user_" + userId;
    doc["email"] = "user" + userId + "@securenet-solutions.local";
    doc["role"] = userId == "1" ? "admin" : "guest";
    doc["ssn"] = "123-45-" + userId;  // Sensitive data
    doc["api_key"] = "sk_user_" + userId + "_" + JWT_SECRET_STR;
    doc["hint"] = "Change user_id parameter to access other users' sensitive data";
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  // XSS vulnerable endpoint
  server.on("/vuln/comment", HTTP_POST, handleXSSVulnerability);
  server.on("/vuln/comments", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Display stored comments (vulnerable to stored XSS)
    String html = "<html><body><h1>Comments</h1>";
    html += "<form method='POST' action='/vuln/comment'>";
    html += "Comment: <input type='text' name='comment' size='50'><br>";
    html += "<input type='submit' value='Submit'>";
    html += "</form><hr>";
    
    String comments = readFile("/comments.txt");
    if (comments != "") {
      html += "<div>" + comments + "</div>";  // No sanitization!
    }
    
    html += "</body></html>";
    request->send(200, "text/html", html);
  });
  
  // Path Traversal vulnerable endpoint
  server.on("/vuln/download", HTTP_GET, handlePathTraversal);
  
  // Command Injection vulnerable endpoint
  server.on("/vuln/ping", HTTP_GET, handleCommandInjection);
  
  // CSRF vulnerable endpoint (no token validation)
  server.on("/vuln/transfer", HTTP_POST, handleCSRF);
  
  // Insecure Direct Object Reference (IDOR)
  server.on("/vuln/user", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("id")) {
      request->send(400, "text/plain", "Missing id parameter");
      return;
    }
    
    String userId = request->getParam("id")->value();
    
    // Intentional vulnerability: No authorization check
    // Any user can access any other user's data
    String userData = selectQuery(userId);
    if (userData != "") {
      sendJSONResponse(request, 200, userData);
    } else {
      request->send(404, "text/plain", "User not found");
    }
  });
  
  // Insecure Deserialization
  server.on("/vuln/deserialize", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("data", true)) {
      request->send(400, "text/plain", "Missing data parameter");
      return;
    }
    
    String data = request->getParam("data", true)->value();
    
    // Intentional vulnerability: Unsafe deserialization
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, data);
    
    if (error) {
      request->send(400, "text/plain", "Invalid JSON");
      return;
    }
    
    // Process deserialized data without validation
    String result = "Deserialized: " + String(doc["type"].as<String>());
    request->send(200, "text/plain", result);
  });
  
  // Information Disclosure
  server.on("/vuln/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Intentional vulnerability: Expose sensitive logs
    String logs = readFile(LOG_FILE_PATH);
    if (logs == "") {
      logs = "No logs available";
    }
    request->send(200, "text/plain", logs);
  });
  
  // Weak Session Management
  server.on("/vuln/session", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Create session with predictable ID
    String sessionId = String(millis());  // Predictable!
    
    DynamicJsonDocument doc(128);
    doc["session_id"] = sessionId;
    doc["message"] = "Session created with predictable ID";
    
    String output;
    serializeJson(doc, output);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Set-Cookie", "vuln_session=" + sessionId + "; Path=/");
    request->send(response);
  });
  
  // Mass Assignment
  server.on("/vuln/profile", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Intentional vulnerability: No field validation
    // Can modify any field including role, permissions, etc.
    
    DynamicJsonDocument doc(512);
    
    // Copy all parameters directly (mass assignment vulnerability)
    for (int i = 0; i < request->params(); i++) {
      const AsyncWebParameter* p = request->getParam(i);
      doc[p->name()] = p->value();
    }
    
    String output;
    serializeJson(doc, output);
    
    request->send(200, "application/json", output);
  });
  
  Serial.println("[VULNERABILITIES] All vulnerable endpoints configured");
  Serial.println("[VULNERABILITIES] ** FOR EDUCATIONAL USE ONLY **");
}

void handleSQLInjection(AsyncWebServerRequest *request) {
  totalRequests++;
  
  if (!VULN_SQL_INJECTION) {
    request->send(403, "text/plain", "Vulnerability disabled");
    return;
  }
  
  if (!request->hasParam("q")) {
    request->send(400, "text/plain", "Missing query parameter 'q'. Try: ?q=admin");
    return;
  }
  
  String query = request->getParam("q")->value();
  
  // Intentional vulnerability: Direct string concatenation
  String sql = "SELECT * FROM users WHERE username='" + query + "'";
  
  Serial.printf("[VULN] SQL Query: %s\n", sql.c_str());
  
  // Detect and log specific SQL injection patterns
  if (query.indexOf("' OR '1'='1") >= 0 || query.indexOf("' OR 1=1") >= 0) {
    Serial.printf("[VULN] ⚠️  Classic Boolean-based SQLi: ' OR '1'='1\n");
    Serial.printf("[HINT] Will bypass authentication and return ALL records\n");
  }
  else if (query.indexOf("UNION") >= 0 || query.indexOf("union") >= 0) {
    Serial.printf("[VULN] ⚠️  UNION-based SQLi detected\n");
    Serial.printf("[HINT] Can extract data from other tables/columns\n");
  }
  else if (query.indexOf("--") >= 0 || query.indexOf("#") >= 0) {
    Serial.printf("[VULN] ⚠️  SQL comment bypass: --  or #\n");
    Serial.printf("[HINT] Comment closes query, bypassing WHERE clause\n");
  }
  else if (query.indexOf("/*") >= 0) {
    Serial.printf("[VULN] ⚠️  Multi-line comment bypass detected\n");
  }
  else if (query.indexOf(";") >= 0 && (query.indexOf("DROP") >= 0 || query.indexOf("DELETE") >= 0 || query.indexOf("UPDATE") >= 0)) {
    Serial.printf("[VULN] ⚠️  Stacked query / destructive SQLi\n");
    Serial.printf("[HINT] Multiple SQL statements executed\n");
  }
  
  // Simulate SQL injection
  if (query.indexOf("'") >= 0 || query.indexOf("OR") >= 0 || query.indexOf("--") >= 0) {
    logError("[VULNERABILITY TRIGGERED] SQL Injection detected in query: " + query);
    
    // Return all users (simulating successful injection)
    String allUsers = getAllUsers();
    request->send(200, "application/json", allUsers);
  } else {
    // Normal query
    String result = getUserByUsername(query);
    if (result != "") {
      request->send(200, "application/json", result);
    } else {
      request->send(404, "text/plain", "User not found");
    }
  }
}

void handleXSSVulnerability(AsyncWebServerRequest *request) {
  totalRequests++;
  
  if (!VULN_XSS) {
    request->send(403, "text/plain", "Vulnerability disabled");
    return;
  }
  
  if (!request->hasParam("comment", true)) {
    request->send(400, "text/plain", "Missing comment parameter");
    return;
  }
  
  String comment = request->getParam("comment", true)->value();
  
  // Intentional vulnerability: No sanitization of user input
  logDebug("Storing comment: " + comment);
  
  if (comment.indexOf("<script>") >= 0 || comment.indexOf("javascript:") >= 0) {
    logError("[VULNERABILITY TRIGGERED] XSS payload detected: " + comment);
  }
  
  // Store comment without sanitization
  String existing = readFile("/comments.txt");
  String updated = existing + "<div>" + comment + "</div>\n";
  writeFile("/comments.txt", updated);
  
  request->redirect("/vuln/comments");
}

void handlePathTraversal(AsyncWebServerRequest *request) {
  totalRequests++;
  
  if (!VULN_PATH_TRAVERSAL) {
    request->send(403, "text/plain", "Vulnerability disabled");
    return;
  }
  
  if (!request->hasParam("file")) {
    request->send(400, "text/plain", "Missing file parameter. Try: ?file=config.txt or ?file=../../.env");
    return;
  }
  
  String filename = request->getParam("file")->value();
  
  Serial.printf("[VULN] Path Traversal: %s\n", filename.c_str());
  
  // Detect traversal patterns
  if (filename.indexOf("..") >= 0) {
    Serial.printf("[VULN] ⚠️  Directory traversal: ../ pattern detected\n");
    Serial.printf("[HINT] Accessing parent directories\n");
  }
  
  // Simulate /etc/passwd access
  if (filename.indexOf("/etc/passwd") >= 0 || filename.indexOf("passwd") >= 0) {
    Serial.printf("[VULN] ⚠️  System file access: /etc/passwd\n");
    String passwd = "root:x:0:0:root:/root:/bin/bash\n";
    passwd += "admin:x:1000:1000:SecureNet Admin:/home/admin:/bin/bash\n";
    passwd += "operator:x:1001:1001:Operator:/home/operator:/bin/bash\n";
    passwd += "guest:x:1002:1002:Guest:/home/guest:/bin/rbash\n";
    passwd += "www-data:x:33:33:www-data:/var/www:/usr/sbin/nologin\n";
    request->send(200, "text/plain", passwd);
    return;
  }
  
  // Simulate .env file access
  if (filename.indexOf(".env") >= 0) {
    Serial.printf("[VULN] ⚠️  Accessing .env file\n");
    String env = "WIFI_SSID=" + WIFI_SSID_STR + "\n";
    env += "WIFI_PASSWORD=" + WIFI_PASSWORD_STR + "\n";
    env += "JWT_SECRET=" + JWT_SECRET_STR + "\n";
    env += "DATABASE_URL=sqlite:///data/app.db\n";
    env += "API_KEY=sk-securenet-prod-key-xyz\n";
    request->send(200, "text/plain", env);
    return;
  }
  
  // Intentional vulnerability: No path validation
  logDebug("Downloading file: " + filename);
  
  if (filename.indexOf("..") >= 0) {
    logError("[VULNERABILITY TRIGGERED] Path traversal attempt: " + filename);
  }
  
  String content = readFile(filename);
  if (content != "") {
    request->send(200, "text/plain", content);
  } else {
    request->send(404, "text/plain", "File not found. Try: ../../../.env or ../../data/config.json");
  }
}

void handleCommandInjection(AsyncWebServerRequest *request) {
  totalRequests++;
  
  if (!VULN_COMMAND_INJECTION) {
    request->send(403, "text/plain", "Vulnerability disabled");
    return;
  }
  
  if (!request->hasParam("host")) {
    request->send(400, "text/plain", "Missing host parameter. Try: ?host=8.8.8.8 or ?host=8.8.8.8; whoami");
    return;
  }
  
  String host = request->getParam("host")->value();
  
  // Intentional vulnerability: Direct command execution
  String command = "ping -c 1 " + host;
  
  Serial.printf("[VULN] Command Injection: %s\n", command.c_str());
  
  // Detect injection patterns
  if (host.indexOf(";") >= 0) {
    Serial.printf("[VULN] ⚠️  Command chaining: semicolon (;) detected\n");
    Serial.printf("[HINT] cmd1 ; cmd2 - executes both commands\n");
  }
  else if (host.indexOf("&&") >= 0) {
    Serial.printf("[VULN] ⚠️  Logic operator: && detected\n");
    Serial.printf("[HINT] cmd1 && cmd2 - cmd2 runs if cmd1 succeeds\n");
  }
  else if (host.indexOf("||") >= 0) {
    Serial.printf("[VULN] ⚠️  Logic operator: || detected\n");
    Serial.printf("[HINT] cmd1 || cmd2 - cmd2 runs if cmd1 fails\n");
  }
  else if (host.indexOf("|") >= 0) {
    Serial.printf("[VULN] ⚠️  Pipe operator: | detected\n");
    Serial.printf("[HINT] cmd1 | cmd2 - pipes cmd1 output to cmd2\n");
  }
  else if (host.indexOf("`") >= 0 || host.indexOf("$(") >= 0) {
    Serial.printf("[VULN] ⚠️  Command substitution detected\n");
    Serial.printf("[HINT] Subshell execution via `cmd` or $(cmd)\n");
  }
  
  if (host.indexOf(";") >= 0 || host.indexOf("&") >= 0 || host.indexOf("|") >= 0) {
    logError("[VULNERABILITY TRIGGERED] Command injection detected: " + host);
  }
  
  String output = "[SIMULATED] Executing: " + command + "\n";
  output += "PING " + host + " (192.168.1.1): 56 data bytes\n";
  output += "64 bytes from 192.168.1.1: icmp_seq=0 ttl=64 time=1.234 ms\n";
  output += "\n--- " + host + " ping statistics ---\n";
  output += "1 packets transmitted, 1 packets received, 0.0% packet loss\n";
  
  if (host.indexOf(";") >= 0 || host.indexOf("&&") >= 0 || host.indexOf("||") >= 0 || host.indexOf("|") >= 0) {
    output += "\n[VULNERABILITY EXPLOITED]\n";
    output += "root:x:0:0:root:/root:/bin/bash\n";
    output += "admin:x:1000:1000:admin:/home/admin:/bin/bash\n";
    output += "Current user: www-data\n";
    output += "\nHint: Try '8.8.8.8; cat /etc/passwd' or '8.8.8.8 && whoami'\n";
  }
  
  request->send(200, "text/plain", output);
}

void handleCSRF(AsyncWebServerRequest *request) {
  totalRequests++;
  
  if (!VULN_CSRF) {
    request->send(403, "text/plain", "Vulnerability disabled");
    return;
  }
  
  // Intentional vulnerability: No CSRF token validation
  String from = request->hasParam("from", true) ? request->getParam("from", true)->value() : "";
  String to = request->hasParam("to", true) ? request->getParam("to", true)->value() : "";
  String amount = request->hasParam("amount", true) ? request->getParam("amount", true)->value() : "";
  
  logDebug("Transfer request: " + from + " -> " + to + " : " + amount);
  logError("[VULNERABILITY] No CSRF protection! Request accepted without token validation.");
  
  DynamicJsonDocument doc(256);
  doc["success"] = true;
  doc["message"] = "Transfer completed (NO CSRF PROTECTION!)";
  doc["from"] = from;
  doc["to"] = to;
  doc["amount"] = amount;
  
  String output;
  serializeJson(doc, output);
  
  request->send(200, "application/json", output);
}
