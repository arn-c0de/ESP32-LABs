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
  size_t totalFS = LittleFS.totalBytes();
  size_t usedFS = LittleFS.usedBytes();
  if (totalFS == 0) {
    Serial.println("\nFilesystem:       ❌ NOT MOUNTED");
    Serial.println("                  Run 'upload.sh' to flash filesystem");
  } else {
    Serial.printf("\nFilesystem Total: %s\n", formatBytes(totalFS).c_str());
    Serial.printf("Filesystem Used:  %s (%d%%)\n",
                  formatBytes(usedFS).c_str(),
                  getFSUsagePercentage());
    Serial.printf("Filesystem Free:  %s\n", 
                  formatBytes(totalFS - usedFS).c_str());
  }
  
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
  Serial.printf("SCADA API:        http://%s/api/dashboard/status\n", getLocalIP().c_str());
  
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
  
  if (millis() - lastCheck > 10000) {  // Check every 10 seconds
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

// Serial Command Handler
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() == 0) return;
    
    Serial.println("\n>>> " + command);
    
    if (command == "/status" || command == "status") {
      printAllServicesStatus();
    } 
    else if (command == "/help" || command == "help") {
      Serial.println("\n=== AVAILABLE SERIAL COMMANDS ===");
      Serial.println("/status   - Show all services status");
      Serial.println("/memory   - Show memory usage");
      Serial.println("/wifi     - Show WiFi information");
      Serial.println("/system   - Show system information");
      Serial.println("/files    - List filesystem contents");
      Serial.println("/restart  - Restart ESP32");
      Serial.println("/help     - Show this help");
      Serial.println("");
      Serial.println("=== DEFENSE COMMANDS (simulated) ===");
      Serial.println("iptables -A INPUT -s <ip> -j DROP --duration <sec>");
      Serial.println("iptables -D INPUT -s <ip> -j DROP");
      Serial.println("iptables -L");
      Serial.println("tc qdisc add rate-limit --src <ip> --duration <sec>");
      Serial.println("tc qdisc del rate-limit --src <ip>");
      Serial.println("tc qdisc show");
      Serial.println("session reset --ip <ip>");
      Serial.println("defense status");
      Serial.println("defense config show");
      Serial.println("defense config set dp=<n> ap=<n> stability=<n>");
      Serial.println("================================\n");
    }
    else if (command == "/memory" || command == "memory") {
      printMemoryUsage();
    }
    else if (command == "/wifi" || command == "wifi") {
      printWiFiInfo();
    }
    else if (command == "/system" || command == "system") {
      printSystemInfo();
    }
    else if (command == "/files" || command == "files") {
      Serial.println("\n=== FILESYSTEM CONTENTS ===");
      listFilesRecursive("/", 0);
      Serial.println();
    }
    else if (command == "/restart" || command == "restart") {
      Serial.println("\n[SYSTEM] Restarting ESP32...\n");
      delay(1000);
      ESP.restart();
    }
    else if (command.startsWith("/defense") || command.startsWith("defense")) {
      // Strip leading slash if present
      String defCmd = command;
      if (defCmd.startsWith("/")) defCmd = defCmd.substring(1);
      
      String result = handleDefenseLine(defCmd);
      Serial.println(result);
    }
    else if (command.startsWith("iptables")) {
      String result = handleDefenseLine(command);
      Serial.println(result);
    }
    else if (command.startsWith("tc ")) {
      String result = handleDefenseLine(command);
      Serial.println(result);
    }
    else if (command.startsWith("session ")) {
      String result = handleDefenseLine(command);
      Serial.println(result);
    }
    else {
      Serial.println("[ERROR] Unknown command. Type /help for available commands.");
    }
  }
}

