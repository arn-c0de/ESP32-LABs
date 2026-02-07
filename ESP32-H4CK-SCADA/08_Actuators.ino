/*
 * Actuator Control Module
 *
 * State machine for motors and valves across 4 production lines.
 * Includes intentional command injection vulnerability.
 */

void initActuators() {
  for (int line = 0; line < NUM_LINES; line++) {
    int base = line * ACTUATORS_PER_LINE;

    // First actuator per line: MOTOR
    ActuatorData &motor = actuators[base];
    snprintf(motor.id, sizeof(motor.id), "MOTOR-L%d-01", line + 1);
    motor.line = line + 1;
    motor.type = MOTOR;
    motor.state = ACT_RUNNING;  // FIXED: Start motors running for normal production simulation
    motor.speed = 75.0f;        // 75% speed
    motor.targetSpeed = 75.0f;
    motor.stateChangeTime = millis();
    motor.locked = false;

    // Second actuator per line: VALVE
    ActuatorData &valve = actuators[base + 1];
    snprintf(valve.id, sizeof(valve.id), "VALVE-L%d-02", line + 1);
    valve.line = line + 1;
    valve.type = VALVE;
    valve.state = ACT_RUNNING;   // Valves start open
    valve.speed = 100.0f;        // 100% = fully open
    valve.targetSpeed = 100.0f;
    valve.stateChangeTime = millis();
    valve.locked = false;
  }

  Serial.printf("[ACTUATORS] Initialized %d actuators across %d lines\n", TOTAL_ACTUATORS, NUM_LINES);
}

String getActuatorListJSON() {
  static String actuatorListCache;
  static unsigned long actuatorListCacheAt = 0;
  const unsigned long now = millis();
  if (actuatorListCache.length() > 0 && (now - actuatorListCacheAt) < 800) {
    return actuatorListCache;
  }
  if (ESP.getFreeHeap() < 20000) {
    if (actuatorListCache.length() > 0) return actuatorListCache;
    return "{\"error\":\"low memory\"}";
  }
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < TOTAL_ACTUATORS; i++) {
    const ActuatorData &a = actuators[i];
    JsonObject obj = arr.createNestedObject();
    obj["id"] = a.id;
    obj["line"] = a.line;
    obj["type"] = ACTUATOR_TYPE_NAMES[a.type];
    obj["state"] = ACTUATOR_STATE_NAMES[a.state];
    obj["speed"] = a.speed;
    obj["targetSpeed"] = a.targetSpeed;
    obj["locked"] = a.locked;
    // Add motor temperature for MOTOR type
    if (a.type == MOTOR && a.line >= 1 && a.line <= NUM_LINES) {
      obj["motor_temp"] = motorTemp[a.line - 1];
    }
  }

  actuatorListCache = "";
  actuatorListCache.reserve(2048);
  serializeJson(doc, actuatorListCache);
  doc.clear();
  actuatorListCacheAt = now;
  return actuatorListCache;
}

// Find actuator index by ID, returns -1 if not found
static int findActuatorByID(const char* actuatorId) {
  for (int i = 0; i < TOTAL_ACTUATORS; i++) {
    if (strcmp(actuators[i].id, actuatorId) == 0) {
      return i;
    }
  }
  return -1;
}

// Build a JSON result string using snprintf (low memory)
static String makeResult(bool success, const char* message, const ActuatorData &a) {
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{\"success\":%s,\"message\":\"%s\","
    "\"actuator\":{\"id\":\"%s\",\"type\":\"%s\","
    "\"state\":\"%s\",\"speed\":%.1f,\"line\":%d}}",
    success ? "true" : "false",
    message,
    a.id,
    ACTUATOR_TYPE_NAMES[a.type],
    ACTUATOR_STATE_NAMES[a.state],
    a.speed,
    a.line);
  return String(buf);
}

