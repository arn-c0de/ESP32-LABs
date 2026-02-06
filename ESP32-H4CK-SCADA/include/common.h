#ifndef ESP32_SCADA_COMMON_H
#define ESP32_SCADA_COMMON_H

#include <Arduino.h>

// ===== Shared enums & types used across modules =====
// Exploit paths
enum ExploitPath {
  PATH_IDOR, PATH_INJECTION, PATH_RACE,
  PATH_PHYSICS, PATH_FORENSICS, PATH_WEAK_AUTH,
  PATH_COUNT
};

// User roles
enum UserRole { ROLE_ADMIN, ROLE_OPERATOR, ROLE_MAINTENANCE, ROLE_VIEWER, ROLE_NONE };

// Session (lightweight)
struct Session {
  String    sessionId;
  String    username;
  UserRole  role;
  String    ip;
  unsigned long createdAt;
  unsigned long lastActivity;
  bool      active;
};

// Player state (forward declaration - full definition lives in Gameplay module)
struct PlayerState;

// Incident types
enum IncidentType {
  INC_STUCK_VALVE, INC_SENSOR_FAULT, INC_MOTOR_OVERLOAD,
  INC_TEMP_SPIKE, INC_PRESSURE_LOSS, INC_LOSS_OF_SIGNAL,
  INC_SAFETY_BYPASS
};

enum IncidentSeverity { INC_SEV_LOW, INC_SEV_MEDIUM, INC_SEV_HIGH, INC_SEV_CRITICAL };

// Incident structure (minimal; full layout in incidents module)
struct Incident {
  char           id[16];
  IncidentType   type;
  IncidentSeverity severity;
  int            line;
  String         equipment;
  String         description;
  String         subFlag;
  unsigned long  createdAt;
  unsigned long  resolvedAt;
  bool           active;
  bool           resolved;
  int            cascadeDepth;
};

// Sensor data
struct SensorData {
  char   id[20];
  int    line;
  int    typeIdx;
  float  value;
  float  prevValue;
  String status;
  unsigned long lastUpdate;
};

// Actuator types & state
enum ActuatorType { ACT_MOTOR, ACT_VALVE, ACT_PUMP };
enum ActuatorState { STATE_STOPPED, STATE_RUNNING, STATE_OPEN, STATE_CLOSED, STATE_STUCK, STATE_ERROR };

struct Actuator {
  char         id[20];
  int          line;
  ActuatorType type;
  ActuatorState state;
  float        speed;
  float        rpm;
  float        flow;
  int          commandCount;
  unsigned long lastCommand;
  String       lastCommandResult;
  bool         locked;
};

// Alarm types
enum AlarmSeverity { ALM_LOW, ALM_MEDIUM, ALM_HIGH, ALM_CRITICAL };
struct Alarm {
  char     id[16];
  char     sensorId[20];
  int      line;
  AlarmSeverity severity;
  float    value;
  float    threshold;
  String   message;
  String   status;
  unsigned long triggeredAt;
  unsigned long clearedAt;
  bool     active;
};

// Safety interlock
struct SafetyInterlock {
  char   id[20];
  int    line;
  String condition;
  String action;
  bool   triggered;
  unsigned long triggeredAt;
};

// Shared function prototypes
String createIncident(IncidentType type, int line, IncidentSeverity severity, int cascadeDepth);
const char* incSeverityStr(IncidentSeverity sev);
int calculateScore(PlayerState* player);
const char* roleToString(UserRole role);
void resetIncidentEffects(Incident& inc);

// Externs
extern const char* pathNames[PATH_COUNT];

#endif // ESP32_SCADA_COMMON_H
