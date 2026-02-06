/*
 * Alarm Checking Module
 *
 * Called from loop() after physics update.
 * Compares each sensor's currentValue against its thresholds,
 * logs alarms into the ring buffer, and provides JSON history.
 *
 * All globals (sensors[], alarms[], alarmCount, MAX_ALARMS, etc.)
 * are defined in ESP32-H4CK-SCADA.ino.
 */

// ===== ALARM CHECKING =====

void checkAlarms() {
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    SensorData &s = sensors[i];

    // Skip sensors that haven't been updated yet
    if (s.lastUpdate == 0) continue;

    // Check CRITICAL threshold first (highest priority)
    if (s.currentValue >= s.critThreshold) {
      // Only add if no active CRITICAL alarm for this sensor
      if (!isAlarmActive(s.id, "CRITICAL")) {
        addAlarm(s.id, s.line, "CRITICAL", s.currentValue, s.critThreshold);
        Serial.printf("[ALARM] CRITICAL: %s = %.2f (threshold %.2f)\n",
                      s.id, s.currentValue, s.critThreshold);
      }
    }
    // Check HIGH threshold
    else if (s.currentValue >= s.maxThreshold) {
      if (!isAlarmActive(s.id, "HIGH")) {
        addAlarm(s.id, s.line, "HIGH", s.currentValue, s.maxThreshold);
        Serial.printf("[ALARM] HIGH: %s = %.2f (threshold %.2f)\n",
                      s.id, s.currentValue, s.maxThreshold);
      }
    }
  }
}

// Check if an alarm with the given sensorId and level is already
// present (not yet acknowledged) in the ring buffer.
bool isAlarmActive(const char* sensorId, const char* level) {
  for (int i = 0; i < alarmCount && i < MAX_ALARMS; i++) {
    if (strcmp(alarms[i].sensorId, sensorId) == 0 &&
        strcmp(alarms[i].level, level) == 0 &&
        !alarms[i].acknowledged) {
      // Consider active if it was raised within the last 5 minutes (300 seconds)
      // This prevents alarm spam from minor sensor fluctuations
      if (millis() - alarms[i].timestamp < 300000) {
        return true;
      }
    }
  }
  return false;
}

// ===== ALARM INSERTION =====

void addAlarm(const char* sensorId, int line, const char* level, float value, float threshold) {
  // Ring buffer: write at (alarmCount % MAX_ALARMS), overwrite oldest when full
  int idx = alarmCount % MAX_ALARMS;

  strncpy(alarms[idx].sensorId, sensorId, sizeof(alarms[idx].sensorId) - 1);
  alarms[idx].sensorId[sizeof(alarms[idx].sensorId) - 1] = '\0';
  alarms[idx].line = line;
  strncpy(alarms[idx].level, level, sizeof(alarms[idx].level) - 1);
  alarms[idx].level[sizeof(alarms[idx].level) - 1] = '\0';
  alarms[idx].value = value;
  alarms[idx].threshold = threshold;
  alarms[idx].timestamp = millis();
  alarms[idx].acknowledged = false;

  if (alarmCount < MAX_ALARMS) {
    alarmCount++;
  }
  // When alarmCount >= MAX_ALARMS the oldest entry has been overwritten.
  // We keep alarmCount clamped at MAX_ALARMS so iteration stays bounded.
}

// ===== ALARM HISTORY JSON =====

String getAlarmHistoryJSON(int line, int limit) {
  if (limit <= 0) limit = 50;

  String json = "[";
  bool first = true;
  int emitted = 0;

  // Walk backward from the most recent entry
  int total = (alarmCount < MAX_ALARMS) ? alarmCount : MAX_ALARMS;

  for (int n = total - 1; n >= 0 && emitted < limit; n--) {
    int idx = n % MAX_ALARMS;
    AlarmEntry &a = alarms[idx];

    // Filter by production line (0 = all lines)
    if (line > 0 && a.line != line) continue;

    if (!first) json += ",";
    first = false;

    json += "{";
    json += "\"sensor_id\":\"" + String(a.sensorId) + "\",";
    json += "\"line\":" + String(a.line) + ",";
    json += "\"level\":\"" + String(a.level) + "\",";
    json += "\"value\":" + String(a.value, 2) + ",";
    json += "\"threshold\":" + String(a.threshold, 2) + ",";
    json += "\"timestamp\":" + String(a.timestamp) + ",";
    json += "\"acknowledged\":" + String(a.acknowledged ? "true" : "false");
    json += "}";
    emitted++;
  }

  json += "]";
  return json;
}
