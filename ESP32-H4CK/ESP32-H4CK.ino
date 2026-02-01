/*
 * ESP32-H4CK - Vulnerable Web Application Lab
 * 
 * An intentionally vulnerable ESP32-based web application designed for
 * cybersecurity training and penetration testing education.
 * 
 * WARNING: This device contains intentional security vulnerabilities.
 * Deploy ONLY in isolated lab environments. DO NOT expose to production networks.
 * 
 * Features:
 * - HTTPS Web Server with Admin/Guest roles
 * - REST API with multiple endpoints
 * - WebSocket-based interactive shell
 * - Telnet service
 * - Multiple intentional vulnerabilities (SQLi, XSS, CSRF, etc.)
 * - Multi-port service hosting
 * 
 * Author: ESP32-H4CK Lab Project
 * License: Educational Use Only
 * Version: 1.0.0
 */

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
#include <map>

// ===== GLOBAL CONFIGURATION & CONSTANTS =====
// (Must be declared before forward declarations for proper compilation order)

// WiFi Configuration
String WIFI_SSID = "ESP32-Lab";
String WIFI_PASSWORD = "hacklab123";
String AP_SSID = "ESP32-H4CK-AP";
String AP_PASSWORD = "vulnerable";

// Pin Definitions
#define LED_PIN 13
#define USER_BUTTON_PIN 14

// Server Configuration
#define HTTPS_PORT 443
#define HTTP_PORT 80
#define TELNET_PORT 23
#define WEBSOCKET_PORT 8080
#define API_PORT 8443
#define DEBUG_PORT 9000

// Feature Flags
bool ENABLE_VULNERABILITIES = true;
bool DEBUG_MODE = true;
bool SSL_ENABLED = false;
bool ENABLE_TELNET = true;
bool ENABLE_WEBSOCKET = true;
bool STATION_MODE = true;

// Timing Constants
#define WIFI_CHECK_INTERVAL 30000
#define SESSION_TIMEOUT 3600000
#define MAX_LOGIN_ATTEMPTS 5
#define RATE_LIMIT_WINDOW 60000

// Security Constants
String JWT_SECRET = "weak_secret_key_123";
#define JWT_EXPIRY 86400
#define SESSION_ID_LENGTH 16
#define PASSWORD_MIN_LENGTH 4

// Database Configuration
#define DB_FILE_PATH "/db/users.json"
#define LOG_FILE_PATH "/logs/access.log"
#define MAX_LOG_SIZE 102400

// Global Server Objects
AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/shell");
WiFiServer telnetServer(TELNET_PORT);
Preferences preferences;

// Runtime State
unsigned long systemStartTime = 0;
int activeConnections = 0;
int totalRequests = 0;
int failedLoginAttempts = 0;
unsigned long lastFailedLogin = 0;

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

// Default Credentials
struct DefaultUser {
  String username;
  String password;
  String role;
};

DefaultUser defaultUsers[] = {
  {"admin", "admin", "admin"},
  {"root", "root", "admin"},
  {"guest", "guest", "guest"},
  {"test", "test", "guest"},
  {"operator", "operator123", "guest"}
};

const int DEFAULT_USERS_COUNT = 5;

// Vulnerability Flags
bool VULN_SQL_INJECTION = true;
bool VULN_XSS = true;
bool VULN_PATH_TRAVERSAL = true;
bool VULN_COMMAND_INJECTION = true;
bool VULN_CSRF = true;
bool VULN_WEAK_AUTH = true;
bool VULN_INFO_DISCLOSURE = true;
bool VULN_INSECURE_DESERIALIZATION = true;

// Allowed Commands
String allowedCommands[] = {
  "help", "ls", "pwd", "whoami", "id", "ps", "netstat", 
  "ifconfig", "date", "uptime", "free", "df", "cat", "echo"
};
const int ALLOWED_COMMANDS_COUNT = 14;

// ===== FORWARD DECLARATIONS =====
// Config Module
void initConfig();
void loadConfigFromFS();
void saveConfigToFS();

// WiFi Module
void initWiFi();
void connectWiFi();
void startAccessPoint();
void checkWiFiConnection();
String getLocalIP();

