/*
 * Sensor Management Module
 *
 * Initializes all 20 sensors across 4 production lines,
 * provides JSON output for API endpoints.
 */

// Base value ranges per sensor type
static const float SENSOR_BASE_MIN[] = { 65.0f, 4.0f, 80.0f, 0.3f, 8.0f };
static const float SENSOR_BASE_MAX[] = { 75.0f, 6.0f, 120.0f, 0.5f, 12.0f };

// Threshold offsets from base value
static const float SENSOR_WARN_OFFSET[]  = { 15.0f, 2.0f, 30.0f, 0.5f, 5.0f };
static const float SENSOR_CRIT_OFFSET[]  = { 25.0f, 3.5f, 50.0f, 1.0f, 8.0f };

void initSensors() {
  for (int line = 0; line < NUM_LINES; line++) {
    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      int idx = line * SENSORS_PER_LINE + s;
      SensorData &sd = sensors[idx];

      // ID: SENSOR-L1-01 through SENSOR-L4-05
      snprintf(sd.id, sizeof(sd.id), "SENSOR-L%d-%02d", line + 1, s + 1);
      sd.line = line + 1;
      sd.type = (SensorType)s;  // TEMP=0, PRESSURE=1, FLOW=2, VIBRATION=3, POWER=4

      // Base value: random within range for variety between lines
      float bmin = SENSOR_BASE_MIN[s];
      float bmax = SENSOR_BASE_MAX[s];
      sd.baseValue = bmin + ((float)random(0, 100) / 100.0f) * (bmax - bmin);
      sd.currentValue = sd.baseValue;

      // Thresholds
      sd.minThreshold = bmin * 0.5f;
      sd.maxThreshold = sd.baseValue + SENSOR_WARN_OFFSET[s];
      sd.critThreshold = sd.baseValue + SENSOR_CRIT_OFFSET[s];

      sd.faulted = false;
      sd.lastUpdate = millis();

      // Clear history
      SensorHistory &h = sensorHistory[idx];
      h.writeIndex = 0;
      h.count = 0;
      memset(h.values, 0, sizeof(h.values));
      memset(h.timestamps, 0, sizeof(h.timestamps));
    }
  }

  Serial.printf("[SENSORS] Initialized %d sensors across %d lines\n", TOTAL_SENSORS, NUM_LINES);
}

// Determine sensor status string based on thresholds
static const char* getSensorStatus(const SensorData &s) {
  if (s.faulted) return "FAULT";
  if (s.currentValue >= s.critThreshold) return "CRITICAL";
  if (s.currentValue >= s.maxThreshold) return "WARNING";
  if (s.currentValue <= s.minThreshold) return "LOW";
  return "NORMAL";
}

String getSensorListJSON() {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < TOTAL_SENSORS; i++) {
    const SensorData &s = sensors[i];
    JsonObject obj = arr.createNestedObject();
    obj["id"] = s.id;
    obj["line"] = s.line;
    obj["type"] = SENSOR_TYPE_NAMES[s.type];
    obj["value"] = serialized(String(s.currentValue, 2));
    obj["unit"] = SENSOR_UNITS[s.type];
    obj["status"] = getSensorStatus(s);
    obj["base"] = serialized(String(s.baseValue, 2));
    obj["warn"] = serialized(String(s.maxThreshold, 2));
    obj["crit"] = serialized(String(s.critThreshold, 2));
  }

  String output;
  serializeJson(doc, output);
  doc.clear();
  return output;
}

