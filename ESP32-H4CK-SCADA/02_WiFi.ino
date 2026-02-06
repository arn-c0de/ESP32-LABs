// ============================================================
// 02_WiFi.ino — WiFi AP + STA mode
// ============================================================

String wifiLocalIP = "";
int wifiConnectedClients = 0;

void wifiInit() {
  Serial.println("[WIFI] Initializing...");

  WiFi.mode(WIFI_AP_STA);

  // Access Point mode
  if (WIFI_AP_MODE) {
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    WiFi.softAP(AP_SSID, AP_PASSWORD, 1, 0, 8);
    Serial.printf("[WIFI] AP started: %s @ %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
    wifiLocalIP = WiFi.softAPIP().toString();
  }

  // Station mode (connect to existing network)
  if (WIFI_STA_MODE) {
    Serial.printf("[WIFI] Connecting to %s...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      wifiLocalIP = WiFi.localIP().toString();
      Serial.printf("\n[WIFI] STA connected: %s\n", wifiLocalIP.c_str());
    } else {
      Serial.println("\n[WIFI] STA connection failed. AP mode only.");
    }
  }

  delay(100);
  
  Serial.printf("[WIFI] Access: http://%s\n", wifiLocalIP.c_str());
}

String wifiGetIP() {
  if (WIFI_STA_MODE && WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return WiFi.softAPIP().toString();
}

int wifiGetClients() {
  return WiFi.softAPgetStationNum();
}

String wifiStatusJson() {
  JsonDocument doc;
  doc["ap_mode"]    = WIFI_AP_MODE;
  doc["sta_mode"]   = WIFI_STA_MODE;
  doc["ap_ssid"]    = AP_SSID;
  doc["ap_ip"]      = WiFi.softAPIP().toString();
  doc["sta_connected"] = (WiFi.status() == WL_CONNECTED);
  if (WiFi.status() == WL_CONNECTED) {
    doc["sta_ip"]   = WiFi.localIP().toString();
    doc["sta_ssid"] = WiFi.SSID();
    doc["sta_rssi"] = WiFi.RSSI();
  }
  doc["clients"]    = wifiGetClients();
  doc["mac"]        = WiFi.macAddress();

  String out;
  serializeJson(doc, out);
  return out;
}
