/*
 * ESP32-H4CK-SCADA - Industrial SCADA Security Lab
 *
 * A vulnerable SCADA/HMI system for cybersecurity training.
 * Students learn by attacking: 4 production lines, 20+ sensors,
 * 6 independent exploit paths. Physics-based sensor simulation.
 *
 * WARNING: Contains intentional vulnerabilities.
 * Deploy ONLY in isolated lab environments.
 *
 * Author: ESP32-H4CK Lab Project
 * License: Educational Use Only
 */

#define LAB_VERSION "2.0.0"
#define BUILD_DATE "2026-02-06"
#define CODENAME "SCADA-Lab"

// Lab Mode: "testing" / "pentest" / "realism"
// This will be injected by upload.sh from .env
// The actual value is set in 01_Config.ino as a variable (LAB_MODE const char*)
String LAB_MODE_STR = "testing"; // will be overridden in initConfig()

// ===== LIBRARY INCLUDES =====
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>
#include <map>

// ===== WiFi Configuration (set by 01_Config.ino) =====
String WIFI_SSID_STR;
String WIFI_PASSWORD_STR;
String AP_SSID_STR;
String AP_PASSWORD_STR;

// Pin Definitions
#define LED_PIN 13

// Server Configuration
#define HTTP_PORT 80
#define MAX_HTTP_CONNECTIONS 16
#define MAX_AP_CLIENTS 4
#define MIN_FREE_HEAP 35000  // Minimum free heap to accept new connections

// Feature Flags
bool ENABLE_VULNERABILITIES = true;
bool DEBUG_MODE = true;
bool PROTECT_ADMIN_ENDPOINTS = true;
bool SSL_ENABLED = false;  // No SSL in lab environment
#ifndef STATION_MODE_DEFAULT
#define STATION_MODE_DEFAULT false
#endif
bool STATION_MODE = STATION_MODE_DEFAULT;

// Timing
#define WIFI_CHECK_INTERVAL 30000
#define SESSION_TIMEOUT 3600000
#define PHYSICS_UPDATE_INTERVAL 2000
#define SENSOR_SAVE_INTERVAL 60000

// Security (set by 01_Config.ino)
String JWT_SECRET_STR;
#define JWT_EXPIRY 86400
#define SESSION_ID_LENGTH 16

// Database
#define DB_FILE_PATH "/db/users.json"
#define SENSORS_DB_PATH "/db/sensors.json"
#define ACTUATORS_DB_PATH "/db/actuators.json"
#define ALARMS_DB_PATH "/db/alarms.json"
#define TX_FILE_PATH "/db/transactions.json"
#define LOG_FILE_PATH "/logs/access.log"
#define MAX_LOG_SIZE 102400
#define MAX_TRANSACTIONS 1000

// ===== SCADA Configuration =====
// Difficulty
enum DifficultyLevel { EASY, NORMAL, HARD };
DifficultyLevel DIFFICULTY = NORMAL;

// Hints
int HINTS_LEVEL = 2;

// Exploit toggles
bool VULN_IDOR_SENSORS = true;
bool VULN_COMMAND_INJECT = true;
bool VULN_RACE_ACTUATORS = true;
bool VULN_WEAK_AUTH = true;
bool VULN_HARDCODED_SECRETS = true;
bool VULN_LOGIC_FLAWS = true;

// Sensor Physics
float SENSOR_NOISE_AMPLITUDE = 0.03;  // 3% noise (proportional to sensor value)
float SENSOR_DRIFT_RATE = 0.01;
bool ENABLE_SENSOR_FAULTS = true;
int FAULT_PROBABILITY_PERCENT = 0.2;  // 0.2% = ~1 fault per 10-20 minutes (much more realistic)

// Production Lines
#define NUM_LINES 4
#define SENSORS_PER_LINE 5
#define ACTUATORS_PER_LINE 2
#define TOTAL_SENSORS (NUM_LINES * SENSORS_PER_LINE)
#define TOTAL_ACTUATORS (NUM_LINES * ACTUATORS_PER_LINE)
#define SENSOR_COUNT TOTAL_SENSORS
#define ACTUATOR_COUNT TOTAL_ACTUATORS

