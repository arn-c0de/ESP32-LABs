// ============================================================
// 08_Actuators.ino — Command Execution + State Machine
// Motors, Valves, Pumps across 4 production lines
// ============================================================

#include "include/common.h"

#define MAX_ACTUATORS 20  // 4 lines × (2 motors + 2 valves + 1 pump)
Actuator actuators[MAX_ACTUATORS];
int actuatorCount = 0;

void actuatorsInit() {
  Serial.println("[ACTUATORS] Initializing actuators...");
  actuatorCount = 0;

  for (int l = 1; l <= NUM_LINES; l++) {
    // 2 Motors per line
    for (int m = 1; m <= 2; m++) {
      Actuator& a = actuators[actuatorCount++];
      snprintf(a.id, sizeof(a.id), "MOTOR-L%d-%02d", l, m);
      a.line  = l;
      a.type  = ACT_MOTOR;
      a.state = (l == 3) ? STATE_STOPPED : STATE_RUNNING;
      a.speed = (l == 3) ? 0.0f : 75.0f;
      a.rpm   = (l == 3) ? 0.0f : 1500.0f;
      a.flow  = 0;
      a.commandCount = 0;
      a.lastCommand  = 0;
      a.locked = false;
    }
    // 2 Valves per line
    for (int v = 1; v <= 2; v++) {
      Actuator& a = actuators[actuatorCount++];
      snprintf(a.id, sizeof(a.id), "VALVE-L%d-%02d", l, v);
      a.line  = l;
      a.type  = ACT_VALVE;
      a.state = STATE_OPEN;
      a.speed = 0;
      a.rpm   = 0;
      a.flow  = 0;
      a.commandCount = 0;
      a.lastCommand  = 0;
      a.locked = false;
    }
    // 1 Pump per line
    {
      Actuator& a = actuators[actuatorCount++];
      snprintf(a.id, sizeof(a.id), "PUMP-L%d-01", l);
      a.line  = l;
      a.type  = ACT_PUMP;
      a.state = (l == 3) ? STATE_STOPPED : STATE_RUNNING;
      a.speed = (l == 3) ? 0.0f : 80.0f;
      a.rpm   = 0;
      a.flow  = (l == 3) ? 0.0f : 120.0f;
      a.commandCount = 0;
      a.lastCommand  = 0;
      a.locked = false;
    }
  }

  Serial.printf("[ACTUATORS] %d actuators initialized.\n", actuatorCount);
}

// ===== Find actuator by ID =====
Actuator* getActuatorById(const String& id) {
  for (int i = 0; i < actuatorCount; i++) {
    if (String(actuators[i].id) == id) return &actuators[i];
  }
  return nullptr;
}

// ===== Get motor speed for a line (for physics cross-correlation) =====
float getLineMotorSpeed(int line) {
  for (int i = 0; i < actuatorCount; i++) {
    if (actuators[i].line == line && actuators[i].type == ACT_MOTOR) {
      return actuators[i].speed;
    }
  }
  return 0.0f;
}

// ===== Execute actuator command =====
String executeActuatorCommand(const String& actuatorId, const String& cmd,
                              JsonVariant params) {
  Actuator* act = getActuatorById(actuatorId);
  if (!act) return jsonError("Actuator not found: " + actuatorId, 404);

  if (act->locked) return jsonError("Actuator locked by safety interlock", 423);

  act->commandCount++;
  act->lastCommand = millis();

  JsonDocument result;
  result["actuator_id"] = actuatorId;
  result["command"]     = cmd;
  result["timestamp"]   = getISOTimestamp();

  // Command Injection vulnerability (Path 2)
  if (VULN_COMMAND_INJECT && params.is<JsonObject>()) {
    String speedStr = params["speed"] | "";

    // Check for injection pattern: "value;simulate:command"
    int injectIdx = speedStr.indexOf(";simulate:");
    if (injectIdx >= 0) {
      String simCmd = speedStr.substring(injectIdx + 10);
      debugLogf("VULN", "Command injection detected: %s", simCmd.c_str());

      logDefenseEvent("COMMAND_INJECTION", "", "Injected: " + simCmd);

      // Simulate command execution (no real shell!)
      String simResult = simulateCommand(simCmd);
      result["sim_output"]    = simResult;
      result["_debug"]        = "Command executed in simulation sandbox";
      result["vulnerability"] = "COMMAND_INJECTION";

      // Extract actual speed value
      speedStr = speedStr.substring(0, injectIdx);
    }

    // Apply speed if valid
    float newSpeed = speedStr.toFloat();
    if (newSpeed >= 0 && newSpeed <= 100) {
      act->speed = newSpeed;
      act->rpm   = newSpeed * 18.0f;  // RPM = speed% × 18
    }
  }

  if (cmd == "start") {
    act->state = STATE_RUNNING;
    act->speed = params["speed"] | 75.0f;
    act->rpm   = act->speed * 18.0f;
    result["status"] = "started";
  } else if (cmd == "stop") {
    act->state = STATE_STOPPED;
    act->speed = 0;
    act->rpm   = 0;
    result["status"] = "stopped";
  } else if (cmd == "set") {
    if (act->type == ACT_VALVE) {
      String pos = params["position"] | "open";
      act->state = (pos == "open") ? STATE_OPEN : STATE_CLOSED;
      result["state"] = (act->state == STATE_OPEN) ? "open" : "closed";
    }
    result["status"] = "updated";
  } else if (cmd == "emergency_stop") {
    act->state = STATE_STOPPED;
    act->speed = 0;
    act->rpm   = 0;
    act->locked = true;
    result["status"] = "emergency_stopped";
    result["locked"] = true;
  } else {
    result["error"] = "Unknown command: " + cmd;
  }

  result["current_state"]  = stateToString(act->state);
  result["current_speed"]  = act->speed;
  result["command_count"]  = act->commandCount;

  act->lastCommandResult = cmd;

  String out;
  serializeJson(result, out);
  return out;
}

