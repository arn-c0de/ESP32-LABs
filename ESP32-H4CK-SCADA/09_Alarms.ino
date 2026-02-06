// ============================================================
// 09_Alarms.ino — Threshold Checks + Escalation
// ============================================================

#include "include/common.h"

#define MAX_ACTIVE_ALARMS 50
Alarm activeAlarms[MAX_ACTIVE_ALARMS];
int alarmCount = 0;
int alarmIdCounter = 1;

void alarmsInit() {
  Serial.println("[ALARMS] Initializing alarm system...");
  memset(activeAlarms, 0, sizeof(activeAlarms));
  alarmCount = 0;
  Serial.println("[ALARMS] Alarm system ready.");
}

// ===== Check all sensors for threshold violations =====
void alarmsUpdate() {
  for (int l = 0; l < NUM_LINES; l++) {
    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      int idx = l * SENSORS_PER_LINE + s;
      float val = sensorArray[idx].value;
      String status = sensorArray[idx].status;

      if (status == "critical") {
        triggerAlarm(sensorArray[idx].id, l + 1, ALM_CRITICAL, val,
          SENSOR_CRIT_THRESH[s],
          String(SENSOR_TYPES[s]) + " critical: " +
          String(val, 1) + SENSOR_UNITS[s]);
      } else if (status == "high") {
        triggerAlarm(sensorArray[idx].id, l + 1, ALM_HIGH, val,
          SENSOR_HIGH_THRESH[s],
          String(SENSOR_TYPES[s]) + " high: " +
          String(val, 1) + SENSOR_UNITS[s]);
      } else if (status == "fault") {
        triggerAlarm(sensorArray[idx].id, l + 1, ALM_HIGH, val, 0,
          String(SENSOR_TYPES[s]) + " sensor fault detected");
      } else {
        // Clear alarm if sensor returned to normal
        clearAlarmForSensor(sensorArray[idx].id);
      }
    }
  }

  // Escalation: alarms active > 60s escalate severity
  for (int i = 0; i < alarmCount; i++) {
    if (activeAlarms[i].active && activeAlarms[i].status == "active") {
      unsigned long age = millis() - activeAlarms[i].triggeredAt;
      if (age > 120000 && activeAlarms[i].severity < ALM_CRITICAL) {
        activeAlarms[i].severity = (AlarmSeverity)(activeAlarms[i].severity + 1);
        debugLogf("ALARMS", "Escalated alarm %s to severity %d",
          activeAlarms[i].id, activeAlarms[i].severity);
      }
    }
  }

  // Broadcast active alarms via WebSocket
  wsBroadcastAlarms();
}

// ===== Trigger alarm =====
void triggerAlarm(const char* sensorId, int line, AlarmSeverity severity,
                  float value, float threshold, const String& message) {
  // Check if alarm already exists for this sensor
  for (int i = 0; i < alarmCount; i++) {
    if (activeAlarms[i].active && strcmp(activeAlarms[i].sensorId, sensorId) == 0) {
      // Update existing alarm
      activeAlarms[i].value    = value;
      activeAlarms[i].severity = max(activeAlarms[i].severity, severity);
      return;
    }
  }

  // Find empty slot
  int slot = -1;
  for (int i = 0; i < MAX_ACTIVE_ALARMS; i++) {
    if (!activeAlarms[i].active) { slot = i; break; }
  }
  if (slot < 0) {
    // Overwrite oldest
    unsigned long oldest = ULONG_MAX;
    for (int i = 0; i < MAX_ACTIVE_ALARMS; i++) {
      if (activeAlarms[i].triggeredAt < oldest) {
        oldest = activeAlarms[i].triggeredAt;
        slot = i;
      }
    }
  }

  Alarm& alm = activeAlarms[slot];
  snprintf(alm.id, sizeof(alm.id), "ALM-%03d", alarmIdCounter++);
  strncpy(alm.sensorId, sensorId, sizeof(alm.sensorId) - 1);
  alm.line        = line;
  alm.severity    = severity;
  alm.value       = value;
  alm.threshold   = threshold;
  alm.message     = message;
  alm.status      = "active";
  alm.triggeredAt = millis();
  alm.clearedAt   = 0;
  alm.active      = true;
  if (slot >= alarmCount) alarmCount = slot + 1;

  // Store in ringbuffer
  JsonDocument entry;
  entry["id"]        = alm.id;
  entry["sensor"]    = sensorId;
  entry["line"]      = line;
  entry["severity"]  = severityToString(severity);
  entry["value"]     = value;
  entry["threshold"] = threshold;
  entry["message"]   = message;
  entry["timestamp"] = getISOTimestamp();
  String json;
  serializeJson(entry, json);
  alarmBuffer.push(json);

  debugLogf("ALARMS", "Alarm %s: %s [%s]", alm.id, message.c_str(),
    severityToString(severity));
}

