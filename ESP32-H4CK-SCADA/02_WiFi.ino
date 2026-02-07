/*
 * WiFi Management Module
 * 
 * Handles WiFi connectivity in both Station and Access Point modes.
 * Provides auto-reconnect functionality and network monitoring.
 */

void initWiFi() {
  Serial.println("[WIFI] Initializing WiFi...");
  
  // Disable WiFi power save for stability and set max TX power
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  delay(50);
  
  if (STATION_MODE) {
    Serial.println("[WIFI] Starting in Station Mode...");
    WiFi.mode(WIFI_MODE_STA);
    delay(200);
    yield();
    WiFi.setAutoReconnect(true);
    connectWiFi();
  } else {
    Serial.println("[WIFI] Starting in Access Point Mode...");
    WiFi.mode(WIFI_MODE_AP);
    delay(200);
    yield();
    // Start AP in background - will be ready shortly
    startAccessPoint();
  }
  
  Serial.println("[WIFI] WiFi initialization queued");
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
  yield();
  
  // Start AP with limited clients (max 4) and channel 1
  bool apStarted = WiFi.softAP(AP_SSID_STR.c_str(), AP_PASSWORD_STR.c_str(), 1, 0, MAX_AP_CLIENTS);
  yield();
  delay(500); // Give AP time to fully initialize
  yield();
  
  if (apStarted) {
    Serial.println("[WIFI] Access Point started successfully!");
    delay(100);
    Serial.printf("[WIFI] AP IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    yield();
    Serial.printf("[WIFI] AP MAC Address: %s\n", WiFi.softAPmacAddress().c_str());
    yield();
    Serial.printf("[WIFI] Connect with password: %s\n", AP_PASSWORD_STR.c_str());
    
    // Solid LED for AP mode
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("[WIFI] Failed to start Access Point!");
    digitalWrite(LED_PIN, LOW);
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

// WiFi Client Tracking (for AP mode)
void trackConnectedClients() {
  if (!STATION_MODE) {  // Only track in AP mode
    uint8_t numStations = WiFi.softAPgetStationNum();
    if (numStations > 0) {
      wifi_sta_list_t stationList = {0};
      esp_wifi_ap_get_sta_list(&stationList);
      
      for (int i = 0; i < numStations && i < stationList.num; i++) {
        // Check if client is already recorded
        bool found = false;
        for (int j = 0; j < wifiClientCount; j++) {
          if (memcmp(wifiClients[j].mac, stationList.sta[i].mac, 6) == 0) {
            // Update lastSeen
            wifiClients[j].lastSeen = millis();
            found = true;
            break;
          }
        }
        
        if (!found && wifiClientCount < MAX_WIFI_CLIENTS) {
          // Add new client
          IPAddress clientIP = WiFi.softAPIP();
          clientIP[3] = 100 + i;  // Simple IP assignment guess
          
          snprintf(wifiClients[wifiClientCount].ip, 16, "%s", clientIP.toString().c_str());
          sprintf(wifiClients[wifiClientCount].mac, "%02X:%02X:%02X:%02X:%02X:%02X",
                  stationList.sta[i].mac[0], stationList.sta[i].mac[1],
                  stationList.sta[i].mac[2], stationList.sta[i].mac[3],
                  stationList.sta[i].mac[4], stationList.sta[i].mac[5]);
          wifiClients[wifiClientCount].connectedTime = millis();
          wifiClients[wifiClientCount].lastSeen = millis();
          
          Serial.printf("[WIFI] New client connected: %s (%s)\n", 
                        wifiClients[wifiClientCount].mac, 
                        wifiClients[wifiClientCount].ip);
          
          wifiClientCount++;
        }
      }
    }
  }
}

String getWiFiClientsJSON() {
  if (ESP.getFreeHeap() < 5000) return "{\"error\":\"low memory\"}";
  
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();
  
  for (int i = 0; i < wifiClientCount; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["mac"] = wifiClients[i].mac;
    obj["ip"] = wifiClients[i].ip;
    obj["connected_at"] = wifiClients[i].connectedTime;
    obj["last_seen"] = wifiClients[i].lastSeen;
    obj["duration_ms"] = (wifiClients[i].lastSeen - wifiClients[i].connectedTime);
  }
  
  String output;
  serializeJson(doc, output);
  return output;
}