// ===== Simulate shell command (for injection vuln) =====
String simulateCommand(const String& cmd) {
  // Read simulated files from LittleFS
  if (cmd.startsWith("cat ")) {
    String path = cmd.substring(4);
    path.trim();
    // Map simulated paths to LittleFS paths
    if (path.indexOf("maintenance") >= 0) {
      return dbGetMaintenance();
    }
    if (path.indexOf("equipment") >= 0) {
      return dbGetEquipment();
    }
    if (path.indexOf("sensors") >= 0) {
      return dbGetSensors();
    }
    if (path.indexOf("logs") >= 0) {
      return dbGetLogs();
    }
    return "cat: " + path + ": No such file. FLAG{command_injection_cat_" +
           sha256Hash(path).substring(0, 8) + "}";
  }

  if (cmd == "ls" || cmd.startsWith("ls ")) {
    return "equipment.json\nsensors.json\nactuators.json\nalarms.json\n"
           "maintenance.json\nincidents.json\nlogs.json\n"
           "FLAG{command_injection_ls_enumeration}";
  }

  if (cmd == "whoami") {
    return "scada-operator\nFLAG{command_injection_whoami}";
  }

  if (cmd == "id") {
    return "uid=1000(scada) gid=1000(scada) groups=1000(scada),27(operator)";
  }

  if (cmd == "env" || cmd == "printenv") {
    return "SCADA_SECRET=" + String(SECRET_KEY) + "\n"
           "JWT_KEY=" + String(JWT_SECRET) + "\n"
           "FLAG{command_injection_env_leak}";
  }

  return "simulate: command not found: " + cmd;
}

// ===== Race condition trigger =====
String triggerRaceCondition(const String& actuatorId, int count) {
  Actuator* act = getActuatorById(actuatorId);
  if (!act) return jsonError("Actuator not found", 404);

  JsonDocument doc;
  doc["actuator"]  = actuatorId;
  doc["commands"]  = count;
  doc["vulnerability"] = "RACE_CONDITION";

  // Simulate rapid concurrent state changes
  String stateLog = "";
  for (int i = 0; i < count; i++) {
    // Toggle state rapidly
    if (act->state == STATE_RUNNING) {
      act->state = STATE_STOPPED;
    } else {
      act->state = STATE_RUNNING;
    }
    act->commandCount++;

    // Intentional race: on specific iteration, corrupt state
    if (i == count / 2) {
      act->lastCommandResult = "RUNNING;FLAG{race_condition_" +
        String(actuatorId) + "_corrupted}";
    }
  }

  doc["final_state"]    = stateToString(act->state);
  doc["corrupted_field"] = act->lastCommandResult;
  doc["total_commands"]  = act->commandCount;
  doc["warning"]         = "State may be corrupted due to concurrent access";

  String out;
  serializeJson(doc, out);
  return out;
}

// ===== State to string =====
const char* stateToString(ActuatorState state) {
  switch (state) {
    case STATE_STOPPED: return "stopped";
    case STATE_RUNNING: return "running";
    case STATE_OPEN:    return "open";
    case STATE_CLOSED:  return "closed";
    case STATE_STUCK:   return "stuck";
    case STATE_ERROR:   return "error";
    default:            return "unknown";
  }
}

// ===== Update actuators (periodic maintenance) =====
void actuatorsUpdate() {
  for (int i = 0; i < actuatorCount; i++) {
    Actuator& a = actuators[i];

    // Update RPM based on speed for motors
    if (a.type == ACT_MOTOR && a.state == STATE_RUNNING) {
      a.rpm = a.speed * 18.0f + randomFloat(-10.0f, 10.0f);
    }

    // Update flow for pumps
    if (a.type == ACT_PUMP && a.state == STATE_RUNNING) {
      a.flow = a.speed * 1.5f + randomFloat(-5.0f, 5.0f);
    }
  }
}

// ===== Set actuator state (for incident engine) =====
void setActuatorState(const String& id, ActuatorState state) {
  Actuator* act = getActuatorById(id);
  if (act) {
    act->state = state;
    if (state == STATE_STOPPED) { act->speed = 0; act->rpm = 0; }
  }
}
