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
  // TEMPORARILY DISABLED FOR STABILITY TESTING
  return;
  
  for (int l = 0; l < NUM_LINES; l++) {
    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      int idx = l * SENSORS_PER_LINE + s;
      float val = sensorArray[idx].value;
      const char* status = sensorArray[idx].status;
      
      char msgBuf[128];

      if (strcmp(status, "critical") == 0) {
        snprintf(msgBuf, sizeof(msgBuf), "%s critical: %.1f%s",
          SENSOR_TYPES[s], val, SENSOR_UNITS[s]);
        triggerAlarm(sensorArray[idx].id, l + 1, ALM_CRITICAL, val,
          SENSOR_CRIT_THRESH[s], msgBuf);
      } else if (strcmp(status, "high") == 0) {
        snprintf(msgBuf, sizeof(msgBuf), "%s high: %.1f%s",
          SENSOR_TYPES[s], val, SENSOR_UNITS[s]);
        triggerAlarm(sensorArray[idx].id, l + 1, ALM_HIGH, val,
          SENSOR_HIGH_THRESH[s], msgBuf);
      } else if (strcmp(status, "fault") == 0) {
        snprintf(msgBuf, sizeof(msgBuf), "%s sensor fault detected",
          SENSOR_TYPES[s]);
        triggerAlarm(sensorArray[idx].id, l + 1, ALM_HIGH, val, 0, msgBuf);
      } else {
        // Clear alarm if sensor returned to normal
        clearAlarmForSensor(sensorArray[idx].id);
      }
    }
  }

  // Escalation: alarms active > 60s escalate severity
  for (int i = 0; i < alarmCount; i++) {
    if (activeAlarms[i].active && strcmp(activeAlarms[i].status, "active") == 0) {
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
                  float value, float threshold, const char* message) {
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
  strncpy(alm.message, message, sizeof(alm.message) - 1);
  alm.message[sizeof(alm.message) - 1] = '\0';
  strncpy(alm.status, "active", sizeof(alm.status) - 1);
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

  debugLogf("ALARMS", "Alarm %s: %s [%s]", alm.id, message,
    severityToString(severity));
}

// ===== Clear alarm for sensor =====
void clearAlarmForSensor(const char* sensorId) {
  for (int i = 0; i < alarmCount; i++) {
    if (activeAlarms[i].active && strcmp(activeAlarms[i].sensorId, sensorId) == 0) {
      activeAlarms[i].active    = false;
      strncpy(activeAlarms[i].status, "cleared", sizeof(activeAlarms[i].status) - 1);
      activeAlarms[i].clearedAt = millis();
    }
  }
}

// ===== Acknowledge alarm =====
bool acknowledgeAlarm(const String& alarmId) {
  for (int i = 0; i < alarmCount; i++) {
    if (String(activeAlarms[i].id) == alarmId && activeAlarms[i].active) {
      strncpy(activeAlarms[i].status, "acknowledged", sizeof(activeAlarms[i].status) - 1);
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
  // TEMPORARILY DISABLED FOR STABILITY TESTING
  return;
  
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
