/*
 * Advanced Vulnerability Endpoints Module
 * 
 * Additional OWASP Top 10 and advanced vulnerability demonstrations
 * including SSRF, XXE, Race Conditions, File Upload, etc.
 */

void setupAdvancedVulnerabilityEndpoints() {
  
  // VERSION ENDPOINT
  server.on("/api/version", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[API] Version check from %s\n", request->client()->remoteIP().toString().c_str());
    
    DynamicJsonDocument doc(512);
    doc["firmware_version"] = LAB_VERSION;
    doc["build_date"] = BUILD_DATE;
    doc["codename"] = CODENAME;
    doc["device"] = "ESP32-D0WD-V3";
    doc["total_vulnerabilities"] = 28;
    doc["learning_modules"] = 12;
    doc["hint"] = "Enumerate with /api/endpoints";
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  // FILE UPLOAD VULNERABILITY (no validation)
  server.on("/api/upload", HTTP_POST, 
    [](AsyncWebServerRequest *request) {
      Serial.printf("[API] ⚠️  File upload completed from %s\n", request->client()->remoteIP().toString().c_str());
      request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"File uploaded\",\"hint\":\"No validation - upload shell.php!\"}");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (index == 0) {
        Serial.printf("[API] ⚠️  UNRESTRICTED FILE UPLOAD: %s\n", filename.c_str());
        
        if (filename.endsWith(".php") || filename.endsWith(".jsp") || 
            filename.endsWith(".asp") || filename.endsWith(".exe")) {
          Serial.printf("[VULN] ⚠️  DANGEROUS FILE: %s - RCE possible!\n", filename.c_str());
          Serial.printf("[HINT] No extension validation allows webshell upload\n");
        }
      }
    }
  );
  
  // SSRF - Server-Side Request Forgery
  server.on("/api/fetch", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("url")) {
      request->send(400, "application/json", "{\"error\":\"Missing url\",\"example\":\"?url=http://internal-api/admin\"}");
      return;
    }
    
    String url = request->getParam("url")->value();
    Serial.printf("[API] ⚠️  SSRF: Fetching %s\n", url.c_str());
    
    if (url.indexOf("127.0.0.1") >= 0 || url.indexOf("localhost") >= 0 || 
        url.indexOf("192.168.") >= 0 || url.indexOf("10.") >= 0) {
      Serial.printf("[VULN] ⚠️  SSRF to internal network!\n");
    }
    
    if (url.indexOf("file://") >= 0) {
      Serial.printf("[VULN] ⚠️  SSRF with file:// - local file disclosure!\n");
    }
    
    DynamicJsonDocument doc(768);
    doc["vulnerability"] = "SSRF";
    doc["requested_url"] = url;
    doc["response"] = "HTTP/1.1 200 OK\\n{\"admin\":\"http://192.168.1.100/admin\",\"db\":\"mongodb://localhost:27017\"}";
    doc["hint"] = "Try: ?url=file:///etc/passwd or ?url=http://localhost:6379";
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  // XXE - XML External Entity
  server.on("/api/xml-parse", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("xml", true)) {
      String hint = "{\"error\":\"Missing xml\",";
      hint += "\"example\":\"<?xml version='1.0'?><!DOCTYPE foo [<!ENTITY xxe SYSTEM 'file:///etc/passwd'>]><root>&xxe;</root>\"}";
      request->send(400, "application/json", hint);
      return;
    }
    
    String xml = request->getParam("xml", true)->value();
    Serial.printf("[API] ⚠️  XXE: Parsing XML\n");
    
    if (xml.indexOf("<!ENTITY") >= 0 && xml.indexOf("SYSTEM") >= 0) {
      Serial.printf("[VULN] ⚠️  XXE ATTACK - External entity detected!\n");
      
      DynamicJsonDocument doc(1024);
      doc["vulnerability"] = "XXE";
      doc["parsed"] = "root:x:0:0:root:/root:/bin/bash\\nadmin:x:1000:1000:admin";
      doc["exploit_success"] = true;
      doc["hint"] = "XXE enables SSRF: <!ENTITY xxe SYSTEM 'http://internal/admin'>";
      
      String output;
      serializeJson(doc, output);
      request->send(200, "application/json", output);
    } else {
      request->send(200, "application/json", "{\"status\":\"parsed\",\"hint\":\"Try <!ENTITY> declaration\"}");
    }
  });
  
  // RACE CONDITION - Now handled by 16_Wallet.ino with persistent balance
  // The /api/wallet/withdraw endpoint uses real user balances from LittleFS

  // SESSION FIXATION
  server.on("/api/auth/session-fixation", HTTP_GET, [](AsyncWebServerRequest *request) {
    String sessionId = "";
    
    if (request->hasParam("session_id")) {
      sessionId = request->getParam("session_id")->value();
      Serial.printf("[API] ⚠️  SESSION FIXATION: Attacker session=%s\n", sessionId.c_str());
    } else {
      sessionId = String(millis());
    }
    
    DynamicJsonDocument doc(256);
    doc["session_id"] = sessionId;
    doc["vuln"] = "Session Fixation";
    doc["hint"] = "Provide ?session_id=ATTACKER_ID before victim login";
    
    String output;
    serializeJson(doc, output);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/");
    request->send(response);
  });
  
  // HTTP PARAMETER POLLUTION
  server.on("/api/user/email", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("user_id")) {
      request->send(400, "application/json", "{\"error\":\"Missing user_id\"}");
      return;
    }
    
    int paramCount = 0;
    String firstId = "";
    String lastId = "";
    
    for (int i = 0; i < request->params(); i++) {
      const AsyncWebParameter* p = request->getParam(i);
      if (p->name() == "user_id") {
        if (paramCount == 0) firstId = p->value();
        lastId = p->value();
        paramCount++;
      }
    }
    
    if (paramCount > 1) {
      Serial.printf("[API] ⚠️  HPP: %d user_id params (first=%s, last=%s)\n", 
                    paramCount, firstId.c_str(), lastId.c_str());
    }
    
    DynamicJsonDocument doc(384);
    doc["user_id_first"] = firstId;
    doc["user_id_last"] = lastId;
    doc["param_count"] = paramCount;
    doc["email"] = "user" + lastId + "@example.com";
    
    if (paramCount > 1) {
      doc["vuln"] = "HTTP Parameter Pollution";
      doc["hint"] = "Try: ?user_id=1&user_id=999";
    }
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  // OPEN REDIRECT
  server.on("/api/redirect", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("url")) {
      request->send(400, "text/plain", "Missing url parameter");
      return;
    }
    
    String redirectUrl = request->getParam("url")->value();
    Serial.printf("[API] ⚠️  OPEN REDIRECT: %s\n", redirectUrl.c_str());
    
    if (redirectUrl.indexOf("http") == 0) {
      Serial.printf("[VULN] ⚠️  External redirect to %s\n", redirectUrl.c_str());
      Serial.printf("[HINT] Can redirect to phishing sites\n");
    }
    
    request->redirect(redirectUrl);
  });
  
  // CLICKJACKING TEST
  server.on("/api/frame-test", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[API] ⚠️  No X-Frame-Options header\n");
    
    String html = "<html><body>";
    html += "<h1>Clickjacking Vulnerable Page</h1>";
    html += "<p>This page has NO X-Frame-Options header</p>";
    html += "<p>It can be embedded in malicious iframes</p>";
    html += "<button onclick='alert(\"Action performed!\")'>Click Me</button>";
    html += "<p>HINT: Missing headers: X-Frame-Options, Content-Security-Policy frame-ancestors</p>";
    html += "</body></html>";
    
    request->send(200, "text/html", html);
  });
  
  // INSECURE DIRECT OBJECT REFERENCE ENHANCED
  server.on("/api/documents", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("doc_id")) {
      request->send(400, "application/json", "{\"error\":\"Missing doc_id\"}");
      return;
    }
    
    String docId = request->getParam("doc_id")->value();
    Serial.printf("[API] ⚠️  IDOR: Document %s accessed WITHOUT auth check\n", docId.c_str());
    
    DynamicJsonDocument doc(512);
    doc["doc_id"] = docId;
    doc["title"] = "Document " + docId;
    doc["content"] = "CONFIDENTIAL: Salary information for doc" + docId;
    doc["owner"] = "user_" + docId;
    doc["hint"] = "Enumerate documents by incrementing doc_id";
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  Serial.println("[API] Advanced vulnerability endpoints configured");
  Serial.printf("[API] Total endpoints: 12+ new learning modules\n");
}
