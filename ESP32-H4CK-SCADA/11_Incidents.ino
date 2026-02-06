// ============================================================
// 11_Incidents.ino — Auto-Incident Generation + Cascade
// ============================================================

#include "include/common.h"

// Incident types and severity are defined in include/common.h


#define MAX_INCIDENTS 20
Incident incidents[MAX_INCIDENTS];
int incidentCount = 0;
int incidentIdCounter = 1;
unsigned long lastIncidentSpawn = 0;

const char* incidentTypeNames[] = {
  "STUCK_VALVE", "SENSOR_FAULT", "MOTOR_OVERLOAD",
  "TEMPERATURE_SPIKE", "PRESSURE_LOSS", "LOSS_OF_SIGNAL",
  "SAFETY_BYPASS_DETECTED"
};

void incidentsInit() {
  Serial.println("[INCIDENTS] Initializing incident engine...");
  memset(incidents, 0, sizeof(incidents));
  incidentCount = 0;
  lastIncidentSpawn = millis();
  Serial.printf("[INCIDENTS] Auto-incidents: %s, interval: %ds\n",
    AUTO_INCIDENTS_ENABLED ? "ON" : "OFF", INCIDENT_SPAWN_INTERVAL_SEC);
}

// ===== Auto-spawn incidents =====
void incidentsUpdate() {
  if (!AUTO_INCIDENTS_ENABLED) return;

  unsigned long now = millis();
  if (now - lastIncidentSpawn < (unsigned long)INCIDENT_SPAWN_INTERVAL_SEC * 1000) return;
  lastIncidentSpawn = now;

  // Count active incidents
  int activeCount = getActiveIncidentCount();
  if (activeCount >= NUM_CONCURRENT_INCIDENTS) return;

  // Spawn random incident
  spawnRandomIncident();
}

// ===== Spawn random incident =====
void spawnRandomIncident() {
  // Pick random type from enabled types
  bool enabled[] = {
    INCIDENT_TYPE_STUCK_VALVE, INCIDENT_TYPE_SENSOR_FAULT,
    INCIDENT_TYPE_MOTOR_OVERLOAD, INCIDENT_TYPE_TEMPERATURE_SPIKE,
    INCIDENT_TYPE_PRESSURE_LOSS, INCIDENT_TYPE_LOSS_OF_SIGNAL,
    INCIDENT_TYPE_SAFETY_BYPASS_DETECTED
  };

  int candidates[7];
  int numCandidates = 0;
  for (int i = 0; i < 7; i++) {
    if (enabled[i]) candidates[numCandidates++] = i;
  }
  if (numCandidates == 0) return;

  int typeIdx = candidates[random(numCandidates)];
  int line = random(1, NUM_LINES + 1);

  createIncident((IncidentType)typeIdx, line, INC_SEV_MEDIUM, 0);
}