// Sensor Types
enum SensorType { TEMP, PRESSURE, FLOW, VIBRATION, POWER };
const char* SENSOR_TYPE_NAMES[] = {"temperature", "pressure", "flow", "vibration", "power"};
const char* SENSOR_UNITS[] = {"C", "bar", "L/min", "mm/s", "kW"};

// Actuator Types
enum ActuatorType { MOTOR, VALVE };
const char* ACTUATOR_TYPE_NAMES[] = {"motor", "valve"};

// Actuator States
enum ActuatorState { ACT_STOPPED, ACT_STARTING, ACT_RUNNING, ACT_STOPPING, ACT_FAULT };
const char* ACTUATOR_STATE_NAMES[] = {"STOPPED", "STARTING", "RUNNING", "STOPPING", "FAULT"};

// Sensor data structure (in-memory)
struct SensorData {
  char id[20];        // e.g. "SENSOR-L1-01"
  int line;           // 1-4
  SensorType type;
  float currentValue;
  float baseValue;    // nominal value
  float minThreshold;
  float maxThreshold;
  float critThreshold;
  bool faulted;
  bool enabled;       // sensor can be disabled via UI
  unsigned long lastUpdate;
};

// Actuator data structure
struct ActuatorData {
  char id[20];        // e.g. "MOTOR-L1-01"
  int line;
  ActuatorType type;
  ActuatorState state;
  float speed;        // 0-100% for motors, 0/100 for valves
  float targetSpeed;
  unsigned long stateChangeTime;
  bool locked;
};

// Sensor history ring buffer
#define SENSOR_HISTORY_SIZE 30
struct SensorHistory {
  float values[SENSOR_HISTORY_SIZE];
  unsigned long timestamps[SENSOR_HISTORY_SIZE];
  int writeIndex;
  int count;
};

// Alarm structure
#define MAX_ALARMS 30
struct AlarmEntry {
  char sensorId[20];
  int line;
  char level[10];     // LOW, MEDIUM, HIGH, CRITICAL
  float value;
  float threshold;
  unsigned long timestamp;
  bool acknowledged;
};

// WiFi Client History (for AP mode)
#define MAX_WIFI_CLIENTS 50
struct WiFiClientEntry {
  char ip[16];
  char mac[18];
  unsigned long connectedTime;
  unsigned long lastSeen;
};

// Global Objects
AsyncWebServer server(HTTP_PORT);
Preferences preferences;

// Runtime State
unsigned long systemStartTime = 0;
int totalRequests = 0;
int failedRequests = 0;
bool setupComplete = false;
unsigned long lastPhysicsUpdate = 0;
unsigned long lastSensorSave = 0;
int failedLoginAttempts = 0;
unsigned long lastFailedLogin = 0;
int activeConnections = 0;

// SCADA Data (in-memory)
SensorData sensors[TOTAL_SENSORS];
SensorHistory sensorHistory[TOTAL_SENSORS];
ActuatorData actuators[TOTAL_ACTUATORS];
AlarmEntry alarms[MAX_ALARMS];
WiFiClientEntry wifiClients[MAX_WIFI_CLIENTS];
int wifiClientCount = 0;
int alarmCount = 0;

// Session Storage
struct Session {
  String sessionId;
  String username;
  String role;
  unsigned long createdAt;
  unsigned long lastActivity;
  String ipAddress;
};
std::map<String, Session> activeSessions;

// Default Users (SCADA roles)
struct DefaultUser {
  String username;
  String password;
  String role;
  String email;
};

DefaultUser defaultUsers[] = {
  {"admin", "admin", "admin", "admin@scada-lab.local"},
  {"operator", "operator123", "operator", "operator@scada-lab.local"},
  {"maintenance", "maint456", "maintenance", "maintenance@scada-lab.local"},
  {"viewer", "viewer", "viewer", "viewer@scada-lab.local"}
};
const int DEFAULT_USERS_COUNT = 4;