String getSensorReadingJSON(const char* sensorId, int limit) {
  // Find sensor index by ID
  int idx = -1;
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    if (strcmp(sensors[i].id, sensorId) == 0) {
      idx = i;
      break;
    }
  }

  if (idx < 0) {
    return "{\"error\":\"Sensor not found\"}";
  }

  const SensorData &s = sensors[idx];
  const SensorHistory &h = sensorHistory[idx];

  if (limit <= 0 || limit > SENSOR_HISTORY_SIZE) {
    limit = SENSOR_HISTORY_SIZE;
  }
  int count = (h.count < limit) ? h.count : limit;

  DynamicJsonDocument doc(3072);
  doc["id"] = s.id;
  doc["line"] = s.line;
  doc["type"] = SENSOR_TYPE_NAMES[s.type];
  doc["unit"] = SENSOR_UNITS[s.type];
  doc["current"] = serialized(String(s.currentValue, 2));
  doc["status"] = getSensorStatus(s);

  JsonArray readings = doc.createNestedArray("history");

  // Read from ring buffer, most recent first
  for (int i = 0; i < count; i++) {
    int rIdx = (h.writeIndex - 1 - i + SENSOR_HISTORY_SIZE) % SENSOR_HISTORY_SIZE;
    JsonObject reading = readings.createNestedObject();
    reading["value"] = serialized(String(h.values[rIdx], 2));
    reading["time"] = h.timestamps[rIdx];
  }

  String output;
  serializeJson(doc, output);
  doc.clear();
  return output;
}

// Determine overall line status from its sensors
static const char* getLineStatus(int line) {
  bool hasCritical = false;
  bool hasWarning = false;
  int base = (line - 1) * SENSORS_PER_LINE;

  for (int i = 0; i < SENSORS_PER_LINE; i++) {
    const char* st = getSensorStatus(sensors[base + i]);
    if (strcmp(st, "CRITICAL") == 0 || strcmp(st, "FAULT") == 0) hasCritical = true;
    else if (strcmp(st, "WARNING") == 0 || strcmp(st, "LOW") == 0) hasWarning = true;
  }

  if (hasCritical) return "CRITICAL";
  if (hasWarning) return "WARNING";
  return "NORMAL";
}

String getDashboardStatusJSON() {
  DynamicJsonDocument doc(4096);

  // System uptime
  doc["uptime"] = millis() - systemStartTime;

  // Lines array
  JsonArray linesArr = doc.createNestedArray("lines");
  for (int line = 1; line <= NUM_LINES; line++) {
    JsonObject lineObj = linesArr.createNestedObject();
    lineObj["line"] = line;
    lineObj["status"] = getLineStatus(line);

    JsonArray sensArr = lineObj.createNestedArray("sensors");
    int base = (line - 1) * SENSORS_PER_LINE;
    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      const SensorData &sd = sensors[base + s];
      JsonObject sObj = sensArr.createNestedObject();
      sObj["id"] = sd.id;
      sObj["type"] = SENSOR_TYPE_NAMES[sd.type];
      sObj["value"] = serialized(String(sd.currentValue, 2));
      sObj["unit"] = SENSOR_UNITS[sd.type];
      sObj["status"] = getSensorStatus(sd);
    }
  }

  // Alarms summary
  JsonObject alarmsObj = doc.createNestedObject("alarms");
  int activeCount = 0;
  int critCount = 0;
  for (int i = 0; i < alarmCount; i++) {
    if (!alarms[i].acknowledged) {
      activeCount++;
      if (strcmp(alarms[i].level, "CRITICAL") == 0) critCount++;
    }
  }
  alarmsObj["active"] = activeCount;
  alarmsObj["critical"] = critCount;

  // Actuators summary
  JsonArray actArr = doc.createNestedArray("actuators");
  for (int i = 0; i < TOTAL_ACTUATORS; i++) {
    const ActuatorData &a = actuators[i];
    JsonObject aObj = actArr.createNestedObject();
    aObj["id"] = a.id;
    aObj["type"] = ACTUATOR_TYPE_NAMES[a.type];
    aObj["state"] = ACTUATOR_STATE_NAMES[a.state];
    aObj["speed"] = serialized(String(a.speed, 1));
    aObj["line"] = a.line;
  }

  String output;
  serializeJson(doc, output);
  doc.clear();
  return output;
}
