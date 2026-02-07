/*
 * Incident Generation Engine - Phase 2 Stub
 *
 * Will generate random or scripted incidents (equipment failures,
 * cyber-attacks, process upsets) that students must diagnose and
 * respond to through the SCADA HMI.
 *
 * Placeholder functions for Phase 2 implementation.
 */

// ===== MANUAL INCIDENT TRIGGERING =====

bool triggerIncident(String type, String details) {
  Serial.println("[INCIDENT] ========================================");
  Serial.printf("[INCIDENT] Type: %s\n", type.c_str());
  Serial.printf("[INCIDENT] Details: %s\n", details.length() > 0 ? details.c_str() : "(none)");
  Serial.println("[INCIDENT] ========================================");
  
  // Apply incident effects based on type
  if (type == "sensor-fault") {
    // Randomly fault 1-3 sensors
    int numFaults = random(1, 4);
    for (int i = 0; i < numFaults; i++) {
      int sensorIdx = random(0, SENSOR_COUNT);
      sensors[sensorIdx].faulted = true;
      sensors[sensorIdx].currentValue = 0.0;
      Serial.printf("[INCIDENT] Sensor %s set to FAULT\n", sensors[sensorIdx].id);
      addAlarm(sensors[sensorIdx].id, sensors[sensorIdx].line, "CRITICAL", 
               sensors[sensorIdx].currentValue, sensors[sensorIdx].minThreshold);
    }
    return true;
  }
  
  else if (type == "motor-failure") {
    // Stop a random motor
    for (int i = 0; i < ACTUATOR_COUNT; i++) {
      if (actuators[i].type == MOTOR && actuators[i].state == ACT_RUNNING) {
        actuators[i].state = ACT_FAULT;
        actuators[i].speed = 0;
        Serial.printf("[INCIDENT] Motor %s failed\n", actuators[i].id);
        return true;
      }
    }
    return false; // No running motors found
  }
  
  else if (type == "valve-stuck") {
    // Stuck a random valve
    for (int i = 0; i < ACTUATOR_COUNT; i++) {
      if (actuators[i].type == VALVE) {
        actuators[i].state = ACT_FAULT;
        actuators[i].locked = true;
        Serial.printf("[INCIDENT] Valve %s stuck at %.0f%%\n", actuators[i].id, actuators[i].speed);
        return true;
      }
    }
    return false;
  }
  
  else if (type == "line-shutdown") {
    // Shutdown entire production line
    int line = random(1, NUM_LINES + 1);
    Serial.printf("[INCIDENT] Shutting down Line %d\n", line);
    for (int i = 0; i < SENSOR_COUNT; i++) {
      if (sensors[i].line == line) {
        sensors[i].faulted = true;
        sensors[i].enabled = false;
        sensors[i].currentValue = 0.0;
      }
    }
    for (int i = 0; i < ACTUATOR_COUNT; i++) {
      if (actuators[i].line == line) {
        actuators[i].state = ACT_STOPPED;
        actuators[i].speed = 0;
        actuators[i].targetSpeed = 0;
      }
    }
    return true;
  }
  
  else if (type == "pressure-spike") {
    // Spike pressure sensors
    for (int i = 0; i < SENSOR_COUNT; i++) {
      if (sensors[i].type == PRESSURE) {
        sensors[i].currentValue = sensors[i].critThreshold * 1.2; // 120% of critical
        Serial.printf("[INCIDENT] Pressure spike on %s: %.2f bar\n", 
                      sensors[i].id, sensors[i].currentValue);
        addAlarm(sensors[i].id, sensors[i].line, "CRITICAL", 
                 sensors[i].currentValue, sensors[i].critThreshold);
      }
    }
    return true;
  }
  
  else if (type == "temperature-runaway") {
    // Temperature runaway on random line
    int line = random(1, NUM_LINES + 1);
    for (int i = 0; i < SENSOR_COUNT; i++) {
      if (sensors[i].type == TEMP && sensors[i].line == line) {
        sensors[i].currentValue = sensors[i].critThreshold + 15.0; // Way over limit
        Serial.printf("[INCIDENT] Temperature runaway on %s: %.2f°C\n", 
                      sensors[i].id, sensors[i].currentValue);
        addAlarm(sensors[i].id, sensors[i].line, "CRITICAL", 
                 sensors[i].currentValue, sensors[i].critThreshold);
      }
    }
    return true;
  }
  
  else if (type == "flow-anomaly") {
    // Flow rate drop or surge
    bool surge = random(0, 2) == 0;
    for (int i = 0; i < SENSOR_COUNT; i++) {
      if (sensors[i].type == FLOW) {
        if (surge) {
          sensors[i].currentValue = sensors[i].maxThreshold * 1.5;
        } else {
          sensors[i].currentValue = sensors[i].minThreshold * 0.3;
        }
        Serial.printf("[INCIDENT] Flow %s on %s: %.2f L/min\n", 
                      surge ? "surge" : "drop", sensors[i].id, sensors[i].currentValue);
      }
    }
    return true;
  }
  
  else if (type == "vibration-alert") {
    // Excessive vibration
    for (int i = 0; i < SENSOR_COUNT; i++) {
      if (sensors[i].type == VIBRATION) {
        sensors[i].currentValue = sensors[i].maxThreshold * 1.8;
        Serial.printf("[INCIDENT] Excessive vibration on %s: %.2f mm/s\n", 
                      sensors[i].id, sensors[i].currentValue);
        addAlarm(sensors[i].id, sensors[i].line, "HIGH", 
                 sensors[i].currentValue, sensors[i].maxThreshold);
      }
    }
    return true;
  }
  
  else if (type == "power-loss") {
    // Simulate power loss on random line
    int line = random(1, NUM_LINES + 1);
    Serial.printf("[INCIDENT] Power loss on Line %d\n", line);
    for (int i = 0; i < SENSOR_COUNT; i++) {
      if (sensors[i].type == POWER && sensors[i].line == line) {
        sensors[i].currentValue = 0.0;
        sensors[i].faulted = true;
        sensors[i].enabled = false;
        addAlarm(sensors[i].id, sensors[i].line, "CRITICAL", 0.0, sensors[i].minThreshold);
      }
    }
    return true;
  }
  
  // Security incidents (logged but no physical effect)
  else if (type == "failed-login" || type == "unauthorized-access" || 
           type == "injection-attempt" || type == "brute-force" || 
           type == "data-exfil" || type == "privilege-escalation") {
    Serial.printf("[SECURITY] %s incident logged\n", type.c_str());
    // These would normally be logged to security incident log
    return true;
  }
  
  // Other incident types
  else {
    Serial.printf("[INCIDENT] Unknown type '%s', logged only\n", type.c_str());
    return true;
  }
}

// Phase 2: Initialize incident engine with scenario scripts
// void initIncidents();

// Phase 2: Tick the incident scheduler (called from loop)
// void tickIncidents();

// Phase 2: Get active incidents as JSON
// String getActiveIncidentsJSON();

// Phase 2: Acknowledge / resolve an incident
// bool resolveIncident(const char* incidentId);
