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

// Forward declarations
bool isAlarmActive(const char* sensorId, const char* level);
bool isSensorMonitored(int sensorIdx);
int countActiveLineAlarms(int line, bool excludeLineEfficiency);
float getLineEfficiency(int line, int lineAlarms);

// ===== ALARM CHECKING =====

void checkAlarms() {
  // FIXED: Don't check alarms during first 30 seconds after boot to allow system stabilization
  if (millis() < 30000) {
    // Initialize line efficiency states
    for (int line = 0; line < NUM_LINES; line++) {
      lastLineEffState[line] = -1.0f;
    }
    return;
  }

  for (int i = 0; i < TOTAL_SENSORS; i++) {
    SensorData &s = sensors[i];

    // Skip sensors that haven't been updated yet
    if (s.lastUpdate == 0) continue;

    // Skip sensors that are disabled or not monitored by running actuators
    if (!isSensorMonitored(i)) continue;

    // Yield every 5 sensors to feed watchdog
    if (i % 5 == 0) yield();

    // CRITICAL alarms should trigger when the sensor is monitored
    if (s.currentValue >= s.critThreshold) {
      if (!isAlarmActive(s.id, "CRITICAL")) {
        addAlarm(s.id, s.line, "CRITICAL", s.currentValue, s.critThreshold);
        Serial.printf("[ALARM] CRITICAL: %s = %.2f (threshold %.2f)\n",
                      s.id, s.currentValue, s.critThreshold);
      }
      continue; // highest priority
    }

    // HIGH alarms when the sensor is monitored and over threshold
    if (s.currentValue >= s.maxThreshold) {
      if (!isAlarmActive(s.id, "HIGH")) {
        addAlarm(s.id, s.line, "HIGH", s.currentValue, s.maxThreshold);
        Serial.printf("[ALARM] HIGH: %s = %.2f (threshold %.2f)\n",
                      s.id, s.currentValue, s.maxThreshold);
      }
    }
  }

  // Line efficiency alerts (trigger on state change)
  for (int line = 1; line <= NUM_LINES; line++) {
    int lineAlarms = countActiveLineAlarms(line, true);
    float efficiency = getLineEfficiency(line, lineAlarms);
    int lineIdx = line - 1;
    
    // Defensive bounds check
    if (lineIdx < 0 || lineIdx >= NUM_LINES) {
      Serial.printf("[ALARM] ERROR: Invalid lineIdx %d (line %d)\n", lineIdx, line);
      continue;
    }
    
    char lineId[20];
    snprintf(lineId, sizeof(lineId), "LINE-L%d-EFF", line);

    // Check if efficiency state changed to trigger new alarm
    bool stateChanged = false;
    if (lastLineEffState[lineIdx] < 0.0f) {
      // First check - initialize
      lastLineEffState[lineIdx] = efficiency;
    } else {
      // Determine if we crossed a threshold boundary
      bool wasZero = (lastLineEffState[lineIdx] <= 0.0f);
      bool wasLow = (lastLineEffState[lineIdx] > 0.0f && lastLineEffState[lineIdx] < 25.0f);
      bool isZero = (efficiency <= 0.0f);
      bool isLow = (efficiency > 0.0f && efficiency < 25.0f);
      
      if (wasZero != isZero || wasLow != isLow) {
        stateChanged = true;
        lastLineEffState[lineIdx] = efficiency;
      }
    }

    if (efficiency <= 0.0f) {
      if (stateChanged || !isAlarmActive(lineId, "CRITICAL")) {
        addAlarm(lineId, line, "CRITICAL", efficiency, 0.0f);
        Serial.printf("[ALARM] ZERO EFF: Line %d = %.1f%%\n", line, efficiency);
      }
    } else if (efficiency < 25.0f) {
      if (stateChanged || !isAlarmActive(lineId, "HIGH")) {
        addAlarm(lineId, line, "HIGH", efficiency, 25.0f);
        Serial.printf("[ALARM] LOW EFF: Line %d = %.1f%% (threshold 25%%)\n",
                      line, efficiency);
      }
    }
  }

  if (DEBUG_MODE) {
    Serial.printf("[ALARM-CHECK] %dActuators %dSensors\n", TOTAL_ACTUATORS, TOTAL_SENSORS);
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

  // Log CRITICAL alarms as security incidents so they persist in the
  // Incidents page even after acknowledgment
  if (strcmp(level, "CRITICAL") == 0) {
    String details = String(sensorId) + " Line " + String(line) +
                     " val=" + String(value, 2) + " thresh=" + String(threshold, 2);
    addDefenseAlert("CRITICAL_ALARM", "system", details);
  }
}

// ===== ALARM HISTORY JSON =====

String getAlarmHistoryJSON(int line, int limit) {
  if (limit <= 0) limit = 30;
  if (limit > MAX_ALARMS) limit = MAX_ALARMS;

  // Guard: check heap before allocating
  if (ESP.getFreeHeap() < 20000) {
    return "{\"error\":\"low memory\"}";
  }

  // Fixed-size JSON doc instead of unbounded String concatenation
  DynamicJsonDocument doc(3072);
  JsonArray arr = doc.to<JsonArray>();

  int emitted = 0;
  int total = (alarmCount < MAX_ALARMS) ? alarmCount : MAX_ALARMS;

  for (int n = total - 1; n >= 0 && emitted < limit; n--) {
    int idx = n % MAX_ALARMS;
    AlarmEntry &a = alarms[idx];

    if (line > 0 && a.line != line) continue;
    if (a.acknowledged && (millis() - a.timestamp) > 30000) continue;

    // Stop if doc is running out of capacity
    if (doc.overflowed()) break;

    JsonObject obj = arr.createNestedObject();
    obj["sensor_id"] = a.sensorId;
    obj["line"] = a.line;
    obj["level"] = a.level;
    obj["value"] = a.value;
    obj["threshold"] = a.threshold;
    obj["timestamp"] = a.timestamp;
    obj["acknowledged"] = a.acknowledged;
    emitted++;
  }

  String output;
  serializeJson(doc, output);
  doc.clear();
  return output;
}
