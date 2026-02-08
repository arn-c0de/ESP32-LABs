/*
 * Physics Simulation Engine
 *
 * Updates sensor values every PHYSICS_UPDATE_INTERVAL (2s).
 * Simulates noise, drift, cross-correlation with actuators,
 * and random sensor faults.
 */

// Internal drift accumulators (per sensor)
static float driftAccum[TOTAL_SENSORS];

void initPhysics() {
  // Zero out drift accumulators
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    driftAccum[i] = 0.0f;
  }
  for (int line = 0; line < NUM_LINES; line++) {
    motorTemp[line] = 65.0f;
  }
  Serial.println("[PHYSICS] Simulation engine initialized");
}

// Approximate gaussian noise using sum of uniform randoms (central limit)
static float gaussianNoise() {
  float sum = 0.0f;
  for (int i = 0; i < 6; i++) {
    sum += (float)random(0, 1000) / 1000.0f;
  }
  return (sum - 3.0f) / 3.0f;  // roughly N(0, ~0.33)
}

void updatePhysics() {
  unsigned long now = millis();

  // Don't inject faults during startup (first 60 seconds) to allow system stabilization
  bool allowFaults = (now >= 60000);

  // Pre-compute which lines have a running motor
  bool motorRunning[NUM_LINES];
  float motorSpeed[NUM_LINES];
  bool valveOpen[NUM_LINES];
  float valvePos[NUM_LINES];
  for (int line = 0; line < NUM_LINES; line++) {
    motorRunning[line] = false;
    motorSpeed[line] = 0.0f;
    valveOpen[line] = false;
    valvePos[line] = 0.0f;
    // Each line has ACTUATORS_PER_LINE actuators; first is MOTOR
    int motorIdx = line * ACTUATORS_PER_LINE;
    if (motorIdx < TOTAL_ACTUATORS &&
        actuators[motorIdx].type == MOTOR &&
        actuators[motorIdx].state == ACT_RUNNING) {
      motorRunning[line] = true;
      motorSpeed[line] = actuators[motorIdx].speed;
    }
    int valveIdx = motorIdx + 1;
    if (valveIdx < TOTAL_ACTUATORS && actuators[valveIdx].type == VALVE) {
      const ActuatorData &v = actuators[valveIdx];
      bool vOn = (v.state == ACT_RUNNING || v.state == ACT_STARTING) &&
                 (v.targetSpeed > 0.0f || v.speed > 0.0f);
      valveOpen[line] = vOn;
      if (vOn) {
        valvePos[line] = max(v.speed, v.targetSpeed);
      }
    }
  }

  // Update motor temperature per line (overheat if motor runs with closed valve)
  for (int line = 0; line < NUM_LINES; line++) {
    float baseTemp = 65.0f;
    float target = baseTemp;
    float speedFactor = motorSpeed[line] / 100.0f;
    if (motorRunning[line]) {
      target = baseTemp + 8.0f * speedFactor;
      if (!valveOpen[line]) {
        target += 25.0f;
      }
    }
    motorTemp[line] += (target - motorTemp[line]) * 0.15f;

    int motorIdx = line * ACTUATORS_PER_LINE;
    if (motorRunning[line] && motorTemp[line] >= 95.0f &&
        motorIdx < TOTAL_ACTUATORS && actuators[motorIdx].state != ACT_FAULT) {
      ActuatorData &motor = actuators[motorIdx];
      motor.state = ACT_FAULT;
      motor.speed = 0.0f;
      motor.targetSpeed = 0.0f;
      motor.stateChangeTime = now;
      Serial.printf("[SAFETY] Motor overheat on line %d (%.1fC). Stopping motor.\n",
                    line + 1, motorTemp[line]);
    }
  }

  // Update each sensor
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    SensorData &s = sensors[i];
    int lineIdx = s.line - 1;  // 0-based

    // If sensor disabled, freeze value until re-enabled
    if (!s.enabled) continue;

    // Noise component - proportional to base value (not absolute)
    // SENSOR_NOISE_AMPLITUDE is now a percentage (e.g., 0.03 = 3%)
    float noise = gaussianNoise() * s.baseValue * SENSOR_NOISE_AMPLITUDE;

    // Drift component (slow random walk)
    driftAccum[i] += ((float)random(-100, 101) / 100.0f) * SENSOR_DRIFT_RATE;
    // Clamp drift to +/- 10% of base value
    float maxDrift = s.baseValue * 0.10f;
    if (driftAccum[i] > maxDrift) driftAccum[i] = maxDrift;
    if (driftAccum[i] < -maxDrift) driftAccum[i] = -maxDrift;

    // Cross-correlation: motor running affects vibration and power
    float crossEffect = 0.0f;
    if (lineIdx >= 0 && lineIdx < NUM_LINES && motorRunning[lineIdx]) {
      float speedFactor = motorSpeed[lineIdx] / 100.0f;
      
      // Overheat factor: above 80°C motor becomes inefficient and vibrates
      float mTemp = motorTemp[lineIdx];
      float overheatFactor = 1.0f;
      if (mTemp > 80.0f) {
        // Linear scale from 80°C (1.0) to 95°C (2.5)
        overheatFactor = 1.0f + ((mTemp - 80.0f) / 15.0f) * 1.5f;
      }
      
      if (s.type == VIBRATION) {
        // Vibration increases significantly with overheat
        crossEffect = s.baseValue * 1.5f * speedFactor * overheatFactor;
      } else if (s.type == POWER) {
        // Power consumption drops with overheat (inefficient/struggle)
        float powerFactor = (mTemp > 80.0f) ? (1.0f / overheatFactor) : 1.0f;
        crossEffect = s.baseValue * 0.8f * speedFactor * powerFactor;
      } else if (s.type == TEMP) {
        crossEffect = 5.0f * speedFactor;  // slight temperature rise
      }
    }

    // Yield periodically to feed watchdog (every 4 sensors)
    if (i % 4 == 0) yield();

    // Base adjustment for line shutdown (actuators off)
    float baseAdjusted = s.baseValue;
    if (lineIdx >= 0 && lineIdx < NUM_LINES) {
      if (s.type == FLOW || s.type == PRESSURE) {
        float vf = valveOpen[lineIdx] ? (valvePos[lineIdx] / 100.0f) : 0.0f;
        baseAdjusted = s.baseValue * (0.1f + 0.9f * vf);
      } else if (s.type == POWER || s.type == VIBRATION) {
        baseAdjusted = s.baseValue * (motorRunning[lineIdx] ? 1.0f : 0.1f);
      } else if (s.type == TEMP) {
        // Sync TEMP sensor with actual motor temperature
        baseAdjusted = motorTemp[lineIdx];
      }
    }

    // Compute new value
    float newVal = baseAdjusted + noise + driftAccum[i] + crossEffect;

    // Fault injection (only after startup period)
    if (allowFaults && ENABLE_SENSOR_FAULTS && !s.faulted) {
      if ((int)random(0, 10000) < (FAULT_PROBABILITY_PERCENT * 100)) {
        s.faulted = true;
        // 50/50: stuck value or spike
        if (random(0, 2) == 0) {
          // Stuck: value stays at current reading (don't update)
        } else {
          // Spike: jump to 2-3x base value
          newVal = s.baseValue * (2.0f + (float)random(0, 100) / 100.0f);
        }
      }
    }

    // If sensor is faulted and stuck, keep previous value
    if (s.faulted && random(0, 100) < 70) {
      // 70% chance stuck sensor stays stuck, 30% chance it recovers
      // (keep currentValue unchanged for stuck)
      // But allow occasional recovery
    } else {
      if (s.faulted && random(0, 100) < 30) {
        s.faulted = false;  // recover from fault (30% recovery chance)
        driftAccum[i] = 0.0f;
      }
      s.currentValue = newVal;
    }

    // Ensure non-negative for physical quantities
    if (s.currentValue < 0.0f) {
      s.currentValue = 0.0f;
    }

    s.lastUpdate = now;

    // Store in history ring buffer
    if (i >= 0 && i < TOTAL_SENSORS) {
      SensorHistory &h = sensorHistory[i];
      h.values[h.writeIndex] = s.currentValue;
      h.timestamps[h.writeIndex] = now;
      h.writeIndex = (h.writeIndex + 1) % SENSOR_HISTORY_SIZE;
      if (h.count < SENSOR_HISTORY_SIZE) {
        h.count++;
      }
    }
  }

  // Update actuator state machine
  for (int i = 0; i < TOTAL_ACTUATORS; i++) {
    ActuatorData &a = actuators[i];
    unsigned long elapsed = now - a.stateChangeTime;

    if (a.state == ACT_STARTING && elapsed > 3000) {
      a.state = ACT_RUNNING;
      a.speed = a.targetSpeed;
      a.stateChangeTime = now;
    } else if (a.state == ACT_STOPPING && elapsed > 2000) {
      a.state = ACT_STOPPED;
      a.speed = 0.0f;
      a.stateChangeTime = now;
    }
  }
}
