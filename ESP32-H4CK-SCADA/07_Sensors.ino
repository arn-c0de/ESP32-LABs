/*
 * Sensor Management Module
 *
 * Initializes all 20 sensors across 4 production lines,
 * provides JSON output for API endpoints.
 */

// Base value ranges per sensor type
static const float SENSOR_BASE_MIN[] = { 65.0f, 4.0f, 80.0f, 0.3f, 8.0f };
static const float SENSOR_BASE_MAX[] = { 75.0f, 6.0f, 120.0f, 0.5f, 12.0f };

// Threshold offsets from normal operating value (more generous to avoid false alarms)
static const float SENSOR_WARN_OFFSET[]  = { 20.0f, 2.5f, 40.0f, 1.0f, 8.0f };
static const float SENSOR_CRIT_OFFSET[]  = { 35.0f, 4.5f, 65.0f, 1.8f, 14.0f };

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
      
      // FIXED: Initialize currentValue to account for normal motor operation (75% speed)
      // so VIBRATION and POWER don't immediately trigger alarms
      float normalOpAdjustment = 0.0f;
      if (s == VIBRATION) {
        // Motors at 75% -> crossEffect = baseValue * 1.5 * 0.75 = baseValue * 1.125
        normalOpAdjustment = sd.baseValue * 1.125f;
      } else if (s == POWER) {
        // Motors at 75% -> crossEffect = baseValue * 0.8 * 0.75 = baseValue * 0.6
        normalOpAdjustment = sd.baseValue * 0.6f;
      } else if (s == TEMP) {
        // Motors at 75% -> crossEffect = 5.0 * 0.75 = 3.75
        normalOpAdjustment = 3.75f;
      }
      sd.currentValue = sd.baseValue + normalOpAdjustment;

      // Thresholds - adjusted to account for normal motor operation cross-effects
      sd.minThreshold = bmin * 0.5f;
      sd.maxThreshold = sd.currentValue + SENSOR_WARN_OFFSET[s];
      sd.critThreshold = sd.currentValue + SENSOR_CRIT_OFFSET[s];

      sd.faulted = false;
      sd.enabled = true;
      sd.lastUpdate = millis();

      if (s == TEMP) {
        motorTemp[line] = sd.currentValue;
      }

      // Link sensors to actuators (one or more per sensor)
      // TEMP: motor+valve, PRESSURE/FLOW: valve, VIBRATION/POWER: motor
      uint8_t linkMask = ACTUATOR_MASK_MOTOR;
      if (s == PRESSURE || s == FLOW) linkMask = ACTUATOR_MASK_VALVE;
      else if (s == VIBRATION || s == POWER) linkMask = ACTUATOR_MASK_MOTOR;
      else linkMask = ACTUATOR_MASK_MOTOR | ACTUATOR_MASK_VALVE;
      sensorActuatorMask[idx] = linkMask;

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

// Actuator considered active if running/starting and has non-zero target/speed
static bool isActuatorEffectivelyOn(const ActuatorData &a) {
  if (a.state != ACT_RUNNING && a.state != ACT_STARTING) return false;
  if (a.targetSpeed > 0.0f || a.speed > 0.0f) return true;
  return false;
}

// Sensor is monitored only if enabled AND at least one linked actuator is active
bool isSensorMonitored(int sensorIdx) {
  if (sensorIdx < 0 || sensorIdx >= TOTAL_SENSORS) return false;
  const SensorData &s = sensors[sensorIdx];
  if (!s.enabled) return false;

  uint8_t mask = sensorActuatorMask[sensorIdx];
  if (mask == 0) return true;  // no mapping -> always monitored

  int base = (s.line - 1) * ACTUATORS_PER_LINE;
  bool anyOn = false;

  if ((mask & ACTUATOR_MASK_MOTOR) && base < TOTAL_ACTUATORS) {
    anyOn = anyOn || isActuatorEffectivelyOn(actuators[base]);
  }
  if ((mask & ACTUATOR_MASK_VALVE) && (base + 1) < TOTAL_ACTUATORS) {
    anyOn = anyOn || isActuatorEffectivelyOn(actuators[base + 1]);
  }

  return anyOn;
}

static bool isLineEfficiencyAlarm(const AlarmEntry &a) {
  if (strncmp(a.sensorId, "LINE-L", 6) != 0) return false;
  return strstr(a.sensorId, "-EFF") != nullptr;
}

int countActiveLineAlarms(int line, bool excludeLineEfficiency) {
  int lineAlarms = 0;
  for (int i = 0; i < alarmCount; i++) {
    if (alarms[i].line != line || alarms[i].acknowledged) continue;
    if (excludeLineEfficiency && isLineEfficiencyAlarm(alarms[i])) continue;
    lineAlarms++;
  }
  return lineAlarms;
}

