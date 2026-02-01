/*
 * Debug and Logging Module
 * 
 * Provides logging, debugging, and system monitoring functions.
 * Includes memory monitoring, WiFi diagnostics, and detailed system info.
 */

void initDebug() {
  Serial.println("[DEBUG] Debug system initialized");
  Serial.printf("[DEBUG] Debug mode: %s\n", DEBUG_MODE ? "ENABLED" : "DISABLED");
  
  // Initialize random seed for UUID generation
  randomSeed(ESP.getCycleCount());
}

void logInfo(String message) {
  Serial.print("[INFO] ");
  Serial.println(message);
  
  // Append to log file
  if (DEBUG_MODE) {
    String logEntry = getCurrentTimestamp() + " [INFO] " + message + "\n";
    appendFile(LOG_FILE_PATH, logEntry);
  }
}

void logError(String message) {
  Serial.print("[ERROR] ");
  Serial.println(message);
  
  // Always log errors
  String logEntry = getCurrentTimestamp() + " [ERROR] " + message + "\n";
  appendFile(LOG_FILE_PATH, logEntry);
}

void logDebug(String message) {
  if (!DEBUG_MODE) return;
  
  Serial.print("[DEBUG] ");
  Serial.println(message);
  
  String logEntry = getCurrentTimestamp() + " [DEBUG] " + message + "\n";
  appendFile(LOG_FILE_PATH, logEntry);
}

void logWarning(String message) {
  Serial.print("[WARNING] ");
  Serial.println(message);
  
  if (DEBUG_MODE) {
    String logEntry = getCurrentTimestamp() + " [WARNING] " + message + "\n";
    appendFile(LOG_FILE_PATH, logEntry);
  }
}

void printSystemInfo() {
  Serial.println("\n=== SYSTEM INFORMATION ===");
  Serial.printf("Chip Model:       %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision:    %d\n", ESP.getChipRevision());
  Serial.printf("CPU Cores:        %d\n", ESP.getChipCores());
  Serial.printf("CPU Frequency:    %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash Size:       %s\n", formatBytes(ESP.getFlashChipSize()).c_str());
  Serial.printf("Flash Speed:      %d Hz\n", ESP.getFlashChipSpeed());
  Serial.printf("Sketch Size:      %s\n", formatBytes(ESP.getSketchSize()).c_str());
  Serial.printf("Free Sketch Space:%s\n", formatBytes(ESP.getFreeSketchSpace()).c_str());
  
  // SDK and framework info
  Serial.printf("SDK Version:      %s\n", ESP.getSdkVersion());
  Serial.printf("IDF Version:      %s\n", esp_get_idf_version());
  
  Serial.println();
}

void printMemoryUsage() {
  Serial.println("=== MEMORY USAGE ===");
  Serial.printf("Total Heap:       %s\n", formatBytes(ESP.getHeapSize()).c_str());
  Serial.printf("Free Heap:        %s (%d%%)\n", 
                formatBytes(ESP.getFreeHeap()).c_str(),
                getFreeHeapPercentage());
  Serial.printf("Min Free Heap:    %s\n", formatBytes(ESP.getMinFreeHeap()).c_str());
  Serial.printf("Max Alloc Heap:   %s\n", formatBytes(ESP.getMaxAllocHeap()).c_str());
  
  // PSRAM info (if available)
  if (ESP.getPsramSize() > 0) {
    Serial.printf("Total PSRAM:      %s\n", formatBytes(ESP.getPsramSize()).c_str());
    Serial.printf("Free PSRAM:       %s\n", formatBytes(ESP.getFreePsram()).c_str());
  } else {
    Serial.println("PSRAM:            Not available");
  }
  
  // Filesystem usage
  Serial.printf("\nFilesystem Total: %s\n", formatBytes(LittleFS.totalBytes()).c_str());
  Serial.printf("Filesystem Used:  %s (%d%%)\n",
                formatBytes(LittleFS.usedBytes()).c_str(),
                getFSUsagePercentage());
  Serial.printf("Filesystem Free:  %s\n", 
                formatBytes(LittleFS.totalBytes() - LittleFS.usedBytes()).c_str());
  
  Serial.println();
}

