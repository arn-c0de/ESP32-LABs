// ============================================================
// 06_Physics.ino — Realistic Sensor Data Generation
// Drift, noise, faults, cross-correlation models
// ============================================================

// Physics state per sensor
struct PhysicsState {
  float baseValue;
  float currentValue;
  float drift;
  float noiseAccum;
  bool  faulted;
  int   faultType;     // 0=none, 1=stuck, 2=spike, 3=dropout
  float faultValue;
  unsigned long faultStart;
  int   faultDurationMs;
};

PhysicsState physicsStates[NUM_LINES * SENSORS_PER_LINE];

// Cross-correlation matrix: motor speed affects vibration and temperature
// Index: [line][sensorType]
float crossCorrelationFactors[5] = {
  0.0f,   // temperature: affected by motor load
  0.1f,   // pressure: slightly affected by pump
  0.87f,  // vibration: strongly correlated with motor speed
  0.3f,   // flow: affected by pump speed
  0.6f    // current: directly from motor
};

void physicsInit() {
  Serial.println("[PHYSICS] Initializing physics engine...");

  for (int l = 0; l < NUM_LINES; l++) {
    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      int idx = l * SENSORS_PER_LINE + s;
      physicsStates[idx].baseValue    = SENSOR_NOMINAL[s];
      physicsStates[idx].currentValue = SENSOR_NOMINAL[s];
      physicsStates[idx].drift        = 0.0f;
      physicsStates[idx].noiseAccum   = 0.0f;
      physicsStates[idx].faulted      = false;
      physicsStates[idx].faultType    = 0;
      physicsStates[idx].faultValue   = 0.0f;
      physicsStates[idx].faultStart   = 0;
      physicsStates[idx].faultDurationMs = 0;
    }
  }

  Serial.printf("[PHYSICS] %d sensor physics models initialized.\n",
    NUM_LINES * SENSORS_PER_LINE);
}

// ===== Apply sensor drift =====
float applyDrift(int idx) {
  physicsStates[idx].drift += randomFloat(-SENSOR_DRIFT_RATE, SENSOR_DRIFT_RATE);
  physicsStates[idx].drift = clampf(physicsStates[idx].drift, -5.0f, 5.0f);
  return physicsStates[idx].drift;
}

// ===== Apply Gaussian noise =====
float applyNoise(int sensorType) {
  return gaussianNoise(0.0f, SENSOR_NOISE_AMPLITUDE * 0.1f);
}

// ===== Apply sensor fault =====
float applyFault(int idx, float normalValue) {
  // DISABLED FOR STABILITY - Faults cause crashes
  return normalValue;
  
  PhysicsState& ps = physicsStates[idx];

  // Check if fault has expired
  if (ps.faulted && millis() - ps.faultStart > (unsigned long)ps.faultDurationMs) {
    ps.faulted   = false;
    ps.faultType = 0;
  }

  // Random fault injection
  if (!ps.faulted && ENABLE_SENSOR_FAULTS && random(100) < FAULT_PROBABILITY_PERCENT) {
    ps.faulted      = true;
    ps.faultType    = random(1, 4);  // 1=stuck, 2=spike, 3=dropout
    ps.faultStart   = millis();
    ps.faultDurationMs = random(10000, 60000);  // 10-60 seconds

    switch (ps.faultType) {
      case 1: ps.faultValue = normalValue; break;   // Stuck at current
      case 2: ps.faultValue = normalValue * 1.5f; break;  // Spike 50% above
      case 3: ps.faultValue = 0.0f; break;           // Dropout to zero
    }

    debugLogf("PHYSICS", "Fault injected: idx=%d type=%d value=%.1f duration=%dms",
      idx, ps.faultType, ps.faultValue, ps.faultDurationMs);
  }

  if (ps.faulted) {
    if (ps.faultType == 1) return ps.faultValue;  // Stuck value
    if (ps.faultType == 2) return ps.faultValue + gaussianNoise(0.0f, 1.0f);  // Spike with noise
    if (ps.faultType == 3) return 0.0f;  // Dropout
  }

  return normalValue;
}