float getLineEfficiency(int line, int lineAlarms) {
  int base = (line - 1) * ACTUATORS_PER_LINE;
  float motorSpeed = 0.0f;
  float valvePos = 100.0f;
  bool motorOn = false;

  if (base >= 0 && base < TOTAL_ACTUATORS && actuators[base].type == MOTOR) {
    motorOn = isActuatorEffectivelyOn(actuators[base]);
    motorSpeed = max(actuators[base].speed, actuators[base].targetSpeed);
  }

  if ((base + 1) < TOTAL_ACTUATORS && actuators[base + 1].type == VALVE) {
    if (isActuatorEffectivelyOn(actuators[base + 1])) {
      valvePos = max(actuators[base + 1].speed, actuators[base + 1].targetSpeed);
    } else {
      valvePos = 0.0f;
    }
  }

  float efficiency = 0.0f;
  if (motorOn && motorSpeed > 0.01f && valvePos > 0.0f) {
    efficiency = (motorSpeed * 0.85f) * (valvePos / 100.0f);
    efficiency -= (lineAlarms * 10.0f);
  }

  if (efficiency < 0.0f) efficiency = 0.0f;
  if (efficiency > 100.0f) efficiency = 100.0f;
  return efficiency;
}

String getSensorListJSON() {
  static String sensorListCache;
  static unsigned long sensorListCacheAt = 0;
  const unsigned long now = millis();
  if (sensorListCache.length() > 0 && (now - sensorListCacheAt) < 800) {
    return sensorListCache;
  }
  if (ESP.getFreeHeap() < 20000) {
    if (sensorListCache.length() > 0) return sensorListCache;
    return "{\"error\":\"low memory\"}";
  }
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < TOTAL_SENSORS; i++) {
    if (doc.overflowed()) {
      Serial.println("[SENSORS] JSON overflow in getSensorListJSON");
      break;
    }
    const SensorData &s = sensors[i];
    JsonObject obj = arr.createNestedObject();
    obj["id"] = s.id;
    obj["line"] = s.line;
    obj["type"] = SENSOR_TYPE_NAMES[s.type];
    obj["value"] = s.currentValue;
    obj["unit"] = SENSOR_UNITS[s.type];
    obj["status"] = getSensorStatus(s);
    obj["base"] = s.baseValue;
    obj["min_threshold"] = s.minThreshold;
    obj["max_threshold"] = s.maxThreshold;
    obj["crit_threshold"] = s.critThreshold;
    obj["faulted"] = s.faulted;
    obj["enabled"] = s.enabled;
  }

  sensorListCache = "";
  sensorListCache.reserve(4096);
  serializeJson(doc, sensorListCache);
  doc.clear();
  sensorListCacheAt = now;
  return sensorListCache;
}

String getSensorReadingJSON(const char* sensorId, int limit) {
  if (ESP.getFreeHeap() < 20000) {
    return "{\"error\":\"low memory\"}";
  }
  // Find sensor index by ID
  int idx = -1;
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    if (strcmp(sensors[i].id, sensorId) == 0) {
      idx = i;
      break;
    }
  }

  if (idx < 0 || idx >= TOTAL_SENSORS) {
    return "{\"error\":\"Sensor not found\"}";
  }

  const SensorData &s = sensors[idx];
  const SensorHistory &h = sensorHistory[idx];

  if (limit <= 0 || limit > SENSOR_HISTORY_SIZE) {
    limit = SENSOR_HISTORY_SIZE;
  }
  int count = (h.count < limit) ? h.count : limit;

  DynamicJsonDocument doc(2048);
  JsonArray readings = doc.to<JsonArray>();

  // Read from ring buffer, most recent first
  for (int i = 0; i < count; i++) {
    int rIdx = (h.writeIndex - 1 - i + SENSOR_HISTORY_SIZE) % SENSOR_HISTORY_SIZE;
    JsonObject reading = readings.createNestedObject();
    reading["value"] = h.values[rIdx];
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
    if (!isSensorMonitored(base + i)) continue;
    const char* st = getSensorStatus(sensors[base + i]);
    if (strcmp(st, "CRITICAL") == 0 || strcmp(st, "FAULT") == 0) hasCritical = true;
    else if (strcmp(st, "WARNING") == 0 || strcmp(st, "LOW") == 0) hasWarning = true;
  }

  if (hasCritical) return "CRITICAL";
  if (hasWarning) return "WARNING";
  return "NORMAL";
}

