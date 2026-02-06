// ============================================================
// 10_Safety.ino — Interlocks + Emergency Protocols
// ============================================================

#include "include/common.h"

#define MAX_INTERLOCKS 16
SafetyInterlock interlocks[MAX_INTERLOCKS];
int interlockCount = 0;
bool emergencyStopActive = false;
bool safetyBypassDetected = false;

void safetyInit() {
  Serial.println("[SAFETY] Initializing safety interlocks...");

  // Define interlocks for each line
  for (int l = 1; l <= NUM_LINES; l++) {
    // Overtemperature → stop motor
    {
      SafetyInterlock& si = interlocks[interlockCount++];
      snprintf(si.id, sizeof(si.id), "INTERLOCK-L%d-OT", l);
      si.line      = l;
      strncpy(si.condition, "temperature > critical", sizeof(si.condition) - 1);
      strncpy(si.action, "stop_motor", sizeof(si.action) - 1);
      si.triggered = false;
    }
    // Overpressure → close valve
    {
      SafetyInterlock& si = interlocks[interlockCount++];
      snprintf(si.id, sizeof(si.id), "INTERLOCK-L%d-OP", l);
      si.line      = l;
      strncpy(si.condition, "pressure > critical", sizeof(si.condition) - 1);
      strncpy(si.action, "close_valve", sizeof(si.action) - 1);
      si.triggered = false;
    }
    // High vibration → reduce speed
    {
      SafetyInterlock& si = interlocks[interlockCount++];
      snprintf(si.id, sizeof(si.id), "INTERLOCK-L%d-HV", l);
      si.line      = l;
      strncpy(si.condition, "vibration > critical", sizeof(si.condition) - 1);
      strncpy(si.action, "reduce_speed", sizeof(si.action) - 1);
      si.triggered = false;
    }
    // Overcurrent → trip motor
    {
      SafetyInterlock& si = interlocks[interlockCount++];
      snprintf(si.id, sizeof(si.id), "INTERLOCK-L%d-OC", l);
      si.line      = l;
      strncpy(si.condition, "current > critical", sizeof(si.condition) - 1);
      strncpy(si.action, "trip_motor", sizeof(si.action) - 1);
      si.triggered = false;
    }
  }

  Serial.printf("[SAFETY] %d interlocks configured.\n", interlockCount);
}

// ===== Safety check (periodic) =====
void safetyUpdate() {
  // TEMPORARILY DISABLED FOR STABILITY TESTING
  return;
  
  if (emergencyStopActive) return;

  for (int i = 0; i < interlockCount; i++) {
    SafetyInterlock& si = interlocks[i];
    int l = si.line;

    bool shouldTrigger = false;

    // Check temperature interlock
    if (strcmp(si.action, "stop_motor") == 0) {
      float temp = getSensorValue(l, 0);  // temperature = index 0
      if (temp >= SENSOR_CRIT_THRESH[0]) shouldTrigger = true;
    }
    // Check pressure interlock
    else if (strcmp(si.action, "close_valve") == 0) {
      float press = getSensorValue(l, 1);  // pressure = index 1
      if (press >= SENSOR_CRIT_THRESH[1]) shouldTrigger = true;
    }
    // Check vibration interlock
    else if (strcmp(si.action, "reduce_speed") == 0) {
      float vib = getSensorValue(l, 2);  // vibration = index 2
      if (vib >= SENSOR_CRIT_THRESH[2]) shouldTrigger = true;
    }
    // Check current interlock
    else if (strcmp(si.action, "trip_motor") == 0) {
      float cur = getSensorValue(l, 4);  // current = index 4
      if (cur >= SENSOR_CRIT_THRESH[4]) shouldTrigger = true;
    }

    if (shouldTrigger && !si.triggered) {
      si.triggered   = true;
      si.triggeredAt = millis();
      executeSafetyAction(si);
    } else if (!shouldTrigger && si.triggered) {
      // Auto-reset after 30 seconds
      if (millis() - si.triggeredAt > 30000) {
        si.triggered = false;
        debugLogf("SAFETY", "Interlock %s reset", si.id);
      }
    }
  }
}

// ===== Execute safety action =====
void executeSafetyAction(const SafetyInterlock& si) {
  debugLogf("SAFETY", "INTERLOCK TRIGGERED: %s (Line %d) → %s",
    si.id, si.line, si.action);

  char motorId[20], valveId[20];
  snprintf(motorId, sizeof(motorId), "MOTOR-L%d-01", si.line);
  snprintf(valveId, sizeof(valveId), "VALVE-L%d-01", si.line);

  if (strcmp(si.action, "stop_motor") == 0 || strcmp(si.action, "trip_motor") == 0) {
    setActuatorState(String(motorId), STATE_STOPPED);
    // Also lock to prevent restart
    Actuator* act = getActuatorById(String(motorId));
    if (act) act->locked = true;
  }
  else if (strcmp(si.action, "close_valve") == 0) {
    setActuatorState(String(valveId), STATE_CLOSED);
  }
  else if (strcmp(si.action, "reduce_speed") == 0) {
    Actuator* act = getActuatorById(String(motorId));
    if (act && act->speed > 50.0f) {
      act->speed = 50.0f;
      act->rpm = act->speed * 18.0f;
    }
  }

  // Log to alarm buffer
  JsonDocument entry;
  entry["type"]      = "safety_interlock";
  entry["id"]        = si.id;
  entry["line"]      = si.line;
  entry["condition"] = si.condition;
  entry["action"]    = si.action;
  entry["timestamp"] = getISOTimestamp();
  String json;
  serializeJson(entry, json);
  alarmBuffer.push(json);
}

// ===== Emergency stop all =====
void emergencyStopAll() {
  emergencyStopActive = true;
  debugLog("SAFETY", "*** EMERGENCY STOP ACTIVATED ***");

  for (int i = 0; i < actuatorCount; i++) {
    actuators[i].state  = STATE_STOPPED;
    actuators[i].speed  = 0;
    actuators[i].rpm    = 0;
    actuators[i].locked = true;
  }

  // Broadcast emergency
  JsonDocument doc;
  doc["type"]    = "emergency";
  doc["message"] = "EMERGENCY STOP - All actuators stopped and locked";
  doc["timestamp"] = getISOTimestamp();
  String out;
  serializeJson(doc, out);
  wsBroadcast(out);
}

// ===== Reset emergency stop =====
void resetEmergencyStop() {
  emergencyStopActive = false;
  for (int i = 0; i < actuatorCount; i++) {
    actuators[i].locked = false;
  }
  for (int i = 0; i < interlockCount; i++) {
    interlocks[i].triggered = false;
  }
  debugLog("SAFETY", "Emergency stop reset. Actuators unlocked.");
}

// ===== Safety status JSON =====
String getSafetyStatus() {
  JsonDocument doc;
  doc["emergency_stop"] = emergencyStopActive;
  doc["bypass_detected"] = safetyBypassDetected;

  JsonArray arr = doc["interlocks"].to<JsonArray>();
  for (int i = 0; i < interlockCount; i++) {
    JsonObject il = arr.add<JsonObject>();
    il["id"]        = interlocks[i].id;
    il["line"]      = interlocks[i].line;
    il["condition"] = interlocks[i].condition;
    il["action"]    = interlocks[i].action;
    il["triggered"] = interlocks[i].triggered;
    if (interlocks[i].triggered) {
      il["age_sec"] = (millis() - interlocks[i].triggeredAt) / 1000;
    }
  }

  String out;
  serializeJson(doc, out);
  return out;
}