// ===== Cross-correlation: motor speed → sensor influence =====
float applyCrossCorrelation(int line, int sensorType, float motorSpeedPercent) {
  if (!ENABLE_CROSS_CORRELATION) return 0.0f;

  float factor = crossCorrelationFactors[sensorType];
  float influence = (motorSpeedPercent / 100.0f - 0.75f) * factor * SENSOR_NOMINAL[sensorType] * 0.1f;
  return influence;
}

// ===== Main physics update =====
void physicsUpdate() {
  for (int l = 0; l < NUM_LINES; l++) {
    // Get motor speed for this line (for cross-correlation)
    float motorSpeed = getLineMotorSpeed(l + 1);

    for (int s = 0; s < SENSORS_PER_LINE; s++) {
      int idx = l * SENSORS_PER_LINE + s;

      // Base value + drift + noise + correlation
      float val = physicsStates[idx].baseValue;
      val += applyDrift(idx);
      val += applyNoise(s);
      val += SENSOR_CALIBRATION_ERROR * (float)(idx % 3 - 1) * 0.1f;
      val += applyCrossCorrelation(l, s, motorSpeed);

      // Apply fault (may override)
      val = applyFault(idx, val);

      // Clamp to physical limits
      val = clampf(val, 0.0f, SENSOR_CRIT_THRESH[s] * 1.5f);

      physicsStates[idx].currentValue = val;
    }
  }
}

// ===== Get current physics value for a sensor =====
float getPhysicsValue(int line, int sensorType) {
  int idx = (line - 1) * SENSORS_PER_LINE + sensorType;
  if (idx < 0 || idx >= NUM_LINES * SENSORS_PER_LINE) return 0.0f;
  return physicsStates[idx].currentValue;
}

// ===== Check if sensor is faulted =====
bool isSensorFaulted(int line, int sensorType) {
  int idx = (line - 1) * SENSORS_PER_LINE + sensorType;
  if (idx < 0 || idx >= NUM_LINES * SENSORS_PER_LINE) return false;
  return physicsStates[idx].faulted;
}

// ===== Get fault info =====
String getSensorFaultInfo(int line, int sensorType) {
  int idx = (line - 1) * SENSORS_PER_LINE + sensorType;
  if (idx < 0 || idx >= NUM_LINES * SENSORS_PER_LINE) return "{}";

  PhysicsState& ps = physicsStates[idx];
  JsonDocument doc;
  doc["faulted"]  = ps.faulted;
  doc["type"]     = ps.faultType;
  doc["duration"] = ps.faultDurationMs;
  if (ps.faulted) {
    doc["elapsed_ms"] = (int)(millis() - ps.faultStart);
    doc["fault_value"] = ps.faultValue;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

// ===== Inject fault (for incident engine) =====
void injectSensorFault(int line, int sensorType, int faultType, int durationMs) {
  int idx = (line - 1) * SENSORS_PER_LINE + sensorType;
  if (idx < 0 || idx >= NUM_LINES * SENSORS_PER_LINE) return;

  PhysicsState& ps = physicsStates[idx];
  ps.faulted      = true;
  ps.faultType    = faultType;
  ps.faultStart   = millis();
  ps.faultDurationMs = durationMs;

  switch (faultType) {
    case 1: ps.faultValue = ps.currentValue; break;
    case 2: ps.faultValue = ps.currentValue * 1.5f; break;
    case 3: ps.faultValue = 0.0f; break;
  }

  debugLogf("PHYSICS", "Fault injected by incident: L%d S%d type=%d",
    line, sensorType, faultType);
}

// ===== Modify base value (for incident cascades) =====
void setPhysicsBase(int line, int sensorType, float newBase) {
  int idx = (line - 1) * SENSORS_PER_LINE + sensorType;
  if (idx < 0 || idx >= NUM_LINES * SENSORS_PER_LINE) return;
  physicsStates[idx].baseValue = newBase;
}