// Comprehensive Services Status Report
void printAllServicesStatus() {
  Serial.println("\n╔════════════════════════════════════════════════════════════╗");
  Serial.println("║           ESP32-H4CK SERVICES STATUS REPORT               ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝");
  
  // System Information
  Serial.println("\n┌─── SYSTEM ───────────────────────────────────────────────┐");
  Serial.printf("│ Firmware:       v%s (%s)\n", LAB_VERSION, CODENAME);
  Serial.printf("│ Uptime:         %lu seconds\n", millis() / 1000);
  Serial.printf("│ Free Heap:      %d bytes (%d%%)\n", ESP.getFreeHeap(), getFreeHeapPercentage());
  Serial.printf("│ Total Requests: %d\n", totalRequests);
  Serial.printf("│ Active Conn:    %d\n", activeConnections);
  Serial.println("└──────────────────────────────────────────────────────────┘");
  
  // WiFi Status
  Serial.println("\n┌─── WIFI CONNECTION ──────────────────────────────────────┐");
  if (STATION_MODE && WiFi.status() == WL_CONNECTED) {
    Serial.println("│ Mode:           ✅ Station (Connected)");
    Serial.printf("│ SSID:           %s\n", WiFi.SSID().c_str());
    Serial.printf("│ IP Address:     %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("│ Gateway:        %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("│ Signal:         %d dBm\n", WiFi.RSSI());
    Serial.printf("│ MAC Address:    %s\n", WiFi.macAddress().c_str());
  } else if (STATION_MODE) {
    Serial.println("│ Mode:           ❌ Station (Disconnected)");
    Serial.printf("│ SSID:           %s\n", WIFI_SSID_STR.c_str());
    Serial.println("│ Status:         Not Connected");
  }
  
  if (!STATION_MODE || WiFi.getMode() == WIFI_AP_STA) {
    Serial.println("│ AP Mode:        ✅ Active");
    Serial.printf("│ AP SSID:        %s\n", AP_SSID_STR.c_str());
    Serial.printf("│ AP IP:          %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("│ Clients:        %d connected\n", WiFi.softAPgetStationNum());
    Serial.printf("│ AP MAC:         %s\n", WiFi.softAPmacAddress().c_str());
  }
  Serial.println("└──────────────────────────────────────────────────────────┘");
  
  // HTTP Server Status
  Serial.println("\n┌─── HTTP SERVER ──────────────────────────────────────────┐");
  Serial.printf("│ Service:        ✅ RUNNING (Port %d)\n", HTTP_PORT);
  Serial.printf("│ SSL/TLS:        %s\n", SSL_ENABLED ? "✅ Enabled" : "❌ Disabled");
  Serial.printf("│ Total Requests: %d\n", totalRequests);
  Serial.printf("│ Endpoints:      / /login /admin /api/*\n");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("│ Access URL:     http://%s/\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.printf("│ Access URL:     http://%s/\n", WiFi.softAPIP().toString().c_str());
  }
  Serial.println("└──────────────────────────────────────────────────────────┘");
  
  // SCADA System Status
  Serial.println("\n┌─── SCADA SYSTEM ─────────────────────────────────────────┐");
  Serial.println("│ Service:        ✅ RUNNING");
  Serial.printf("│ Production Lines:%d\n", 4);
  Serial.printf("│ Sensors:        %d active\n", SENSOR_COUNT);
  Serial.printf("│ Actuators:      %d active\n", ACTUATOR_COUNT);
  Serial.printf("│ Active Alarms:  %d\n", alarmCount);
  Serial.println("└──────────────────────────────────────────────────────────┘");
  
  // REST API Status
  Serial.println("\n┌─── REST API ─────────────────────────────────────────────┐");
  Serial.println("│ Service:        ✅ RUNNING");
  Serial.println("│ Endpoints:      /api/login, /api/logout");
  Serial.println("│                 /api/users, /api/system");
  Serial.println("│                 /api/debug, /api/config");
  Serial.printf("│ Auth:           JWT (Secret: %s...)\n", JWT_SECRET_STR.substring(0, 8).c_str());
  Serial.println("└──────────────────────────────────────────────────────────┘");
  
  // Database Status
  Serial.println("\n┌─── DATABASE ─────────────────────────────────────────────┐");
  Serial.println("│ Type:           JSON (LittleFS)");
  Serial.printf("│ File:           %s\n", DB_FILE_PATH);
  File dbFile = LittleFS.open(DB_FILE_PATH, "r");
  if (dbFile) {
    Serial.printf("│ Size:           %d bytes\n", dbFile.size());
    Serial.println("│ Status:         ✅ OK");
    dbFile.close();
  } else {
    Serial.println("│ Status:         ❌ File not found");
  }
  Serial.printf("│ Default Users:  %d configured\n", DEFAULT_USERS_COUNT);
  Serial.println("└──────────────────────────────────────────────────────────┘");
  
  // Filesystem Status
  Serial.println("\n┌─── FILESYSTEM ───────────────────────────────────────────┐");
  Serial.println("│ Type:           LittleFS");
  size_t totalFS = LittleFS.totalBytes();
  size_t usedFS = LittleFS.usedBytes();
  if (totalFS == 0) {
    Serial.println("│ Status:         ❌ NOT MOUNTED");
    Serial.println("│ Action:         Run ./upload.sh to flash filesystem");
  } else {
    Serial.printf("│ Total:          %d bytes\n", totalFS);
    Serial.printf("│ Used:           %d bytes\n", usedFS);
    Serial.printf("│ Free:           %d bytes\n", totalFS - usedFS);
    Serial.printf("│ Usage:          %d%%\n", (usedFS * 100) / totalFS);
  }
  Serial.println("└──────────────────────────────────────────────────────────┘");
  
  // Security Status
  Serial.println("\n┌─── SECURITY FEATURES ────────────────────────────────────┐");
  Serial.printf("│ Vulnerabilities: %s\n", ENABLE_VULNERABILITIES ? "⚠️  ENABLED (LAB MODE)" : "✅ Disabled");
  Serial.printf("│ Debug Mode:      %s\n", DEBUG_MODE ? "⚠️  ENABLED" : "✅ Disabled");
  Serial.printf("│ SQL Injection:   %s\n", VULN_SQL_INJECTION ? "⚠️  Vulnerable" : "✅ Protected");
  Serial.printf("│ XSS:             %s\n", VULN_XSS ? "⚠️  Vulnerable" : "✅ Protected");
  Serial.printf("│ Path Traversal:  %s\n", VULN_PATH_TRAVERSAL ? "⚠️  Vulnerable" : "✅ Protected");
  Serial.printf("│ Cmd Injection:   %s\n", VULN_COMMAND_INJECTION ? "⚠️  Vulnerable" : "✅ Protected");
  Serial.printf("│ CSRF:            %s\n", VULN_CSRF ? "⚠️  Vulnerable" : "✅ Protected");
  Serial.println("└──────────────────────────────────────────────────────────┘");
  
  // Active Sessions
  Serial.println("\n┌─── ACTIVE SESSIONS ──────────────────────────────────────┐");
  Serial.printf("│ Total Sessions:  %d\n", activeSessions.size());
  if (activeSessions.size() > 0) {
    int count = 0;
    for (auto& session : activeSessions) {
      if (count < 5) { // Show max 5 sessions
        Serial.printf("│ - User: %-10s Role: %-8s IP: %s\n", 
                      session.second.username.c_str(), 
                      session.second.role.c_str(),
                      session.second.ipAddress.c_str());
        count++;
      }
    }
    if (activeSessions.size() > 5) {
      Serial.printf("│ ... and %d more sessions\n", activeSessions.size() - 5);
    }
  }
  Serial.println("└──────────────────────────────────────────────────────────┘");
  
  Serial.println("\n╔════════════════════════════════════════════════════════════╗");
  Serial.println("║ Type /help for more commands                              ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝\n");
}

// List filesystem contents recursively
void listFilesRecursive(String path, int level) {
  File root = LittleFS.open(path);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open directory: " + path);
    return;
  }

  File file = root.openNextFile();
  while (file) {
    // Indentation
    for (int i = 0; i < level; i++) {
      Serial.print("  ");
    }
    
    // Print filename and size
    if (file.isDirectory()) {
      Serial.print("📁 ");
      Serial.println(file.name());
      listFilesRecursive(file.path(), level + 1);
    } else {
      Serial.print("📄 ");
      Serial.print(file.name());
      Serial.print(" (");
      Serial.print(formatBytes(file.size()));
      Serial.println(")");
    }
    
    file = root.openNextFile();
  }
}