// ===== Clear alarm for sensor =====
void clearAlarmForSensor(const char* sensorId) {
  for (int i = 0; i < alarmCount; i++) {
    if (activeAlarms[i].active && strcmp(activeAlarms[i].sensorId, sensorId) == 0) {
      activeAlarms[i].active    = false;
      activeAlarms[i].status    = "cleared";
      activeAlarms[i].clearedAt = millis();
    }
  }
}

// ===== Acknowledge alarm =====
bool acknowledgeAlarm(const String& alarmId) {
  for (int i = 0; i < alarmCount; i++) {
    if (String(activeAlarms[i].id) == alarmId && activeAlarms[i].active) {
      activeAlarms[i].status = "acknowledged";
      return true;
    }
  }
  return false;
}

// ===== Get active alarms JSON =====
String getActiveAlarms() {
  JsonDocument doc;
  JsonArray arr = doc["alarms"].to<JsonArray>();

  for (int i = 0; i < alarmCount; i++) {
    if (activeAlarms[i].active) {
      JsonObject a = arr.add<JsonObject>();
      a["id"]        = activeAlarms[i].id;
      a["sensor"]    = activeAlarms[i].sensorId;
      a["line"]      = activeAlarms[i].line;
      a["severity"]  = severityToString(activeAlarms[i].severity);
      a["value"]     = round(activeAlarms[i].value * 100.0f) / 100.0f;
      a["threshold"] = activeAlarms[i].threshold;
      a["message"]   = activeAlarms[i].message;
      a["status"]    = activeAlarms[i].status;
      a["age_sec"]   = (millis() - activeAlarms[i].triggeredAt) / 1000;
    }
  }

  doc["count"] = getActiveAlarmCount();
  String out;
  serializeJson(doc, out);
  return out;
}

// ===== Get alarm history =====
String getAlarmHistory(const String& line, int limit) {
  // IDOR vulnerability: no line access check when VULN_IDOR_ALARMS is true
  String data = alarmBuffer.toJsonArray(limit);

  JsonDocument doc;
  doc["line"]    = line;
  doc["limit"]   = limit;
  doc["history"] = serialized(data);

  // Embed IDOR flag if accessing different line
  if (VULN_IDOR_ALARMS && line.length() > 0) {
    doc["_access_note"] = "FLAG{idor_alarm_history_line_" + line + "}";
  }

  String out;
  serializeJson(doc, out);
  return out;
}

int getActiveAlarmCount() {
  int count = 0;
  for (int i = 0; i < alarmCount; i++) {
    if (activeAlarms[i].active) count++;
  }
  return count;
}

const char* severityToString(AlarmSeverity sev) {
  switch (sev) {
    case ALM_LOW:      return "LOW";
    case ALM_MEDIUM:   return "MEDIUM";
    case ALM_HIGH:     return "HIGH";
    case ALM_CRITICAL: return "CRITICAL";
    default:           return "UNKNOWN";
  }
}

// ===== WebSocket broadcast =====
void wsBroadcastAlarms() {
  if (getActiveAlarmCount() == 0) return;

  JsonDocument doc;
  doc["type"] = "alarms";
  JsonArray arr = doc["data"].to<JsonArray>();

  for (int i = 0; i < alarmCount; i++) {
    if (activeAlarms[i].active) {
      JsonObject a = arr.add<JsonObject>();
      a["id"]       = activeAlarms[i].id;
      a["severity"] = severityToString(activeAlarms[i].severity);
      a["message"]  = activeAlarms[i].message;
      a["line"]     = activeAlarms[i].line;
    }
  }

  String out;
  serializeJson(doc, out);
  wsBroadcast(out);
}
