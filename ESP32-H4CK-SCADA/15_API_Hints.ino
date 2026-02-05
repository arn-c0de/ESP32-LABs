// ============================================================
// 15_API_Hints.ino — Progressive Hint Disclosure
// ============================================================

struct HintEntry {
  const char* endpoint;
  const char* pathName;
  int         level;
  int         unlockAfterMin;
  const char* text;
};

// Hint database (static)
const HintEntry hintDatabase[] = {
  // IDOR hints
  {"GET_/api/sensor/reading", "IDOR", 1, 0,
    "Sensors have IDs like SENSOR-L1-01, SENSOR-L2-03... try different lines."},
  {"GET_/api/sensor/reading", "IDOR", 2, 5,
    "IDOR = Insecure Direct Object Reference. The server doesn't verify which line you belong to."},
  {"GET_/api/sensor/reading", "IDOR", 3, 15,
    "Try: GET /api/sensor/reading?sensor_id=SENSOR-L2-03&limit=50 — look for FLAG in the response."},

  {"GET_/api/alarms/history", "IDOR", 1, 0,
    "Alarm history accepts a line parameter. Try different line numbers."},
  {"GET_/api/alarms/history", "IDOR", 2, 5,
    "Can you see alarms from lines you don't operate?"},
  {"GET_/api/alarms/history", "IDOR", 3, 15,
    "Try: GET /api/alarms/history?line=2&limit=50"},

  // Injection hints
  {"POST_/api/actuators/control", "INJECTION", 1, 0,
    "Actuator commands accept JSON with params. What if params contain unexpected values?"},
  {"POST_/api/actuators/control", "INJECTION", 2, 5,
    "The 'speed' parameter is passed to a simulated command parser. Try shell-like syntax."},
  {"POST_/api/actuators/control", "INJECTION", 3, 15,
    "Try: {\"id\":\"MOTOR-L1-01\",\"cmd\":\"set\",\"params\":{\"speed\":\"50;simulate:cat /data/db/maintenance.json\"}}"},

  // Race Condition hints
  {"POST_/api/test/race", "RACE", 1, 0,
    "Some operations aren't thread-safe. What happens with rapid concurrent commands?"},
  {"POST_/api/test/race", "RACE", 2, 5,
    "Try sending many commands to the same actuator simultaneously."},
  {"POST_/api/test/race", "RACE", 3, 15,
    "Try: POST /api/test/race?actuator=MOTOR-L2-01&count=10 — check the corrupted_field."},

  // Physics hints
  {"physics_analysis", "PHYSICS", 1, 0,
    "Some sensors should correlate with each other. Motor speed should affect vibration."},
  {"physics_analysis", "PHYSICS", 2, 5,
    "A sensor fault means a reading is stuck. Compare with correlated sensors."},
  {"physics_analysis", "PHYSICS", 3, 15,
    "Cross-reference: if motor runs but vibration is zero, that's a physics anomaly. Check maintenance logs."},

  // Forensics hints
  {"forensics_analysis", "FORENSICS", 1, 0,
    "Check /api/forensics/logs for combined log data with hidden clues."},
  {"forensics_analysis", "FORENSICS", 2, 5,
    "Defense events show what exploits others have tried. State import endpoint may be vulnerable."},
  {"forensics_analysis", "FORENSICS", 3, 15,
    "Try: POST /api/import/state with arbitrary JSON. Error messages leak information."},

  // Weak Auth hints
  {"weak_auth", "WEAK_AUTH", 1, 0,
    "Some systems have default credentials. Check maintenance logs for credential patterns."},
  {"weak_auth", "WEAK_AUTH", 2, 5,
    "Maintenance technicians sometimes write passwords in their work notes."},
  {"weak_auth", "WEAK_AUTH", 3, 15,
    "Try: GET /api/maintenance/logs after logging in as maintenance user. Credentials might be in the notes."},
};

const int HINT_COUNT = sizeof(hintDatabase) / sizeof(HintEntry);

void hintsInit() {
  Serial.println("[HINTS] Initializing hint system...");
  Serial.printf("[HINTS] %d hints available (level: %d)\n", HINT_COUNT, HINTS_LEVEL);
}

// ===== Get hint for endpoint =====
String getHint(const String& endpoint, int requestedLevel, const String& sessionId) {
  if (!HINTS_AVAILABLE) {
    return jsonError("Hints are disabled in current difficulty mode", 403);
  }

  if (requestedLevel > HINTS_LEVEL) {
    return jsonError("Hint level " + String(requestedLevel) +
      " not available (max: " + String(HINTS_LEVEL) + ")", 403);
  }

  // Track hint request
  PlayerState* player = getPlayer(sessionId);
  if (player) player->hintsRequested++;

  // Calculate time since session start
  int minutesElapsed = 0;
  if (player) {
    minutesElapsed = (millis() - player->startedAt) / 60000;
  }

  JsonDocument doc;
  doc["endpoint"]  = endpoint;
  doc["level"]     = requestedLevel;
  doc["available"] = true;

  JsonArray hints = doc["hints"].to<JsonArray>();
  bool found = false;

  for (int i = 0; i < HINT_COUNT; i++) {
    if (endpoint == hintDatabase[i].endpoint || endpoint.isEmpty()) {
      if (hintDatabase[i].level <= requestedLevel) {
        // Check time unlock
        if (minutesElapsed >= hintDatabase[i].unlockAfterMin || !PROGRESSIVE_DISCLOSURE) {
          JsonObject h = hints.add<JsonObject>();
          h["level"]     = hintDatabase[i].level;
          h["path"]      = hintDatabase[i].pathName;
          h["text"]      = hintDatabase[i].text;
          h["unlocked"]  = true;
          found = true;
        } else {
          JsonObject h = hints.add<JsonObject>();
          h["level"]     = hintDatabase[i].level;
          h["path"]      = hintDatabase[i].pathName;
          h["text"]      = "Unlocks in " +
            String(hintDatabase[i].unlockAfterMin - minutesElapsed) + " minutes";
          h["unlocked"]  = false;
        }
      }
    }
  }

  if (!found && hints.size() == 0) {
    doc["message"] = "No hints found for endpoint: " + endpoint;

    // List available endpoints
    JsonArray available = doc["available_endpoints"].to<JsonArray>();
    available.add("GET_/api/sensor/reading");
    available.add("GET_/api/alarms/history");
    available.add("POST_/api/actuators/control");
    available.add("POST_/api/test/race");
    available.add("physics_analysis");
    available.add("forensics_analysis");
    available.add("weak_auth");
  }

  String out;
  serializeJson(doc, out);
  return out;
}