// Vulnerability flags (keep some from SNS for compatibility)
bool VULN_SQL_INJECTION = false;
bool VULN_XSS = false;
bool VULN_PATH_TRAVERSAL = false;
bool VULN_COMMAND_INJECTION = true;
bool VULN_CSRF = false;
bool VULN_INFO_DISCLOSURE = true;
bool VULN_INSECURE_DESERIALIZATION = false;
bool VULN_IDOR = true;

// ===== DEFENSE SYSTEM =====
enum DefenseType {
  DEFENSE_NONE = 0,
  DEFENSE_IP_BLOCK,
  DEFENSE_RATE_LIMIT,
  DEFENSE_SESSION_RESET,
  DEFENSE_MISTRUST_MODE
};

struct DefenseCost {
  int dp;
  int ap;
  int ss;
};

struct DefenseRule {
  String id;
  String requestId;
  DefenseType type;
  String target;
  unsigned long createdAt;
  unsigned long expiresAt;
  int cost_dp;
  int cost_ap;
  bool active;
  String metadata;
};

struct DefenseConfig {
  int max_dp;
  int max_ap;
  int max_stability;
  int current_dp;
  int current_ap;
  int current_stability;
  DefenseCost ipblock_cost;
  DefenseCost ratelimit_cost;
  DefenseCost sessionreset_cost;
  DefenseCost mistrust_cost;
  int ipblock_cooldown;
  int ratelimit_cooldown;
  int sessionreset_cooldown;
  int mistrust_cooldown;
  unsigned long ipblock_last;
  unsigned long ratelimit_last;
  unsigned long sessionreset_last;
  unsigned long mistrust_last;
};

// Defense global variables
DefenseConfig defenseConfig;
DefenseRule activeRules[32];
int activeRuleCount = 0;

// ===== FORWARD DECLARATIONS =====
// Config
void initConfig();
void loadConfigFromFS();
void saveConfigToFS();

// WiFi
void initWiFi();
void connectWiFi();
void startAccessPoint();
void checkWiFiConnection();
String getLocalIP();

// WebServer
void initWebServer();
void setupRoutes();
void serveStaticFiles();
void handleNotFound(AsyncWebServerRequest *request);

// Auth
void initAuth();
bool authenticateUser(String username, String password);
String generateJWT(String username, String role);
bool validateJWT(String token);
bool isAuthenticated(AsyncWebServerRequest *request);
bool isAdmin(AsyncWebServerRequest *request);
bool requireAdmin(AsyncWebServerRequest *request);
bool requireRole(AsyncWebServerRequest *request, const char* minRole);
String getUserRole(String token);
String getRequestRole(AsyncWebServerRequest *request);
String getRequestUsername(AsyncWebServerRequest *request);
void handleLogin(AsyncWebServerRequest *request);
void handleLoginJSON(AsyncWebServerRequest *request, uint8_t *data, size_t len);
void handleLogout(AsyncWebServerRequest *request);

// Database
void initDatabase();
void createUserTable();
bool insertUser(String username, String password, String role);
String getUserByUsername(String username);
String getAllUsers();
void seedTestData();

// Physics
void initPhysics();
void updatePhysics();

// Sensors
void initSensors();
String getSensorListJSON();
String getSensorReadingJSON(const char* sensorId, int limit);
String getDashboardStatusJSON();

// Actuators
void initActuators();
String getActuatorListJSON();
String executeActuatorCommand(const char* actuatorId, const char* cmd, float param);

// Alarms
void checkAlarms();
String getAlarmHistoryJSON(int line, int limit);
void addAlarm(const char* sensorId, int line, const char* level, float value, float threshold);

// API
void setupSCADARoutes();

// Defense
void initDefense();
void tickDefenseResources();
void tickDefenseRules();
bool isIpBlocked(String ip);
bool checkRateLimit(String ip);
String handleDefenseLine(String line);
String handleDefenseStatus();

