// ============================================================
// 05_Database.ino — LittleFS + JSON Ringbuffers + CRUD
// ============================================================

// ===== Ringbuffer for time-series data =====
struct RingBuffer {
  String*  entries;
  int      maxSize;
  int      head;
  int      count;

  void init(int size) {
    maxSize = size;
    entries = new String[size];
    head = 0;
    count = 0;
  }

  void push(const String& entry) {
    entries[head] = entry;
    head = (head + 1) % maxSize;
    if (count < maxSize) count++;
  }

  String get(int index) const {
    if (index >= count) return "";
    int pos = (head - count + index + maxSize) % maxSize;
    return entries[pos];
  }

  String getLatest() const {
    if (count == 0) return "";
    return entries[(head - 1 + maxSize) % maxSize];
  }

  String toJsonArray(int limit = 0) const {
    int n = (limit > 0 && limit < count) ? limit : count;
    String out = "[";
    int start = count - n;
    for (int i = start; i < count; i++) {
      if (i > start) out += ",";
      out += get(i);
    }
    out += "]";
    return out;
  }

  void clear() {
    head = 0;
    count = 0;
  }
};

// ===== Global ringbuffers =====
RingBuffer sensorBuffer;
RingBuffer alarmBuffer;
RingBuffer logBuffer;
RingBuffer incidentBuffer;
RingBuffer defenseLogBuffer;

// ===== Database state =====
bool dbInitialized = false;

// ===== File helpers =====
String dbReadFile(const char* path) {
  if (!LittleFS.exists(path)) return "";
  File f = LittleFS.open(path, "r");
  if (!f) return "";
  String content = f.readString();
  f.close();
  return content;
}

bool dbWriteFile(const char* path, const String& content) {
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  f.print(content);
  f.close();
  return true;
}

bool dbAppendFile(const char* path, const String& content) {
  File f = LittleFS.open(path, "a");
  if (!f) return false;
  f.println(content);
  f.close();
  return true;
}

bool dbFileExists(const char* path) {
  return LittleFS.exists(path);
}