// WebServer Module
void initWebServer();
void setupRoutes();
void setupSSL();
void serveStaticFiles();
void handleNotFound(AsyncWebServerRequest *request);
void addCORSHeaders(AsyncWebServerRequest *request);

// Auth Module
void initAuth();
bool authenticateUser(String username, String password);
String generateJWT(String username, String role);
bool validateJWT(String token);
bool isAuthenticated(AsyncWebServerRequest *request);
String getUserRole(String token);
void handleLogin(AsyncWebServerRequest *request);
void handleLogout(AsyncWebServerRequest *request);

// Database Module
void initDatabase();
bool executeQuery(String operation, String data);
String selectQuery(String key);
void createUserTable();
bool insertUser(String username, String password, String role);
String getUserByUsername(String username);
void seedTestData();

// REST API Module
void setupRESTRoutes();
void handleGetUsers(AsyncWebServerRequest *request);
void handlePostUser(AsyncWebServerRequest *request);
void handleGetSystemInfo(AsyncWebServerRequest *request);
void sendJSONResponse(AsyncWebServerRequest *request, int code, String json);

// WebSocket Module
void initWebSocket();
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                       AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(AsyncWebSocketClient *client, String message);
void executeShellCommand(AsyncWebSocketClient *client, String cmd);

// Telnet Module
void initTelnet();
void handleTelnetClients();
void processTelnetCommand(WiFiClient &client, String cmd);
void sendTelnetPrompt(WiFiClient &client);

// Vulnerabilities Module
void setupVulnerableEndpoints();
void handleSQLInjection(AsyncWebServerRequest *request);
void handleXSSVulnerability(AsyncWebServerRequest *request);
void handlePathTraversal(AsyncWebServerRequest *request);
void handleCommandInjection(AsyncWebServerRequest *request);
void handleCSRF(AsyncWebServerRequest *request);

// Crypto Module
String hashPassword(String password);
String generateSalt();
bool verifyPassword(String password, String hash);
String generateRandomToken(int length);
String base64Encode(String input);
String base64Decode(String input);
String sha256Hash(String input);

// Utils Module
String urlDecode(String input);
String urlEncode(String input);
String getContentType(String filename);
bool fileExists(String path);
String readFile(String path);
bool writeFile(String path, String content);
String getCurrentTimestamp();
String generateUUID();

// Debug Module
void initDebug();
void logInfo(String message);
void logError(String message);
void logDebug(String message);
void printSystemInfo();
void printMemoryUsage();
void printWiFiInfo();

// ===== ARDUINO ENTRY POINTS =====

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("  ESP32-H4CK Vulnerable Lab v1.0.0");
  Serial.println("========================================");
  Serial.println();
  
  // Initialize modules in order
  initDebug();
  logInfo("Starting initialization sequence...");
  
  initConfig();
  logInfo("Configuration loaded");
  
  if (!LittleFS.begin(true)) {
    logError("LittleFS Mount Failed");
    return;
  }
  logInfo("Filesystem mounted");
  
  initDatabase();
  logInfo("Database initialized");
  
  initAuth();
  logInfo("Authentication system ready");
  
  initWiFi();
  logInfo("WiFi initialized");
  
  initWebServer();
  logInfo("Web server started");
  
  initWebSocket();
  logInfo("WebSocket handler ready");
  
  initTelnet();
  logInfo("Telnet service started");
  
  setupRESTRoutes();
  logInfo("REST API configured");
  
  if (ENABLE_VULNERABILITIES) {
    setupVulnerableEndpoints();
    logInfo("Vulnerable endpoints enabled (LAB MODE)");
  }
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  System Ready!");
  Serial.println("========================================");
  printSystemInfo();
  printWiFiInfo();
  printMemoryUsage();
  Serial.println("========================================");
}

void loop() {
  // Check WiFi connection periodically
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    checkWiFiConnection();
    lastWiFiCheck = millis();
  }
  
  // Handle telnet clients
  handleTelnetClients();
  
  // Small delay to prevent watchdog issues
  delay(10);
}