// ===== Create incident =====
String createIncident(IncidentType type, int line, IncidentSeverity severity, int cascadeDepth) {
  int slot = -1;
  for (int i = 0; i < MAX_INCIDENTS; i++) {
    if (!incidents[i].active) { slot = i; break; }
  }
  if (slot < 0) return jsonError("Max incidents reached", 500);

  Incident& inc = incidents[slot];
  snprintf(inc.id, sizeof(inc.id), "INC-%03d", incidentIdCounter++);
  inc.type         = type;
  inc.severity     = severity;
  inc.line         = line;
  inc.cascadeDepth = cascadeDepth;
  inc.createdAt    = millis();
  inc.resolvedAt   = 0;
  inc.active       = true;
  inc.resolved     = false;

  // Generate sub-flag
  char flagBuf[64];
  snprintf(flagBuf, sizeof(flagBuf), "FLAG{%s_L%d_%04X}",
    incidentTypeNames[type], line, (uint16_t)esp_random());
  inc.subFlag = String(flagBuf);

  // Apply incident effects
  switch (type) {
    case INC_STUCK_VALVE: {
      char vid[20];
      snprintf(vid, sizeof(vid), "VALVE-L%d-%02d", line, random(1, 3));
      inc.equipment   = String(vid);
      inc.description = "Pneumatic valve stuck in current position";
      setActuatorState(inc.equipment, STATE_STUCK);
      break;
    }
    case INC_SENSOR_FAULT: {
      int sensorIdx = random(0, SENSORS_PER_LINE);
      char sid[20];
      snprintf(sid, sizeof(sid), "SENSOR-L%d-%02d", line, sensorIdx + 1);
      inc.equipment   = String(sid);
      inc.description = String(SENSOR_TYPES[sensorIdx]) + " sensor reads constant value";
      injectSensorFault(line, sensorIdx, 1, 120000);  // Stuck for 2 min
      break;
    }
    case INC_MOTOR_OVERLOAD: {
      char mid[20];
      snprintf(mid, sizeof(mid), "MOTOR-L%d-01", line);
      inc.equipment   = String(mid);
      inc.description = "Motor current exceeds rated limit";
      setPhysicsBase(line, 4, SENSOR_CRIT_THRESH[4] * 1.1f);  // current index=4
      break;
    }
    case INC_TEMP_SPIKE: {
      char sid[20];
      snprintf(sid, sizeof(sid), "SENSOR-L%d-01", line);
      inc.equipment   = String(sid);
      inc.description = "Temperature above critical threshold";
      setPhysicsBase(line, 0, SENSOR_CRIT_THRESH[0] * 1.1f);  // temp index=0
      break;
    }
    case INC_PRESSURE_LOSS: {
      char sid[20];
      snprintf(sid, sizeof(sid), "SENSOR-L%d-02", line);
      inc.equipment   = String(sid);
      inc.description = "System pressure below minimum";
      setPhysicsBase(line, 1, SENSOR_LOW_THRESH[1] * 0.8f);  // pressure index=1
      break;
    }
    case INC_LOSS_OF_SIGNAL: {
      int sensorIdx = random(0, SENSORS_PER_LINE);
      char sid[20];
      snprintf(sid, sizeof(sid), "SENSOR-L%d-%02d", line, sensorIdx + 1);
      inc.equipment   = String(sid);
      inc.description = "Communication lost with field device";
      injectSensorFault(line, sensorIdx, 3, 90000);  // Dropout for 90s
      break;
    }
    case INC_SAFETY_BYPASS: {
      inc.equipment   = "INTERLOCK-L" + String(line);
      inc.description = "Safety interlock bypassed";
      safetyBypassDetected = true;
      break;
    }
  }

  if (slot >= incidentCount) incidentCount = slot + 1;

  // Log to ringbuffer
  JsonDocument entry;
  entry["id"]          = inc.id;
  entry["type"]        = incidentTypeNames[inc.type];
  entry["line"]        = inc.line;
  entry["severity"]    = incSeverityStr(inc.severity);
  entry["equipment"]   = inc.equipment;
  entry["description"] = inc.description;
  entry["timestamp"]   = getISOTimestamp();
  String json;
  serializeJson(entry, json);
  incidentBuffer.push(json);

  debugLogf("INCIDENTS", "Created %s: %s (Line %d) [%s]",
    inc.id, incidentTypeNames[inc.type], line, incSeverityStr(inc.severity));

  // Cascade: spawn sub-incidents
  if (cascadeDepth < INCIDENT_CASCADE_DEPTH && INCIDENT_CASCADE_DEPTH > 0) {
    // Cascade effect: stuck valve → temp spike
    if (type == INC_STUCK_VALVE) {
      createIncident(INC_TEMP_SPIKE, line, INC_SEV_HIGH, cascadeDepth + 1);
    }
    // Motor overload → sensor fault on vibration
    if (type == INC_MOTOR_OVERLOAD) {
      createIncident(INC_SENSOR_FAULT, line, INC_SEV_MEDIUM, cascadeDepth + 1);
    }
  }

  // Broadcast via WebSocket
  JsonDocument wsDoc;
  wsDoc["type"]     = "incident";
  wsDoc["incident"] = entry;
  String wsOut;
  serializeJson(wsDoc, wsOut);
  wsBroadcast(wsOut);

  return jsonSuccess("Incident created: " + String(inc.id));
}

