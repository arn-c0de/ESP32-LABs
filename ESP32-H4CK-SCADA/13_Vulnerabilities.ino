// ============================================================
// 13_Vulnerabilities.ino — 6 Gated Exploit Endpoints
// IDOR, Injection, Race, Physics, Forensics, Weak Auth
// ============================================================

// Vulnerability-specific routes (called from setupAPIRoutes)
void setupVulnRoutes() {
  // ===== VULN: Sensor Tamper (IDOR helper) =====
  server.on("/vuln/sensor-tamper", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!checkDefense(request)) return;

    if (!VULN_IDOR_SENSORS) {
      sendJson(request, 403, jsonError("Sensor tamper endpoint disabled", 403));
      return;
    }

    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];

    JsonDocument doc;
    if (deserializeJson(doc, body)) {
      sendJson(request, 400, jsonError("Invalid JSON", 400));
      return;
    }

    String sensorId = doc["sensor_id"] | "";
    float fakeValue = doc["value"] | 0.0f;

    // Inject fake reading
    SensorData* sd = getSensorById(sensorId);
    if (sd) {
      sd->value = fakeValue;
      sd->status = "tampered";

      JsonDocument result;
      result["success"]   = true;
      result["sensor_id"] = sensorId;
      result["injected_value"] = fakeValue;
      result["flag"]      = "FLAG{sensor_tamper_" + sensorId + "_injected}";
      result["vulnerability"] = "IDOR_SENSOR_TAMPER";
      String out;
      serializeJson(result, out);
      sendJson(request, 200, out);

      logDefenseEvent("SENSOR_TAMPER", getClientIP(request),
        "Tampered: " + sensorId + " = " + String(fakeValue));
    } else {
      sendJson(request, 404, jsonError("Sensor not found", 404));
    }
  });

  // ===== VULN: Debug endpoint (EASY mode only) =====
  server.on("/api/debug/state", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!EXPOSE_DEBUG_ENDPOINTS) {
      sendJson(request, 404, jsonError("Not found", 404));
      return;
    }

    JsonDocument doc;
    doc["sensors"]     = serialized(dbGetSensors());
    doc["actuators"]   = serialized(dbGetActuators());
    doc["alarms"]      = serialized(getActiveAlarms());
    doc["incidents"]   = serialized(getActiveIncidents());
    doc["config"]      = serialized(configGetJson());
    doc["defense"]     = serialized(getDefenseStatus());
    doc["maintenance"] = serialized(dbGetMaintenance());
    doc["_debug_flag"] = "FLAG{debug_endpoint_exposed}";

    String out;
    serializeJson(doc, out);
    sendJson(request, 200, out);
  });

  // ===== VULN: Insecure deserialization =====
  server.on("/api/import/state", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (!checkDefense(request)) return;

    if (!VULN_INSECURE_DESERIALIZATION) {
      sendJson(request, 403, jsonError("Import disabled", 403));
      return;
    }

    String body = "";
    for (size_t i = 0; i < len; i++) body += (char)data[i];

    // Intentionally vulnerable: deserialize arbitrary JSON into state
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);

    if (err) {
      // Leak error details (vulnerability)
      JsonDocument errDoc;
      errDoc["error"]   = true;
      errDoc["message"] = err.c_str();
      errDoc["input_size"] = (int)len;
      errDoc["flag"]    = "FLAG{insecure_deserialization_error_leak}";
      String out;
      serializeJson(errDoc, out);
      sendJson(request, 400, out);
      return;
    }

    // Accept the corrupted state
    JsonDocument result;
    result["success"] = true;
    result["message"] = "State imported (no validation)";
    result["keys_imported"] = doc.size();
    result["vulnerability"] = "INSECURE_DESERIALIZATION";
    result["flag"] = "FLAG{insecure_deser_state_corrupted}";

    String out;
    serializeJson(result, out);
    sendJson(request, 200, out);

    logDefenseEvent("INSECURE_DESER", getClientIP(request),
      "State import: " + String(doc.size()) + " keys");
  });

  // ===== VULN: Forensics endpoint =====
  server.on("/api/forensics/logs", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!checkDefense(request)) return;

    if (!ENABLE_EXPLOIT_FORENSICS) {
      sendJson(request, 403, jsonError("Forensics endpoint disabled", 403));
      return;
    }

    // Return combined logs with hidden clues
    JsonDocument doc;

    // Sensor log excerpts
    doc["sensor_log"] = serialized(sensorBuffer.toJsonArray(20));

    // Alarm log excerpts
    doc["alarm_log"] = serialized(alarmBuffer.toJsonArray(20));

    // Defense events
    doc["defense_log"] = serialized(defenseLogBuffer.toJsonArray(20));

    // Incident history
    doc["incident_log"] = serialized(incidentBuffer.toJsonArray(20));

    // Hidden forensic clues
    JsonArray clues = doc["_forensic_clues"].to<JsonArray>();
    clues.add("Check maintenance logs for credential patterns");
    clues.add("Cross-reference sensor faults with incident timestamps");
    clues.add("Defense events reveal exploitation attempts");
    clues.add("FLAG{forensics_log_analysis_" +
      sha256Hash(getISOTimestamp()).substring(0, 8) + "}");

    String out;
    serializeJson(doc, out);
    sendJson(request, 200, out);
  });

  // ===== VULN: Honeypot endpoints =====
  if (HONEYPOT_ENDPOINTS_ENABLED) {
    // Common attack targets that are actually traps
    server.on("/admin/shell", HTTP_GET, [](AsyncWebServerRequest* request) {
      logDefenseEvent("HONEYPOT", getClientIP(request), "Hit /admin/shell");
      sendJson(request, 200, "{\"message\":\"Access logged.\",\"flag\":\"FLAG{honeypot_admin_shell}\"}");
    });

    server.on("/api/v1/users", HTTP_GET, [](AsyncWebServerRequest* request) {
      logDefenseEvent("HONEYPOT", getClientIP(request), "Hit /api/v1/users");
      sendJson(request, 200, "{\"users\":[],\"flag\":\"FLAG{honeypot_user_enum}\"}");
    });

    server.on("/.env", HTTP_GET, [](AsyncWebServerRequest* request) {
      logDefenseEvent("HONEYPOT", getClientIP(request), "Hit /.env");
      sendJson(request, 200, "{\"flag\":\"FLAG{honeypot_env_file}\"}");
    });

    server.on("/backup.sql", HTTP_GET, [](AsyncWebServerRequest* request) {
      logDefenseEvent("HONEYPOT", getClientIP(request), "Hit /backup.sql");
      sendJson(request, 200, "{\"flag\":\"FLAG{honeypot_sql_backup}\"}");
    });
  }
}