// Crypto
String hashPassword(String password);
bool verifyPassword(String password, String hash);
String generateRandomToken(int length);
String base64Encode(String input);
String base64Decode(String input);
String sha256Hash(String input);

// Utils
String urlDecode(String input);
String getContentType(String filename);
bool fileExists(String path);
String readFile(String path);
bool writeFile(String path, String content);
String getCurrentTimestamp();
String generateUUID();

// Debug
void initDebug();
void logInfo(String message);
void logError(String message);
void logDebug(String message);
void logWarning(String message);
void printSystemInfo();
void printMemoryUsage();
void printWiFiInfo();
void handleSerialCommands();
void trackConnectedClients();

// Incidents
bool triggerIncident(String type, String details);

// ===== ARDUINO ENTRY POINTS =====

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.printf("  ESP32-H4CK-SCADA v%s\n", LAB_VERSION);
  Serial.printf("  %s Edition\n", CODENAME);
  Serial.printf("  Build: %s\n", BUILD_DATE);
  Serial.println("  Industrial SCADA Security Lab");
  Serial.println("  ** Intentionally Vulnerable **");
  Serial.println("========================================");
  Serial.println();

  initDebug();
  logInfo("Starting initialization...");

  initConfig();
  logInfo("Configuration loaded");

  if (!LittleFS.begin(false)) {
    logError("LittleFS Mount Failed - trying format...");
    if (!LittleFS.begin(true)) {
      logError("LittleFS Format Failed - upload filesystem with ./upload.sh");
    } else {
      logInfo("Filesystem formatted successfully");
    }
  } else {
    logInfo("Filesystem mounted");
  }

  initDatabase();
  logInfo("Database initialized");

  initAuth();
  logInfo("Auth system ready");

  initWiFi();
  logInfo("WiFi initialized");
  yield();

  initPhysics();
  logInfo("Physics engine initialized");

  initSensors();
  logInfo("Sensors initialized");

  initActuators();
  logInfo("Actuators initialized");

  initWebServer();
  logInfo("Web server started");
  yield();

  setupSCADARoutes();
  logInfo("SCADA API routes configured");
  yield();

  initDefense();
  logInfo("Defense system initialized");
  yield();

  Serial.println();
  Serial.println("========================================");
  Serial.println("  SCADA System Ready!");
  Serial.println("========================================");
  printSystemInfo();
  printWiFiInfo();
  printMemoryUsage();
  Serial.println("========================================");

  setupComplete = true;
}

void loop() {
  // Handle serial commands
  handleSerialCommands();

  // Check WiFi
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    checkWiFiConnection();
    trackConnectedClients();  // Track connected WiFi clients in AP mode
    lastWiFiCheck = millis();
  }

  // Update physics simulation
  if (millis() - lastPhysicsUpdate > PHYSICS_UPDATE_INTERVAL) {
    updatePhysics();
    checkAlarms();
    lastPhysicsUpdate = millis();
  }

  // Defense ticks
  tickDefenseResources();
  tickDefenseRules();

  // Memory monitoring and cleanup
  static unsigned long lastMemCheck = 0;
  if (millis() - lastMemCheck > 10000) {
    uint32_t freeHeap = ESP.getFreeHeap();
    
    // Log memory status periodically
    if (freeHeap < 50000) {
      Serial.printf("[MEMORY] Free: %d bytes, Largest block: %d bytes\n", 
                    freeHeap, ESP.getMaxAllocHeap());
    }
    
    // Critical memory - restart
    if (freeHeap < 25000) {
      Serial.printf("[CRITICAL] Low memory: %d bytes. Restarting...\n", freeHeap);
      delay(1000);
      ESP.restart();
    }
    
    // Reset activeConnections counter periodically to prevent drift
    if (activeConnections > 0) {
      activeConnections = 0;  // Reset for clean state
    }
    
    lastMemCheck = millis();
  }

  yield();
  delay(10);
}