// ===== Seed data creation =====
void dbCreateSeedData() {
  debugLog("DB", "Creating seed data...");

  // Equipment seed
  if (!dbFileExists("/db/equipment.json")) {
    JsonDocument doc;
    JsonArray lines = doc["lines"].to<JsonArray>();
    for (int l = 1; l <= NUM_LINES; l++) {
      JsonObject line = lines.add<JsonObject>();
      char lineId[8];
      snprintf(lineId, sizeof(lineId), "L%d", l);
      line["id"] = lineId;
      line["name"] = LINE_NAMES[l - 1];
      line["status"] = (l == 3) ? "stopped" : "running";
      line["efficiency"] = randomFloat(85.0f, 98.0f);
      line["uptime_hours"] = randomFloat(1.0f, 24.0f);

      JsonArray motors = line["motors"].to<JsonArray>();
      for (int m = 1; m <= 2; m++) {
        JsonObject motor = motors.add<JsonObject>();
        char mid[16];
        snprintf(mid, sizeof(mid), "MOTOR-L%d-%02d", l, m);
        motor["id"]     = mid;
        motor["type"]   = "motor";
        motor["status"] = (l == 3) ? "stopped" : "running";
        motor["speed"]  = (l == 3) ? 0 : (int)randomFloat(60.0f, 95.0f);
        motor["rpm"]    = (l == 3) ? 0 : (int)randomFloat(1200.0f, 1800.0f);
      }

      JsonArray valves = line["valves"].to<JsonArray>();
      for (int v = 1; v <= 2; v++) {
        JsonObject valve = valves.add<JsonObject>();
        char vid[16];
        snprintf(vid, sizeof(vid), "VALVE-L%d-%02d", l, v);
        valve["id"]    = vid;
        valve["type"]  = "valve";
        valve["state"] = "open";
      }

      JsonArray pumps = line["pumps"].to<JsonArray>();
      JsonObject pump = pumps.add<JsonObject>();
      char pid[16];
      snprintf(pid, sizeof(pid), "PUMP-L%d-01", l);
      pump["id"]     = pid;
      pump["type"]   = "pump";
      pump["status"] = (l == 3) ? "stopped" : "running";
      pump["flow"]   = (l == 3) ? 0 : (int)randomFloat(100.0f, 150.0f);
    }
    String out;
    serializeJson(doc, out);
    dbWriteFile("/db/equipment.json", out);
  }

  // Sensors seed
  if (!dbFileExists("/db/sensors.json")) {
    JsonDocument doc;
    JsonArray sensors = doc["sensors"].to<JsonArray>();
    for (int l = 1; l <= NUM_LINES; l++) {
      for (int s = 0; s < SENSORS_PER_LINE; s++) {
        JsonObject sensor = sensors.add<JsonObject>();
        char sid[20];
        snprintf(sid, sizeof(sid), "SENSOR-L%d-%02d", l, s + 1);
        sensor["id"]     = sid;
        sensor["line"]   = l;
        sensor["type"]   = SENSOR_TYPES[s];
        sensor["unit"]   = SENSOR_UNITS[s];
        sensor["value"]  = SENSOR_NOMINAL[s] + randomFloat(-2.0f, 2.0f);
        sensor["status"] = "normal";

        JsonObject thresh = sensor["thresholds"].to<JsonObject>();
        thresh["low"]      = SENSOR_LOW_THRESH[s];
        thresh["high"]     = SENSOR_HIGH_THRESH[s];
        thresh["critical"] = SENSOR_CRIT_THRESH[s];
      }
    }
    String out;
    serializeJson(doc, out);
    dbWriteFile("/db/sensors.json", out);
  }

  // Actuators seed
  if (!dbFileExists("/db/actuators.json")) {
    JsonDocument doc;
    JsonArray acts = doc["actuators"].to<JsonArray>();
    for (int l = 1; l <= NUM_LINES; l++) {
      // Motors
      for (int m = 1; m <= 2; m++) {
        JsonObject act = acts.add<JsonObject>();
        char aid[20];
        snprintf(aid, sizeof(aid), "MOTOR-L%d-%02d", l, m);
        act["id"]       = aid;
        act["line"]     = l;
        act["type"]     = "motor";
        act["status"]   = (l == 3) ? "stopped" : "running";
        act["speed"]    = (l == 3) ? 0 : 75;
        act["commands"] = 0;
      }
      // Valves
      for (int v = 1; v <= 2; v++) {
        JsonObject act = acts.add<JsonObject>();
        char aid[20];
        snprintf(aid, sizeof(aid), "VALVE-L%d-%02d", l, v);
        act["id"]     = aid;
        act["line"]   = l;
        act["type"]   = "valve";
        act["state"]  = "open";
        act["commands"] = 0;
      }
      // Pump
      {
        JsonObject act = acts.add<JsonObject>();
        char aid[20];
        snprintf(aid, sizeof(aid), "PUMP-L%d-01", l);
        act["id"]     = aid;
        act["line"]   = l;
        act["type"]   = "pump";
        act["status"] = (l == 3) ? "stopped" : "running";
        act["flow"]   = (l == 3) ? 0 : 120;
        act["commands"] = 0;
      }
    }
    String out;
    serializeJson(doc, out);
    dbWriteFile("/db/actuators.json", out);
  }

  // Alarms seed (empty)
  if (!dbFileExists("/db/alarms.json")) {
    dbWriteFile("/db/alarms.json", "{\"alarms\":[]}");
  }

  // Maintenance seed
  if (!dbFileExists("/db/maintenance.json")) {
    JsonDocument doc;
    JsonArray logs = doc["logs"].to<JsonArray>();

    JsonObject log1 = logs.add<JsonObject>();
    log1["id"]        = "MAINT-001";
    log1["timestamp"] = "2026-01-15T08:30:00Z";
    log1["equipment"] = "MOTOR-L2-01";
    log1["type"]      = "preventive";
    log1["tech"]      = "maintenance";
    log1["notes"]     = "Bearing replacement. Credentials reset to default: maintenance:m4int3n@nc3";
    log1["status"]    = "completed";

    JsonObject log2 = logs.add<JsonObject>();
    log2["id"]        = "MAINT-002";
    log2["timestamp"] = "2026-01-20T14:15:00Z";
    log2["equipment"] = "VALVE-L1-02";
    log2["type"]      = "corrective";
    log2["tech"]      = "operator";
    log2["notes"]     = "Valve actuator recalibrated. Operator credentials: operator:changeme";
    log2["status"]    = "completed";

    JsonObject log3 = logs.add<JsonObject>();
    log3["id"]        = "MAINT-003";
    log3["timestamp"] = "2026-01-25T10:00:00Z";
    log3["equipment"] = "SENSOR-L3-01";
    log3["type"]      = "calibration";
    log3["tech"]      = "maintenance";
    log3["notes"]     = "Temperature sensor drift detected. Recalibrated. FLAG{sensor_fault_L3_calibration_2026}";
    log3["status"]    = "completed";

    String out;
    serializeJson(doc, out);
    dbWriteFile("/db/maintenance.json", out);
  }

  // Incidents seed (templates + empty history)
  if (!dbFileExists("/db/incidents.json")) {
    JsonDocument doc;
    JsonArray templates = doc["templates"].to<JsonArray>();

    const char* types[]  = {"STUCK_VALVE", "SENSOR_FAULT", "MOTOR_OVERLOAD",
                            "TEMPERATURE_SPIKE", "PRESSURE_LOSS", "LOSS_OF_SIGNAL",
                            "SAFETY_BYPASS_DETECTED"};
    const char* descs[]  = {
      "Pneumatic valve stuck in current position",
      "Sensor reading constant (no variation)",
      "Motor current exceeds rated limit",
      "Temperature above critical threshold",
      "System pressure below minimum",
      "Communication lost with field device",
      "Safety interlock bypassed"
    };
    const char* equip[]  = {"VALVE-L2-02", "SENSOR-L1-03", "MOTOR-L2-01",
                            "SENSOR-L2-01", "SENSOR-L4-02", "SENSOR-L3-01",
                            "VALVE-L1-01"};

    for (int i = 0; i < 7; i++) {
      JsonObject tmpl = templates.add<JsonObject>();
      tmpl["type"]         = types[i];
      tmpl["description"]  = descs[i];
      tmpl["equipment"]    = equip[i];
      tmpl["severity"]     = (i < 3) ? "HIGH" : "MEDIUM";
      char flag[64];
      snprintf(flag, sizeof(flag), "FLAG{%s_detected_%04X}", types[i], (uint16_t)esp_random());
      tmpl["sub_flag"] = flag;
    }

    doc["history"] = JsonArray();
    String out;
    serializeJson(doc, out);
    dbWriteFile("/db/incidents.json", out);
  }

  // Logs seed (game state)
  if (!dbFileExists("/db/logs.json")) {
    JsonDocument doc;
    doc["players"]    = JsonArray();
    JsonObject gs     = doc["game_state"].to<JsonObject>();
    gs["current_incidents"] = JsonArray();
    gs["defense_events"]    = JsonArray();
    gs["root_flag_hmac"]    = hmacSHA256("ESP32-SCADA-ROOT-FLAG-2026", String(SECRET_KEY));
    gs["root_flag_unlocked"] = false;
    doc["hints_database"]    = JsonObject();
    String out;
    serializeJson(doc, out);
    dbWriteFile("/db/logs.json", out);
  }

  debugLog("DB", "Seed data created.");
}

