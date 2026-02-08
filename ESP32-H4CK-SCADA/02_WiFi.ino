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

// Resolve WiFi client IP from DHCP/netif - try multiple methods
static bool resolveStationIp(const uint8_t *mac, char *ipStr, size_t len) {
  // Method 1: tcpip_adapter (legacy but widely compatible)
  // Re-fetch a fresh station list for tcpip_adapter
  wifi_sta_list_t freshList = {0};
  esp_wifi_ap_get_sta_list(&freshList);
  tcpip_adapter_sta_list_t tcpList = {0};
  if (tcpip_adapter_get_sta_list(&freshList, &tcpList) == ESP_OK) {
    for (int i = 0; i < tcpList.num; i++) {
      if (memcmp(tcpList.sta[i].mac, mac, 6) == 0 && tcpList.sta[i].ip.addr != 0) {
        uint32_t ip = tcpList.sta[i].ip.addr;
        snprintf(ipStr, len, "%d.%d.%d.%d",
                 ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
        return true;
      }
    }
  }

  // Method 2: esp_netif (newer API)
  esp_netif_sta_list_t netifList = {0};
  if (esp_netif_get_sta_list(&freshList, &netifList) == ESP_OK) {
    for (int i = 0; i < netifList.num; i++) {
      if (memcmp(netifList.sta[i].mac, mac, 6) == 0 && netifList.sta[i].ip.addr != 0) {
        uint32_t ip = netifList.sta[i].ip.addr;
        snprintf(ipStr, len, "%d.%d.%d.%d",
                 ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
        return true;
      }
    }
  }

  return false;
}

// WiFi Client Tracking (for AP mode)
void trackConnectedClients() {
  if (!STATION_MODE) {  // Only track in AP mode
    uint8_t numStations = WiFi.softAPgetStationNum();
    if (numStations > 0) {
      wifi_sta_list_t stationList = {0};
      esp_wifi_ap_get_sta_list(&stationList);

      for (int i = 0; i < (int)stationList.num; i++) {
        uint8_t *mac = stationList.sta[i].mac;

        // Format MAC to string for comparison with stored entries
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        // Check if client is already recorded (compare formatted MAC strings)
        bool found = false;
        for (int j = 0; j < wifiClientCount; j++) {
          if (strcmp(wifiClients[j].mac, macStr) == 0) {
            wifiClients[j].lastSeen = millis();
            // Try to resolve IP if still unknown
            if (strcmp(wifiClients[j].ip, "resolving") == 0) {
              char ipStr[16];
              if (resolveStationIp(mac, ipStr, sizeof(ipStr))) {
                strncpy(wifiClients[j].ip, ipStr, 15);
                wifiClients[j].ip[15] = '\0';
                Serial.printf("[WIFI] Resolved client %s -> %s\n", macStr, ipStr);
              }
            }
            found = true;
            break;
          }
        }

        if (!found && wifiClientCount < MAX_WIFI_CLIENTS) {
          // Try to resolve IP
          char ipStr[16] = "resolving";
          resolveStationIp(mac, ipStr, sizeof(ipStr));

          strncpy(wifiClients[wifiClientCount].ip, ipStr, 15);
          wifiClients[wifiClientCount].ip[15] = '\0';
          strncpy(wifiClients[wifiClientCount].mac, macStr, 17);
          wifiClients[wifiClientCount].mac[17] = '\0';
          wifiClients[wifiClientCount].connectedTime = millis();
          wifiClients[wifiClientCount].lastSeen = millis();

          Serial.printf("[WIFI] New client connected: %s (%s)\n", macStr, ipStr);

          wifiClientCount++;
        }
      }
    }
  }
}

// Called from HTTP request handlers to update WiFi client IP from actual traffic
void updateWifiClientIpFromRequest(const String &ip) {
  if (STATION_MODE || ip.length() == 0) return;
  // Only update clients that still show "resolving"
  for (int i = 0; i < wifiClientCount; i++) {
    if (strcmp(wifiClients[i].ip, "resolving") == 0) {
      // Check if this IP is already assigned to another client
      bool ipTaken = false;
      for (int j = 0; j < wifiClientCount; j++) {
        if (j != i && strcmp(wifiClients[j].ip, ip.c_str()) == 0) {
          ipTaken = true;
          break;
        }
      }
      if (!ipTaken) {
        strncpy(wifiClients[i].ip, ip.c_str(), 15);
        wifiClients[i].ip[15] = '\0';
        Serial.printf("[WIFI] Resolved client %s -> %s (from HTTP)\n", wifiClients[i].mac, ip.c_str());
        return;
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