void printWiFiInfo() {
  Serial.println("=== WIFI INFORMATION ===");
  
  if (STATION_MODE) {
    Serial.println("Mode:             Station");
    Serial.printf("SSID:             %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Address:       %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Subnet Mask:      %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("Gateway:          %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS:              %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("MAC Address:      %s\n", WiFi.macAddress().c_str());
    Serial.printf("Signal Strength:  %d dBm\n", WiFi.RSSI());
    Serial.printf("Channel:          %d\n", WiFi.channel());
  } else {
    Serial.println("Mode:             Access Point");
    Serial.printf("SSID:             %s\n", AP_SSID_STR.c_str());
    Serial.printf("IP Address:       %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("MAC Address:      %s\n", WiFi.softAPmacAddress().c_str());
    Serial.printf("Connected Clients:%d\n", WiFi.softAPgetStationNum());
  }
  
  Serial.println();
}

void printServiceInfo() {
  Serial.println("=== ACTIVE SERVICES ===");
  Serial.printf("HTTP Server:      %s:%d\n", getLocalIP().c_str(), HTTP_PORT);
  Serial.printf("WebSocket:        ws://%s/shell\n", getLocalIP().c_str());
  
  if (ENABLE_TELNET) {
    Serial.printf("Telnet:           %s:%d\n", getLocalIP().c_str(), TELNET_PORT);
  }
  
  Serial.printf("Active Sessions:  %d\n", activeSessions.size());
  Serial.printf("Active Connections:%d\n", activeConnections);
  Serial.printf("Total Requests:   %d\n", totalRequests);
  Serial.printf("Uptime:           %s\n", formatTime(millis()).c_str());
  
  Serial.println();
}

void printSecurityStatus() {
  Serial.println("=== SECURITY STATUS ===");
  Serial.printf("Vulnerabilities:  %s\n", ENABLE_VULNERABILITIES ? "ENABLED (LAB MODE)" : "DISABLED");
  Serial.printf("SSL/TLS:          %s\n", SSL_ENABLED ? "ENABLED" : "DISABLED");
  Serial.printf("Debug Mode:       %s\n", DEBUG_MODE ? "ENABLED" : "DISABLED");
  
  Serial.println("\nVulnerability Flags:");
  Serial.printf("  SQL Injection:  %s\n", VULN_SQL_INJECTION ? "YES" : "NO");
  Serial.printf("  XSS:            %s\n", VULN_XSS ? "YES" : "NO");
  Serial.printf("  Path Traversal: %s\n", VULN_PATH_TRAVERSAL ? "YES" : "NO");
  Serial.printf("  Command Inject: %s\n", VULN_COMMAND_INJECTION ? "YES" : "NO");
  Serial.printf("  CSRF:           %s\n", VULN_CSRF ? "YES" : "NO");
  Serial.printf("  Weak Auth:      %s\n", VULN_WEAK_AUTH ? "YES" : "NO");
  
  Serial.println();
}

void dumpRequestInfo(AsyncWebServerRequest *request) {
  if (!DEBUG_MODE) return;
  
  Serial.println("=== REQUEST DEBUG ===");
  Serial.printf("Method:   %s\n", request->methodToString());
  Serial.printf("URL:      %s\n", request->url().c_str());
  Serial.printf("Host:     %s\n", request->host().c_str());
  Serial.printf("Client:   %s\n", request->client()->remoteIP().toString().c_str());
  Serial.printf("Args:     %d\n", request->args());
  
  for (uint8_t i = 0; i < request->args(); i++) {
    Serial.printf("  [%d] %s = %s\n", i, request->argName(i).c_str(), request->arg(i).c_str());
  }
  
  Serial.printf("Headers:  %d\n", request->headers());
  for (uint8_t i = 0; i < request->headers(); i++) {
    const AsyncWebHeader* h = request->getHeader(i);
    Serial.printf("  %s: %s\n", h->name().c_str(), h->value().c_str());
  }
  
  Serial.println();
}

void printAllFiles() {
  Serial.println("=== FILESYSTEM CONTENTS ===");
  Serial.println(listFiles("/"));
}

void monitorMemory() {
  static unsigned long lastCheck = 0;
  static size_t lastFreeHeap = 0;
  
  if (millis() - lastCheck > 5000) {  // Check every 5 seconds
    size_t currentFreeHeap = ESP.getFreeHeap();
    
    if (lastFreeHeap > 0) {
      int diff = currentFreeHeap - lastFreeHeap;
      if (abs(diff) > 1000) {  // Significant change
        Serial.printf("[MEMORY] Heap change: %+d bytes (now: %s)\n", 
                      diff, formatBytes(currentFreeHeap).c_str());
      }
    }
    
    lastFreeHeap = currentFreeHeap;
    lastCheck = millis();
    
    // Warn on low memory
    if (getFreeHeapPercentage() < 20) {
      logWarning("Low memory: Only " + String(getFreeHeapPercentage()) + "% free heap remaining!");
    }
  }
}

void performSelfTest() {
  Serial.println("\n=== SYSTEM SELF-TEST ===");
  
  // Test filesystem
  Serial.print("Filesystem:       ");
  if (LittleFS.begin()) {
    Serial.println("OK");
  } else {
    Serial.println("FAIL");
  }
  
  // Test WiFi
  Serial.print("WiFi:             ");
  if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() >= 0) {
    Serial.println("OK");
  } else {
    Serial.println("FAIL");
  }
  
  // Test memory
  Serial.print("Memory:           ");
  if (ESP.getFreeHeap() > 50000) {
    Serial.println("OK");
  } else {
    Serial.println("WARNING - Low memory");
  }
  
  Serial.println("=== SELF-TEST COMPLETE ===\n");
}
