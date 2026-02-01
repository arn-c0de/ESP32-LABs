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
  Serial.printf("[CONFIG] Station Mode: %s\n", STATION_MODE ? "YES" : "NO");
  Serial.printf("[CONFIG] Vulnerabilities: %s\n", ENABLE_VULNERABILITIES ? "ENABLED" : "DISABLED");
  Serial.printf("[CONFIG] Debug Mode: %s\n", DEBUG_MODE ? "ENABLED" : "DISABLED");
}

void loadConfigFromFS() {
  // Load configuration from preferences
  if (preferences.isKey("wifi_ssid")) {
    WIFI_SSID = preferences.getString("wifi_ssid", WIFI_SSID);
  }
  if (preferences.isKey("wifi_pass")) {
    WIFI_PASSWORD = preferences.getString("wifi_pass", WIFI_PASSWORD);
  }
  if (preferences.isKey("debug_mode")) {
    DEBUG_MODE = preferences.getBool("debug_mode", DEBUG_MODE);
  }
  if (preferences.isKey("enable_vulns")) {
    ENABLE_VULNERABILITIES = preferences.getBool("enable_vulns", ENABLE_VULNERABILITIES);
  }
}

void saveConfigToFS() {
  preferences.putString("wifi_ssid", WIFI_SSID);
  preferences.putString("wifi_pass", WIFI_PASSWORD);
  preferences.putBool("debug_mode", DEBUG_MODE);
  preferences.putBool("enable_vulns", ENABLE_VULNERABILITIES);
  Serial.println("[CONFIG] Configuration saved to flash");
}

String getConfigValue(String key) {
  if (key == "wifi_ssid") return WIFI_SSID;
  if (key == "wifi_password") return WIFI_PASSWORD;
  if (key == "debug_mode") return DEBUG_MODE ? "true" : "false";
  if (key == "enable_vulnerabilities") return ENABLE_VULNERABILITIES ? "true" : "false";
  return "";
}

void setConfigValue(String key, String value) {
  if (key == "wifi_ssid") WIFI_SSID = value;
  else if (key == "wifi_password") WIFI_PASSWORD = value;
  else if (key == "debug_mode") DEBUG_MODE = (value == "true");
  else if (key == "enable_vulnerabilities") ENABLE_VULNERABILITIES = (value == "true");
  
  saveConfigToFS();
}
