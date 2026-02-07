/*
 * SCADA Configuration Module
 * 
 * These values will be automatically injected from .env by upload.sh
 */

// WiFi Configuration (injected by upload.sh from .env)
const char* WIFI_SSID     = "YourNetworkName";
const char* WIFI_PASSWORD = "YourNetworkPassword";
const char* AP_SSID       = "ESP32-H4CK-AP";
const char* AP_PASSWORD   = "vulnerable";
bool        WIFI_STA_MODE = false;

// Security Configuration (injected by upload.sh from .env)
const char* JWT_SECRET              = "weak_secret_key_123";
const char* LAB_MODE                = "testing";
const char* TELNET_ADMIN_PASSWORD   = "admin";
const char* TELNET_GUEST_PASSWORD   = "guest";
const char* TELNET_ROOT_PASSWORD    = "toor";

void initConfig() {
  // Override global strings with config values
  WIFI_SSID_STR = String(WIFI_SSID);
  WIFI_PASSWORD_STR = String(WIFI_PASSWORD);
  AP_SSID_STR = String(AP_SSID);
  AP_PASSWORD_STR = String(AP_PASSWORD);
  STATION_MODE = WIFI_STA_MODE;
  JWT_SECRET_STR = String(JWT_SECRET);
  LAB_MODE_STR = String(LAB_MODE);  // Initialize from injected config
  
  preferences.begin("scada-lab", false);
  loadConfigFromFS();
  systemStartTime = millis();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("[CONFIG] SCADA Configuration initialized");
  Serial.printf("[CONFIG] Lab Mode: %s\n", LAB_MODE_STR.c_str());
  Serial.printf("[CONFIG] Difficulty: %s\n", DIFFICULTY == EASY ? "EASY" : DIFFICULTY == NORMAL ? "NORMAL" : "HARD");
  Serial.printf("[CONFIG] Station Mode: %s\n", STATION_MODE ? "YES" : "NO");
  Serial.printf("[CONFIG] Vulnerabilities: %s\n", ENABLE_VULNERABILITIES ? "ENABLED" : "DISABLED");
}

void loadConfigFromFS() {
  if (preferences.isKey("wifi_ssid")) WIFI_SSID_STR = preferences.getString("wifi_ssid", WIFI_SSID_STR);
  if (preferences.isKey("wifi_pass")) WIFI_PASSWORD_STR = preferences.getString("wifi_pass", WIFI_PASSWORD_STR);
  if (preferences.isKey("debug_mode")) DEBUG_MODE = preferences.getBool("debug_mode", DEBUG_MODE);
  if (preferences.isKey("enable_vulns")) ENABLE_VULNERABILITIES = preferences.getBool("enable_vulns", ENABLE_VULNERABILITIES);
  if (preferences.isKey("lab_mode")) LAB_MODE_STR = preferences.getString("lab_mode", LAB_MODE_STR);
  if (preferences.isKey("difficulty")) DIFFICULTY = (DifficultyLevel)preferences.getInt("difficulty", DIFFICULTY);
  if (preferences.isKey("protect_admin")) PROTECT_ADMIN_ENDPOINTS = preferences.getBool("protect_admin", PROTECT_ADMIN_ENDPOINTS);
}

void saveConfigToFS() {
  preferences.putString("wifi_ssid", WIFI_SSID_STR);
  preferences.putString("wifi_pass", WIFI_PASSWORD_STR);
  preferences.putString("lab_mode", LAB_MODE_STR);
  preferences.putBool("debug_mode", DEBUG_MODE);
  preferences.putBool("enable_vulns", ENABLE_VULNERABILITIES);
  preferences.putInt("difficulty", (int)DIFFICULTY);
  preferences.putBool("protect_admin", PROTECT_ADMIN_ENDPOINTS);
  Serial.println("[CONFIG] Configuration saved");
}
