// ============================================================
// 07_Sensors.ino — Sensor Management + Timeseries
// 20 sensors across 4 production lines (5 per line)
// ============================================================

#include "include/common.h"

SensorData sensorArray[NUM_LINES * SENSORS_PER_LINE];

void sensorsInit() {
  Serial.println("[SENSORS] Initializing sensor array...");

  for (int l = 0; l < NUM_LINES; l++) {
    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      int idx = l * SENSORS_PER_LINE + s;
      snprintf(sensorArray[idx].id, sizeof(sensorArray[idx].id),
               "SENSOR-L%d-%02d", l + 1, s + 1);
      sensorArray[idx].line      = l + 1;
      sensorArray[idx].typeIdx   = s;
      sensorArray[idx].value     = SENSOR_NOMINAL[s];
      sensorArray[idx].prevValue = SENSOR_NOMINAL[s];
      strncpy(sensorArray[idx].status, "normal", sizeof(sensorArray[idx].status) - 1);
      sensorArray[idx].lastUpdate = millis();
    }
  }

  Serial.printf("[SENSORS] %d sensors initialized across %d lines.\n",
    NUM_LINES * SENSORS_PER_LINE, NUM_LINES);
}

// ===== Update all sensors from physics =====
void sensorsUpdate() {
  for (int l = 0; l < NUM_LINES; l++) {
    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      int idx = l * SENSORS_PER_LINE + s;
      SensorData& sd = sensorArray[idx];

      sd.prevValue = sd.value;
      sd.value     = getPhysicsValue(l + 1, s);
      sd.lastUpdate = millis();

      // Determine status
      if (isSensorFaulted(l + 1, s)) {
        strncpy(sd.status, "fault", sizeof(sd.status) - 1);
      } else if (sd.value >= SENSOR_CRIT_THRESH[s]) {
        strncpy(sd.status, "critical", sizeof(sd.status) - 1);
      } else if (sd.value >= SENSOR_HIGH_THRESH[s]) {
        strncpy(sd.status, "high", sizeof(sd.status) - 1);
      } else if (sd.value <= SENSOR_LOW_THRESH[s]) {
        strncpy(sd.status, "low", sizeof(sd.status) - 1);
      } else {
        strncpy(sd.status, "normal", sizeof(sd.status) - 1);
      }

      // Store in ringbuffer
      JsonDocument entry;
      entry["id"]        = sd.id;
      entry["value"]     = round(sd.value * 100.0f) / 100.0f;
      entry["status"]    = sd.status;
      entry["timestamp"] = getISOTimestamp();
      String json;
      serializeJson(entry, json);
      sensorBuffer.push(json);
    }
  }

  // Broadcast via WebSocket
  wsBroadcastSensorData();
}

// ===== WebSocket broadcast =====
void wsBroadcastSensorData() {
  // TEMPORARILY DISABLED FOR STABILITY TESTING
  return;
  
  JsonDocument doc;
  doc["type"] = "sensors";
  JsonArray arr = doc["data"].to<JsonArray>();

  for (int i = 0; i < NUM_LINES * SENSORS_PER_LINE; i++) {
    JsonObject s = arr.add<JsonObject>();
    s["id"]     = sensorArray[i].id;
    s["value"]  = round(sensorArray[i].value * 100.0f) / 100.0f;
    s["status"] = sensorArray[i].status;
    s["type"]   = SENSOR_TYPES[sensorArray[i].typeIdx];
    s["unit"]   = SENSOR_UNITS[sensorArray[i].typeIdx];
    s["line"]   = sensorArray[i].line;
  }

  String out;
  serializeJson(doc, out);
  wsBroadcast(out);
}

