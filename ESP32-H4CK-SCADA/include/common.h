#ifndef ESP32_SCADA_COMMON_H
#define ESP32_SCADA_COMMON_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Forward declarations
class AsyncWebServerRequest;

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
  char   status[16];
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
  char     message[128];
  char     status[16];
  unsigned long triggeredAt;
  unsigned long clearedAt;
  bool     active;
};

// Safety interlock
struct SafetyInterlock {
  char   id[20];
  int    line;
  char   condition[64];
  char   action[32];
  bool   triggered;
  unsigned long triggeredAt;
};

// Shared function prototypes
String createIncident(IncidentType type, int line, IncidentSeverity severity, int cascadeDepth);
const char* incSeverityStr(IncidentSeverity sev);
int calculateScore(PlayerState* player);
const char* roleToString(UserRole role);
void resetIncidentEffects(Incident& inc);

// ===== MODULE FUNCTION PROTOTYPES =====

// 01_Config.ino
void configInit();
String configGetJson();
bool configUpdateFromJson(const String& json);

// 02_WiFi.ino
void wifiInit();
String wifiGetIP();
int wifiGetClients();
String wifiStatusJson();

// 03_WebServer.ino
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
void wsBroadcast(const String& json);
String getClientIP(AsyncWebServerRequest* request);
void addCorsHeaders(AsyncWebServerResponse* response);
void sendJson(AsyncWebServerRequest* request, int code, const String& json);
bool checkDefense(AsyncWebServerRequest* request);
bool requireRole(AsyncWebServerRequest* request, UserRole minRole);
void setupAPIRoutes();
void setupStaticFiles();
void webServerInit();

// 04_Auth.ino
void authInit();
Session* getRequestSession(AsyncWebServerRequest* request);
UserRole getRequestRole(AsyncWebServerRequest* request);
UserRole authenticateUser(const String& username, const String& password);
String createSession(const String& username, UserRole role, const String& ip);
void destroySession(const String& sessionId);
void destroySessionsByIP(const String& ip);
String generateJWT(const String& username, UserRole role);
bool validateJWT(const String& token, String& username, UserRole& role);
String authStatusJson();

// 05_Database.ino
String dbReadFile(const char* path);
bool dbWriteFile(const char* path, const String& content);
bool dbAppendFile(const char* path, const String& content);
bool dbFileExists(const char* path);
void dbCreateSeedData();
void databaseInit();
void databaseAutoSave();
String dbGetEquipment();
String dbGetSensors();
String dbGetActuators();
String dbGetAlarms();
String dbGetMaintenance();
String dbGetIncidents();
String dbGetLogs();
void dbSaveEquipment(const String& json);
void dbSaveSensors(const String& json);
void dbSaveActuators(const String& json);
void dbSaveAlarms(const String& json);
void dbSaveIncidents(const String& json);
void dbSaveLogs(const String& json);

// 06_Physics.ino
void physicsInit();
float applyDrift(int idx);
float applyNoise(int sensorType);
float applyFault(int idx, float normalValue);
float applyCrossCorrelation(int line, int sensorType, float motorSpeedPercent);
void physicsUpdate();
float getPhysicsValue(int line, int sensorType);
bool isSensorFaulted(int line, int sensorType);
String getSensorFaultInfo(int line, int sensorType);
void injectSensorFault(int line, int sensorType, int faultType, int durationMs);
void setPhysicsBase(int line, int sensorType, float newBase);
float gaussianNoise(float mean, float stddev);
float getLineMotorSpeed(int line);

// 07_Sensors.ino
void sensorsInit();
void sensorsUpdate();
void wsBroadcastSensorData();
String getSensorReadings(const String& sensorId, int limit);
float getSensorValue(int line, int sensorType);
String getDashboardStatus();
String getLineStatus(int line);

// 08_Actuators.ino
void actuatorsInit();
String executeActuatorCommand(const String& actuatorId, const String& cmd, JsonVariant params);
String simulateCommand(const String& cmd);
String triggerRaceCondition(const String& actuatorId, int count);
const char* stateToString(ActuatorState state);
void actuatorsUpdate();
void setActuatorState(const String& id, ActuatorState state);

// 09_Alarms.ino
void alarmsInit();
void alarmsUpdate();
void triggerAlarm(const char* sensorId, int line, AlarmSeverity severity, float value, float threshold, const char* message);
void clearAlarmForSensor(const char* sensorId);
bool acknowledgeAlarm(const String& alarmId);
String getActiveAlarms();
String getAlarmHistory(const String& line, int limit);
int getActiveAlarmCount();
const char* severityToString(AlarmSeverity sev);
void wsBroadcastAlarms();

// 10_Safety.ino
void safetyInit();
void safetyUpdate();
void executeSafetyAction(const SafetyInterlock& si);
void emergencyStopAll();
void resetEmergencyStop();
String getSafetyStatus();

// 11_Incidents.ino
void incidentsInit();
void incidentsUpdate();
void spawnRandomIncident();
String submitIncidentReport(const String& body);
String createManualIncident(const String& body);
String getActiveIncidents();
int getActiveIncidentCount();

// 12_Defense.ino
void defenseInit();
void defenseUpdate();
bool isIPBlocked(const String& ip);
void blockIP(const String& ip, int durationSec);
void unblockIP(const String& ip);
bool checkRateLimit(const String& ip);
bool wafCheck(const String& input);
void logDefenseEvent(const String& type, const String& ip, const String& details);
String getDefenseStatus();
String getDefenseAlerts();
void setGlobalRateLimit(int reqPerMin);
void resetSessionsForIP(const String& ip, const String& reason);

// 13_Vulnerabilities.ino
void vulnerabilitiesInit();
void setupVulnRoutes();

// 14_Gameplay.ino
void gameplayInit();
void gameplayRegisterPlayer(const String& sessionId, const String& username, UserRole role);
String submitFlag(const String& body, const String& sessionId, const String& ip);
String submitRootFlag(const String& body);
String getPlayerProgress(const String& sessionId);
void gameplayUpdate();

// 15_API_Hints.ino
void hintsInit();
String getHint(const String& endpoint, int requestedLevel, const String& sessionId);

// 16_Leaderboard.ino
void leaderboardInit();
void leaderboardUpdate();
String getLeaderboard(const String& sortBy, int limit);
void serialCommandHandler();

// 17_Utils.ino
String hmacSHA256(const String& message, const String& key);
String sha256Hash(const String& input);
String base64Encode(const String& input);
String base64Decode(const String& input);
float randomFloat(float minVal, float maxVal);
String getTimestamp();
String getISOTimestamp();
String generateId(const char* prefix);
String generateSessionId();
String urlDecode(const String& str);
int stringSplit(const String& input, char delimiter, String* results, int maxResults);
String jsonError(const String& message, int code);
String jsonSuccess(const String& message);
float clampf(float val, float minVal, float maxVal);
float lerpf(float a, float b, float t);
void debugLog(const char* module, const char* message);
void debugLogf(const char* module, const char* fmt, ...);

// Externs
extern const char* pathNames[PATH_COUNT];

#endif // ESP32_SCADA_COMMON_H