// ===== Submit incident report (RCA) =====
String submitIncidentReport(const String& body) {
  JsonDocument doc;
  if (deserializeJson(doc, body)) return jsonError("Invalid JSON", 400);

  String incId     = doc["incident_id"] | "";
  String diagnosis = doc["diagnosis"] | "";

  for (int i = 0; i < incidentCount; i++) {
    if (String(incidents[i].id) == incId && incidents[i].active) {
      incidents[i].resolved   = true;
      incidents[i].resolvedAt = millis();

      JsonDocument result;
      result["success"]    = true;
      result["incident_id"] = incId;
      result["sub_flag"]   = incidents[i].subFlag;
      result["message"]    = "Incident resolved. Root cause analysis recorded.";
      result["resolution_time_sec"] = (incidents[i].resolvedAt - incidents[i].createdAt) / 1000;

      // Reset physics effects
      resetIncidentEffects(incidents[i]);

      String out;
      serializeJson(result, out);
      return out;
    }
  }

  return jsonError("Incident not found: " + incId, 404);
}

// ===== Reset incident effects =====
void resetIncidentEffects(Incident& inc) {
  switch (inc.type) {
    case INC_STUCK_VALVE:
      setActuatorState(inc.equipment, STATE_OPEN);
      break;
    case INC_TEMP_SPIKE:
      setPhysicsBase(inc.line, 0, SENSOR_NOMINAL[0]);
      break;
    case INC_MOTOR_OVERLOAD:
      setPhysicsBase(inc.line, 4, SENSOR_NOMINAL[4]);
      break;
    case INC_PRESSURE_LOSS:
      setPhysicsBase(inc.line, 1, SENSOR_NOMINAL[1]);
      break;
    case INC_SAFETY_BYPASS:
      safetyBypassDetected = false;
      break;
    default:
      break;
  }
  inc.active = false;
}

// ===== Create manual incident (admin) =====
String createManualIncident(const String& body) {
  JsonDocument doc;
  if (deserializeJson(doc, body)) return jsonError("Invalid JSON", 400);

  String typeStr = doc["type"] | "";
  int line       = doc["line"] | 1;

  IncidentType type = INC_STUCK_VALVE;
  for (int i = 0; i < 7; i++) {
    if (typeStr == incidentTypeNames[i]) { type = (IncidentType)i; break; }
  }

  return createIncident(type, line, INC_SEV_HIGH, 0);
}

// ===== Get active incidents =====
String getActiveIncidents() {
  JsonDocument doc;
  JsonArray arr = doc["incidents"].to<JsonArray>();

  for (int i = 0; i < incidentCount; i++) {
    if (incidents[i].active) {
      JsonObject inc = arr.add<JsonObject>();
      inc["id"]          = incidents[i].id;
      inc["type"]        = incidentTypeNames[incidents[i].type];
      inc["severity"]    = incSeverityStr(incidents[i].severity);
      inc["line"]        = incidents[i].line;
      inc["equipment"]   = incidents[i].equipment;
      inc["description"] = incidents[i].description;
      inc["resolved"]    = incidents[i].resolved;
      inc["age_sec"]     = (millis() - incidents[i].createdAt) / 1000;
    }
  }

  doc["count"] = getActiveIncidentCount();
  String out;
  serializeJson(doc, out);
  return out;
}

int getActiveIncidentCount() {
  int c = 0;
  for (int i = 0; i < incidentCount; i++) {
    if (incidents[i].active && !incidents[i].resolved) c++;
  }
  return c;
}

const char* incSeverityStr(IncidentSeverity sev) {
  switch (sev) {
    case INC_SEV_LOW:      return "LOW";
    case INC_SEV_MEDIUM:   return "MEDIUM";
    case INC_SEV_HIGH:     return "HIGH";
    case INC_SEV_CRITICAL: return "CRITICAL";
    default:               return "UNKNOWN";
  }
}
