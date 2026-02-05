/*
 * Configuration Module
 * 
 * Configuration management functions.
 * Note: All global variables and constants are now declared in the main .ino file
 * for proper Arduino IDE compilation order.
 */

void initConfig() {
  // Initialize preferences for persistent storage
  preferences.begin("esp32-hack", false);
  
  // Load saved configuration if exists
  loadConfigFromFS();
  
  // Set system start time
  systemStartTime = millis();
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize button pin
  pinMode(USER_BUTTON_PIN, INPUT_PULLUP);
  
  Serial.println("[CONFIG] Configuration initialized");
  Serial.printf("[CONFIG] Lab Mode: %s\n", LAB_MODE_STR.c_str());
  Serial.printf("[CONFIG] Station Mode: %s\n", STATION_MODE ? "YES" : "NO");
  Serial.printf("[CONFIG] Vulnerabilities: %s\n", ENABLE_VULNERABILITIES ? "ENABLED" : "DISABLED");
  Serial.printf("[CONFIG] Debug Mode: %s\n", DEBUG_MODE ? "ENABLED" : "DISABLED");
  Serial.printf("[CONFIG] Admin protection: %s\n", PROTECT_ADMIN_ENDPOINTS ? "ENABLED" : "DISABLED");
}

void loadConfigFromFS() {
  // Load configuration from preferences
  if (preferences.isKey("wifi_ssid")) {
    WIFI_SSID_STR = preferences.getString("wifi_ssid", WIFI_SSID_STR);
  }
  if (preferences.isKey("wifi_pass")) {
    WIFI_PASSWORD_STR = preferences.getString("wifi_pass", WIFI_PASSWORD_STR);
  }
  if (preferences.isKey("debug_mode")) {
    DEBUG_MODE = preferences.getBool("debug_mode", DEBUG_MODE);
  }
  if (preferences.isKey("enable_vulns")) {
    ENABLE_VULNERABILITIES = preferences.getBool("enable_vulns", ENABLE_VULNERABILITIES);
  }
  if (preferences.isKey("lab_mode")) {
    LAB_MODE_STR = preferences.getString("lab_mode", LAB_MODE_STR);
  }
  if (preferences.isKey("protect_admin_endpoints")) {
    PROTECT_ADMIN_ENDPOINTS = preferences.getBool("protect_admin_endpoints", PROTECT_ADMIN_ENDPOINTS);
  }
}

void saveConfigToFS() {
  preferences.putString("wifi_ssid", WIFI_SSID_STR);
  preferences.putString("wifi_pass", WIFI_PASSWORD_STR);
  preferences.putString("lab_mode", LAB_MODE_STR);
  preferences.putBool("debug_mode", DEBUG_MODE);
  preferences.putBool("enable_vulns", ENABLE_VULNERABILITIES);
  preferences.putBool("protect_admin_endpoints", PROTECT_ADMIN_ENDPOINTS);
  Serial.println("[CONFIG] Configuration saved to flash");
}

String getConfigValue(String key) {
  if (key == "wifi_ssid") return WIFI_SSID_STR;
  if (key == "wifi_password" || key == "wifi_pass") return WIFI_PASSWORD_STR;
  if (key == "debug_mode") return DEBUG_MODE ? "true" : "false";
  if (key == "enable_vulnerabilities") return ENABLE_VULNERABILITIES ? "true" : "false";
  if (key == "protect_admin_endpoints") return PROTECT_ADMIN_ENDPOINTS ? "true" : "false";
  return "";
}

void setConfigValue(String key, String value) {
  if (key == "wifi_ssid") WIFI_SSID_STR = value;
  else if (key == "wifi_password" || key == "wifi_pass") WIFI_PASSWORD_STR = value;
  else if (key == "debug_mode") DEBUG_MODE = (value == "true");
  else if (key == "enable_vulnerabilities") ENABLE_VULNERABILITIES = (value == "true");
  else if (key == "protect_admin_endpoints") PROTECT_ADMIN_ENDPOINTS = (value == "true");
  
  saveConfigToFS();
  
  saveConfigToFS();
}