// ============================================================
// databaseInit
// ============================================================
void databaseInit() {
  Serial.println("[DB] Initializing LittleFS...");

  if (!LittleFS.begin(true)) {
    Serial.println("[DB] ERROR: LittleFS mount failed!");
    return;
  }

  // Create db directory if needed
  if (!LittleFS.exists("/db")) {
    LittleFS.mkdir("/db");
  }

  // Initialize ringbuffers
  sensorBuffer.init(RINGBUFFER_SENSOR_MAX_ENTRIES);
  alarmBuffer.init(RINGBUFFER_ALARM_MAX_ENTRIES);
  logBuffer.init(RINGBUFFER_LOG_MAX_ENTRIES);
  incidentBuffer.init(RINGBUFFER_INCIDENT_MAX_ENTRIES);
  defenseLogBuffer.init(500);

  // Create seed data
  dbCreateSeedData();

  // Report storage info
  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  Serial.printf("[DB] LittleFS: %u / %u bytes used (%.1f%%)\n",
    used, total, (float)used / total * 100.0f);

  dbInitialized = true;
  Serial.println("[DB] Database ready.");
}

// ============================================================
// databaseAutoSave — periodic persist to flash
// ============================================================
void databaseAutoSave() {
  if (!SAVE_TO_LITTLEFS || !dbInitialized) return;

  // Save recent sensor data
  String sensorData = sensorBuffer.toJsonArray(100);
  dbWriteFile("/db/sensor_recent.json", sensorData);

  // Save recent alarms
  String alarmData = alarmBuffer.toJsonArray(50);
  dbWriteFile("/db/alarm_recent.json", alarmData);

  if (SERIAL_DEBUG) {
    debugLog("DB", "Auto-save complete.");
  }
}

// ============================================================
// Database query helpers
// ============================================================
String dbGetEquipment() {
  return dbReadFile("/db/equipment.json");
}

String dbGetSensors() {
  return dbReadFile("/db/sensors.json");
}

String dbGetActuators() {
  return dbReadFile("/db/actuators.json");
}

String dbGetAlarms() {
  return dbReadFile("/db/alarms.json");
}

String dbGetMaintenance() {
  return dbReadFile("/db/maintenance.json");
}

String dbGetIncidents() {
  return dbReadFile("/db/incidents.json");
}

String dbGetLogs() {
  return dbReadFile("/db/logs.json");
}

void dbSaveEquipment(const String& json) {
  dbWriteFile("/db/equipment.json", json);
}

void dbSaveSensors(const String& json) {
  dbWriteFile("/db/sensors.json", json);
}

void dbSaveActuators(const String& json) {
  dbWriteFile("/db/actuators.json", json);
}

void dbSaveAlarms(const String& json) {
  dbWriteFile("/db/alarms.json", json);
}

void dbSaveIncidents(const String& json) {
  dbWriteFile("/db/incidents.json", json);
}

void dbSaveLogs(const String& json) {
  dbWriteFile("/db/logs.json", json);
}
