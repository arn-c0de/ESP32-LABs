/*
 * Vulnerability Module - SCADA-specific exploit endpoints
 * 
 * Contains intentional vulnerabilities for security training.
 * WARNING: Do not enable in production systems!
 */

void setupVulnerableRoutes() {
  if (!ENABLE_VULNERABILITIES) {
    Serial.println("[VULN] Vulnerabilities DISABLED - secure mode");
    return;
  }
  
  Serial.println("[VULN] Setting up vulnerable endpoints for training...");
  
  // VULN: IDOR - Sensor data access without authorization
  // Already implemented in 06_API_SCADA.ino /api/sensors/{id}/readings
  
  // VULN: Command Injection - Actuator control
  // Already implemented in 08_Actuators.ino executeActuatorCommand()
  
  // VULN: Race Condition - Actuator state
  // Already implemented in 08_Actuators.ino (no mutex on state changes)
  
  // VULN: Weak Authentication - Visible default credentials
  server.on("/vuln/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<h1>SCADA Vulnerabilities Info</h1>";
    html += "<p>This system has intentional vulnerabilities for training purposes:</p>";
    html += "<ul>";
    html += "<li><b>IDOR:</b> Sensor readings accessible without proper authorization</li>";
    html += "<li><b>Command Injection:</b> Actuator commands not properly sanitized</li>";
    html += "<li><b>Race Condition:</b> Actuator state changes without locking</li>";
html += "<li><b>Weak Auth:</b> Default credentials (admin/admin, operator/operator123)</li>";
    html += "<li><b>Info Disclosure:</b> System info leaks configuration details</li>";
    html += "</ul>";
    html += "<p>Default users:</p><ul>";
    html += "<li>admin / admin (role: admin)</li>";
    html += "<li>operator / operator123 (role: operator)</li>";
    html += "<li>maint / maint456 (role: maintenance)</li>";
    html += "<li>view / view789 (role: viewer)</li>";
    html += "</ul>";
    request->send(200, "text/html", html);
  });
  
  // VULN: Debug endpoint exposing system state
  server.on("/vuln/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["total_requests"] = totalRequests;
    doc["active_connections"] = activeConnections;
    doc["failed_logins"] = failedLoginAttempts;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["uptime_ms"] = millis();
    
    JsonArray sessionsArr = doc["active_sessions"].to<JsonArray>();
    for (auto& session : activeSessions) {
      JsonObject sessObj = sessionsArr.add<JsonObject>();
      sessObj["token"] = session.first;
      sessObj["username"] = session.second.username;
      sessObj["role"] = session.second.role;
      sessObj["ip"] = session.second.ipAddress;
    }
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  Serial.printf("[VULN] Vulnerable routes enabled - LAB MODE: %s\n", LAB_MODE_STR.c_str());
}