String getDashboardStatusJSON() {
  static String dashboardCache;
  static unsigned long dashboardCacheAt = 0;
  const unsigned long now = millis();
  if (dashboardCache.length() > 0 && (now - dashboardCacheAt) < 800) {
    return dashboardCache;
  }
  if (ESP.getFreeHeap() < 20000) {
    if (dashboardCache.length() > 0) return dashboardCache;
    return "{\"error\":\"low memory\"}";
  }
  DynamicJsonDocument doc(4096);

  // System uptime (seconds)
  unsigned long uptimeMs = millis() - systemStartTime;
  doc["uptime"] = uptimeMs / 1000;
  
  // System statistics
  doc["total_sensors"] = TOTAL_SENSORS;
  
  // Count active actuators (running or starting)
  int activeActCount = 0;
  for (int i = 0; i < TOTAL_ACTUATORS; i++) {
    if (actuators[i].state == ACT_RUNNING || actuators[i].state == ACT_STARTING) {
      activeActCount++;
    }
  }
  doc["active_actuators"] = activeActCount;

  // Count active alarms
  int activeAlarms = 0;
  for (int i = 0; i < alarmCount; i++) {
    if (!alarms[i].acknowledged) {
      activeAlarms++;
    }
  }
  doc["active_alarms"] = activeAlarms;
  
  // CPU usage (simulate based on active components)
  int cpuUsage = 25 + (activeActCount * 10) + (activeAlarms * 5);
  if (cpuUsage > 100) cpuUsage = 100;
  doc["cpu_usage"] = cpuUsage;
  
  // Memory usage
  doc["memory_usage"] = 42 + (uptimeMs % 30000) / 1000;  // Simulate 42-72%
  
  // Network stats
  doc["total_requests"] = totalRequests;
  doc["failed_requests"] = failedRequests;

  // Production Lines array with detailed metrics
  JsonArray linesArr = doc.createNestedArray("lines");
  for (int line = 1; line <= NUM_LINES; line++) {
    JsonObject lineObj = linesArr.createNestedObject();
    lineObj["number"] = line;
    
    String status = getLineStatus(line);
    lineObj["status"] = status;
    
    // Get sensor values for this line
    int base = (line - 1) * SENSORS_PER_LINE;
    float temp = 0, pressure = 0, flow = 0, vibration = 0, power = 0;
    
    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      const SensorData &sd = sensors[base + s];
      if (sd.type == TEMP) temp = sd.currentValue;
      else if (sd.type == PRESSURE) pressure = sd.currentValue;
      else if (sd.type == FLOW) flow = sd.currentValue;
      else if (sd.type == VIBRATION) vibration = sd.currentValue;
      else if (sd.type == POWER) power = sd.currentValue;
    }
    
    lineObj["temp"] = temp;
    lineObj["pressure"] = pressure;
    lineObj["flow"] = flow;
    lineObj["vibration"] = vibration;
    lineObj["power"] = power;
    
    // Get motor state for this line (first actuator is motor)
    int motorIdx = (line - 1) * ACTUATORS_PER_LINE;
    if (motorIdx >= 0 && motorIdx < TOTAL_ACTUATORS && (line - 1) >= 0 && (line - 1) < NUM_LINES) {
      const ActuatorData &motor = actuators[motorIdx];
      lineObj["motor_state"] = ACTUATOR_STATE_NAMES[motor.state];
      lineObj["motor_speed"] = motor.speed;
      lineObj["motor_temp"] = motorTemp[line - 1];
    } else {
      lineObj["motor_state"] = "UNKNOWN";
      lineObj["motor_speed"] = 0;
      lineObj["motor_temp"] = 65.0;
    }

    // Get valve state for this line (second actuator is valve)
    int valveIdx = motorIdx + 1;
    if (valveIdx < TOTAL_ACTUATORS) {
      const ActuatorData &valve = actuators[valveIdx];
      lineObj["valve_state"] = ACTUATOR_STATE_NAMES[valve.state];
      lineObj["valve_pos"] = valve.speed;
    } else {
      lineObj["valve_state"] = "UNKNOWN";
      lineObj["valve_pos"] = 0;
    }
    
    // Count alarms for this line
    int lineAlarms = countActiveLineAlarms(line, true);
    lineObj["alarms"] = lineAlarms;
    
    // Production efficiency (simulate based on motor speed and alarms)
    // Check if motor is actually running, not sensor status
    lineObj["efficiency"] = (int)getLineEfficiency(line, lineAlarms);
  }

  dashboardCache = "";
  dashboardCache.reserve(4096);
  serializeJson(doc, dashboardCache);
  doc.clear();
  dashboardCacheAt = now;
  return dashboardCache;
}
