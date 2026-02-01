/*
 * WiFi Management Module
 * 
 * Handles WiFi connectivity in both Station and Access Point modes.
 * Provides auto-reconnect functionality and network monitoring.
 */

void initWiFi() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.setAutoReconnect(true);
  
  if (STATION_MODE) {
    Serial.println("[WIFI] Starting in Station Mode...");
    connectWiFi();
  } else {
    Serial.println("[WIFI] Starting in Access Point Mode...");
    startAccessPoint();
  }
}

void connectWiFi() {
  Serial.printf("[WIFI] Connecting to: %s\n", WIFI_SSID_STR.c_str());
  
  WiFi.begin(WIFI_SSID_STR.c_str(), WIFI_PASSWORD_STR.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected successfully!");
    Serial.printf("[WIFI] IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WIFI] MAC Address: %s\n", WiFi.macAddress().c_str());
    Serial.printf("[WIFI] Signal Strength: %d dBm\n", WiFi.RSSI());
    
    // Blink LED to indicate successful connection
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  } else {
    Serial.println("[WIFI] Connection failed! Starting AP mode as fallback...");
    startAccessPoint();
  }
}

void startAccessPoint() {
  Serial.printf("[WIFI] Starting Access Point: %s\n", AP_SSID_STR.c_str());
  
  WiFi.mode(WIFI_AP);
  bool apStarted = WiFi.softAP(AP_SSID_STR.c_str(), AP_PASSWORD_STR.c_str());
  
  if (apStarted) {
    Serial.println("[WIFI] Access Point started successfully!");
    Serial.printf("[WIFI] AP IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("[WIFI] AP MAC Address: %s\n", WiFi.softAPmacAddress().c_str());
    Serial.printf("[WIFI] Connect with password: %s\n", AP_PASSWORD_STR.c_str());
    
    // Solid LED for AP mode
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("[WIFI] Failed to start Access Point!");
  }
}

void checkWiFiConnection() {
  if (STATION_MODE && WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Connection lost. Attempting to reconnect...");
    connectWiFi();
  }
}

String getLocalIP() {
  if (STATION_MODE) {
    return WiFi.localIP().toString();
  } else {
    return WiFi.softAPIP().toString();
  }
}

String getMACAddress() {
  if (STATION_MODE) {
    return WiFi.macAddress();
  } else {
    return WiFi.softAPmacAddress();
  }
}

int getSignalStrength() {
  if (STATION_MODE && WiFi.status() == WL_CONNECTED) {
    return WiFi.RSSI();
  }
  return 0;
}

void scanNetworks() {
  Serial.println("[WIFI] Scanning for networks...");
  int n = WiFi.scanNetworks();
  
  Serial.printf("[WIFI] Found %d networks:\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("  %d: %s (%d dBm) %s\n", 
                  i + 1, 
                  WiFi.SSID(i).c_str(), 
                  WiFi.RSSI(i),
                  WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "[OPEN]" : "[SECURED]");
  }
}

void handleWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("[WIFI EVENT] Connected to AP");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("[WIFI EVENT] Disconnected from AP");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.printf("[WIFI EVENT] Got IP: %s\n", WiFi.localIP().toString().c_str());
      break;
    default:
      break;
  }
}