// ===== Get sensor readings for API =====
String getSensorReadings(const String& sensorId, int limit) {
  JsonDocument doc;
  doc["sensor_id"] = sensorId;

  // Find the sensor
  int found = -1;
  for (int i = 0; i < NUM_LINES * SENSORS_PER_LINE; i++) {
    if (String(sensorArray[i].id) == sensorId) {
      found = i;
      break;
    }
  }

  if (found < 0) {
    doc["error"] = "Sensor not found";
    String out;
    serializeJson(doc, out);
    return out;
  }

  SensorData& sd = sensorArray[found];
  doc["type"]   = SENSOR_TYPES[sd.typeIdx];
  doc["unit"]   = SENSOR_UNITS[sd.typeIdx];
  doc["line"]   = sd.line;
  doc["status"] = sd.status;

  JsonObject current = doc["current"].to<JsonObject>();
  current["value"]     = round(sd.value * 100.0f) / 100.0f;
  current["timestamp"] = getISOTimestamp();

  JsonObject thresholds = doc["thresholds"].to<JsonObject>();
  thresholds["low"]      = SENSOR_LOW_THRESH[sd.typeIdx];
  thresholds["high"]     = SENSOR_HIGH_THRESH[sd.typeIdx];
  thresholds["critical"] = SENSOR_CRIT_THRESH[sd.typeIdx];

  // History from ringbuffer (filter by sensor ID)
  JsonArray history = doc["history"].to<JsonArray>();
  int count = 0;
  for (int i = sensorBuffer.count - 1; i >= 0 && count < limit; i--) {
    String entry = sensorBuffer.get(i);
    if (entry.indexOf(sensorId) >= 0) {
      JsonDocument entryDoc;
      deserializeJson(entryDoc, entry);
      history.add(entryDoc);
      count++;
    }
  }

  // Physics-based hidden flag for IDOR exploit path
  if (VULN_IDOR_SENSORS && sd.line > 1) {
    // Embed flag hint in metadata when accessing cross-line sensor
    doc["_metadata"] = "FLAG{idor_cross_line_access_" + String(sd.line) + "}";
  }

  String out;
  serializeJson(doc, out);
  return out;
}

// ===== Get specific sensor =====
SensorData* getSensorById(const String& id) {
  for (int i = 0; i < NUM_LINES * SENSORS_PER_LINE; i++) {
    if (String(sensorArray[i].id) == id) return &sensorArray[i];
  }
  return nullptr;
}

// ===== Get sensor value =====
float getSensorValue(int line, int sensorType) {
  if (line < 1 || line > NUM_LINES || sensorType < 0 || sensorType >= SENSORS_PER_LINE) return 0.0f;
  int idx = (line - 1) * SENSORS_PER_LINE + sensorType;
  return sensorArray[idx].value;
}

// ===== Get dashboard summary =====
String getDashboardStatus() {
  JsonDocument doc;

  // Lines overview
  JsonArray lines = doc["lines"].to<JsonArray>();
  for (int l = 1; l <= NUM_LINES; l++) {
    JsonObject line = lines.add<JsonObject>();
    char lid[4];
    snprintf(lid, sizeof(lid), "L%d", l);
    line["id"]     = lid;
    line["name"]   = LINE_NAMES[l - 1];
    line["status"] = getLineStatus(l);

    JsonObject metrics = line["metrics"].to<JsonObject>();
    metrics["output"]     = (int)randomFloat(70.0f, 98.0f);
    metrics["efficiency"] = (int)randomFloat(85.0f, 99.0f);

    JsonArray sensors = line["sensors"].to<JsonArray>();
    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      int idx = (l - 1) * SENSORS_PER_LINE + s;
      JsonObject sensor = sensors.add<JsonObject>();
      sensor["id"]     = sensorArray[idx].id;
      sensor["type"]   = SENSOR_TYPES[s];
      sensor["value"]  = round(sensorArray[idx].value * 100.0f) / 100.0f;
      sensor["unit"]   = SENSOR_UNITS[s];
      sensor["status"] = sensorArray[idx].status;
    }
  }

  // Active alarms count
  doc["active_alarms"] = getActiveAlarmCount();

  // Active incidents count
  doc["active_incidents"] = getActiveIncidentCount();

  // System info
  doc["uptime_sec"]  = millis() / 1000;
  doc["heap_free"]   = ESP.getFreeHeap();
  doc["timestamp"]   = getISOTimestamp();

  String out;
  serializeJson(doc, out);
  return out;
}

// ===== Get line status =====
String getLineStatus(int line) {
  bool hasAlarm = false;
  bool hasCritical = false;

  for (int s = 0; s < SENSORS_PER_LINE; s++) {
    int idx = (line - 1) * SENSORS_PER_LINE + s;
    if (strcmp(sensorArray[idx].status, "critical") == 0 || strcmp(sensorArray[idx].status, "fault") == 0) hasCritical = true;
    if (strcmp(sensorArray[idx].status, "high") == 0) hasAlarm = true;
  }

  if (hasCritical) return "alarm";
  if (hasAlarm) return "warning";
  if (line == 3) return "stopped";  // Line 3 starts stopped
  return "running";
}