String executeActuatorCommand(const char* actuatorId, const char* cmd, float param) {

  // ===== VULNERABILITY: Command Injection =====
  // If VULN_COMMAND_INJECT is enabled, check if the actuatorId or cmd
  // contains a ";simulate:" payload and return simulated file contents.
  if (VULN_COMMAND_INJECT) {
    // Check actuatorId for injection
    const char* injectA = strstr(actuatorId, ";simulate:");
    const char* injectC = strstr(cmd, ";simulate:");
    const char* payload = injectA ? injectA : injectC;

    if (payload) {
      const char* filename = payload + 10;  // skip ";simulate:"
      // Simulated file system responses (no real FS access)
      char response[512];

      if (strstr(filename, "/etc/passwd") || strstr(filename, "passwd")) {
        snprintf(response, sizeof(response),
          "{\"output\":\"root:x:0:0:root:/root:/bin/bash\\n"
          "scada:x:1000:1000:SCADA Service:/home/scada:/bin/bash\\n"
          "plc:x:1001:1001:PLC Controller:/home/plc:/usr/sbin/nologin\\n"
          "hmi:x:1002:1002:HMI Interface:/home/hmi:/bin/bash\","
          "\"flag\":\"FLAG{cmd_injection_scada_0x41}\"}");
      } else if (strstr(filename, "shadow")) {
        snprintf(response, sizeof(response),
          "{\"output\":\"root:$6$rounds=5000$salt$hash:18000:0:99999:7:::\\n"
          "scada:$6$rounds=5000$weak$hash:18000:0:99999:7:::\","
          "\"flag\":\"FLAG{shadow_file_accessed_0x42}\"}");
      } else if (strstr(filename, "config") || strstr(filename, ".env")) {
        snprintf(response, sizeof(response),
          "{\"output\":\"DB_HOST=192.168.1.100\\n"
          "DB_USER=scada_admin\\n"
          "DB_PASS=Pr0duct10n!\\n"
          "PLC_KEY=A1B2C3D4E5F6\","
          "\"flag\":\"FLAG{config_leak_scada_0x43}\"}");
      } else {
        snprintf(response, sizeof(response),
          "{\"output\":\"cat: %s: simulated access\\n"
          "Contents would appear here in a real system.\","
          "\"flag\":\"FLAG{arbitrary_read_scada_0x44}\"}", filename);
      }

      return String(response);
    }
  }
  // ===== END VULNERABILITY =====

  // Find actuator
  int idx = findActuatorByID(actuatorId);
  if (idx < 0) {
    char buf[128];
    snprintf(buf, sizeof(buf),
      "{\"success\":false,\"message\":\"Actuator '%s' not found\"}", actuatorId);
    return String(buf);
  }

  ActuatorData &a = actuators[idx];

  // Check if locked
  if (a.locked) {
    return makeResult(false, "Actuator is locked by safety system", a);
  }

  unsigned long now = millis();

  // === COMMAND: start ===
  if (strcmp(cmd, "start") == 0) {
    if (a.state == ACT_RUNNING) {
      return makeResult(false, "Already running", a);
    }
    if (a.state == ACT_STARTING) {
      return makeResult(false, "Already starting", a);
    }
    if (a.state == ACT_FAULT) {
      // Check if there's an approved repair request
      bool hasApprovedRepair = false;
      for (int i = 0; i < repairRequestCount && i < MAX_REPAIR_REQUESTS; i++) {
        if (strcmp(repairRequests[i].actuatorId, actuatorId) == 0 &&
            repairRequests[i].status == REQ_APPROVED) {
          hasApprovedRepair = true;
          // Clear fault after approved repair
          a.state = ACT_STOPPED;
          a.stateChangeTime = now;
          break;
        }
      }
      
      if (!hasApprovedRepair) {
        return makeResult(false, "Fault state - repair request required", a);
      }
    }

    a.state = ACT_STARTING;
    a.stateChangeTime = now;

    if (a.type == MOTOR) {
      a.targetSpeed = (param > 0.0f && param <= 100.0f) ? param : 75.0f;
    } else {
      // Valve: start means open (100%)
      a.targetSpeed = 100.0f;
    }

    return makeResult(true, "Starting", a);
  }

  // === COMMAND: stop ===
  if (strcmp(cmd, "stop") == 0) {
    if (a.state == ACT_STOPPED) {
      return makeResult(false, "Already stopped", a);
    }
    if (a.state == ACT_STOPPING) {
      return makeResult(false, "Already stopping", a);
    }

    a.state = ACT_STOPPING;
    a.targetSpeed = 0.0f;
    a.stateChangeTime = now;

    return makeResult(true, "Stopping", a);
  }

  // === COMMAND: open / close (valves) ===
  if (strcmp(cmd, "open") == 0 || strcmp(cmd, "close") == 0) {
    if (a.type != VALVE) {
      return makeResult(false, "Open/close only valid for valves", a);
    }

    float pos = (strcmp(cmd, "open") == 0) ? 100.0f : 0.0f;

    if (pos > 0.0f && a.state != ACT_RUNNING) {
      a.state = ACT_STARTING;
      a.stateChangeTime = now;
    } else if (pos == 0.0f && a.state == ACT_RUNNING) {
      a.state = ACT_STOPPING;
      a.stateChangeTime = now;
    }

    a.targetSpeed = pos;
    if (a.state == ACT_RUNNING) {
      a.speed = pos;
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "Valve %s", pos > 0.0f ? "opening" : "closing");
    return makeResult(true, msg, a);
  }

  // === COMMAND: set (speed/position) ===
  if (strcmp(cmd, "set") == 0) {
    if (a.type == MOTOR) {
      // Motor: speed 0-100
      if (param < 0.0f) param = 0.0f;
      if (param > 100.0f) param = 100.0f;

      if (a.state != ACT_RUNNING && param > 0.0f) {
        // Auto-start if setting speed > 0
        a.state = ACT_STARTING;
        a.stateChangeTime = now;
      } else if (param == 0.0f && a.state == ACT_RUNNING) {
        a.state = ACT_STOPPING;
        a.stateChangeTime = now;
      }

      a.targetSpeed = param;
      if (a.state == ACT_RUNNING) {
        a.speed = param;  // immediate for running motors
      }

      char msg[64];
      snprintf(msg, sizeof(msg), "Speed set to %.1f%%", param);
      return makeResult(true, msg, a);
    } else {
      // Valve: binary 0 (closed) or 100 (open)
      float pos = (param >= 50.0f) ? 100.0f : 0.0f;

      if (pos > 0.0f && a.state != ACT_RUNNING) {
        a.state = ACT_STARTING;
        a.stateChangeTime = now;
      } else if (pos == 0.0f && a.state == ACT_RUNNING) {
        a.state = ACT_STOPPING;
        a.stateChangeTime = now;
      }

      a.targetSpeed = pos;
      if (a.state == ACT_RUNNING) {
        a.speed = pos;
      }

      char msg[64];
      snprintf(msg, sizeof(msg), "Valve %s", pos > 0.0f ? "opening" : "closing");
      return makeResult(true, msg, a);
    }
  }

  // === COMMAND: emergency_stop ===
  if (strcmp(cmd, "emergency_stop") == 0) {
    a.state = ACT_STOPPED;
    a.speed = 0.0f;
    a.targetSpeed = 0.0f;
    a.stateChangeTime = now;

    return makeResult(true, "Emergency stop executed", a);
  }

  // === COMMAND: clear_fault ===
  if (strcmp(cmd, "clear_fault") == 0) {
    if (a.state != ACT_FAULT) {
      return makeResult(false, "No fault to clear", a);
    }
    a.state = ACT_STOPPED;
    a.speed = 0.0f;
    a.targetSpeed = 0.0f;
    a.stateChangeTime = now;

    return makeResult(true, "Fault cleared", a);
  }

  // Unknown command
  char buf[128];
  snprintf(buf, sizeof(buf),
    "{\"success\":false,\"message\":\"Unknown command '%s'\"}", cmd);
  return String(buf);
}
