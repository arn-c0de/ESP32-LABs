/*
 * Telnet Module
 * 
 * Provides telnet service for remote shell access.
 * Intentionally vulnerable for training purposes (cleartext, weak auth).
 */

WiFiClient telnetClients[5];  // Support up to 5 concurrent telnet clients
bool telnetAuthenticated[5] = {false, false, false, false, false};
String telnetUsernames[5];

void initTelnet() {
  if (!ENABLE_TELNET) {
    Serial.println("[TELNET] Service disabled");
    return;
  }
  
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  
  Serial.printf("[TELNET] Telnet server started on port %d\n", TELNET_PORT);
  Serial.printf("[TELNET] Credentials from .env: admin/*****, guest/*****, root/*****\n");
  Serial.printf("[TELNET] Connect with: telnet %s %d\n", getLocalIP().c_str(), TELNET_PORT);
}

void handleTelnetClients() {
  if (!ENABLE_TELNET) return;
  
  // Check for new clients
  if (telnetServer.hasClient()) {
    // Find empty slot
    int freeSlot = -1;
    for (int i = 0; i < 5; i++) {
      if (!telnetClients[i] || !telnetClients[i].connected()) {
        if (telnetClients[i]) telnetClients[i].stop();
        telnetClients[i] = telnetServer.available();
        telnetAuthenticated[i] = false;
        telnetUsernames[i] = "";
        freeSlot = i;
        
        Serial.printf("[TELNET] ✅ New client #%d from %s\n", 
                      i, telnetClients[i].remoteIP().toString().c_str());
        
        telnetClients[i].println("ESP32-H4CK Telnet Service");
        telnetClients[i].println("========================");
        telnetClients[i].println();
        
        // Intentional vulnerability: Optional authentication
        if (VULN_WEAK_AUTH) {
          telnetClients[i].println("WARNING: Weak authentication mode enabled!");
          telnetAuthenticated[i] = true;  // Auto-authenticate in vulnerable mode
          telnetUsernames[i] = "anonymous";
          Serial.printf("[TELNET] Client #%d auto-authenticated as anonymous\n", i);
          sendTelnetPrompt(telnetClients[i]);
        } else {
          telnetClients[i].print("Username: ");
        }
        
        break;
      }
    }
    
    if (freeSlot == -1) {
      // No free slots, reject connection
      WiFiClient rejectedClient = telnetServer.available();
      rejectedClient.println("ERROR: Server full");
      rejectedClient.stop();
      Serial.println("[TELNET] Connection rejected - server full");
    }
  }
  
  // Handle existing clients
  for (int i = 0; i < 5; i++) {
    if (telnetClients[i] && telnetClients[i].connected()) {
      if (telnetClients[i].available()) {
        String line = telnetClients[i].readStringUntil('\n');
        line.trim();
        
        if (!telnetAuthenticated[i]) {
          // Handle authentication
          if (telnetUsernames[i] == "") {
            // Reading username
            telnetUsernames[i] = line;
            telnetClients[i].print("Password: ");
          } else {
            // Reading password
            String password = line;
            String username = telnetUsernames[i];
            bool authenticated = false;
            
            // Check default users first
            if (authenticateUser(username, password)) {
              authenticated = true;
            }
            // Also check environment-based passwords
            else if (username == "admin" && password == TELNET_ADMIN_PASSWORD_STR) {
              authenticated = true;
            } else if (username == "guest" && password == TELNET_GUEST_PASSWORD_STR) {
              authenticated = true;
            } else if (username == "root" && password == TELNET_ROOT_PASSWORD_STR) {
              authenticated = true;
            }
            
            if (authenticated) {
              telnetAuthenticated[i] = true;
              telnetClients[i].println("\nLogin successful!");
              Serial.printf("[TELNET] ✅ User '%s' authenticated (slot %d)\n", username.c_str(), i);
              sendTelnetPrompt(telnetClients[i]);
            } else {
              telnetClients[i].println("\nLogin failed!");
              Serial.printf("[TELNET] ❌ Failed login for '%s' (slot %d)\n", username.c_str(), i);
              telnetClients[i].stop();
            }
          }
        } else {
          // Process command
          if (line.length() > 0) {
            processTelnetCommand(telnetClients[i], line);
          }
          sendTelnetPrompt(telnetClients[i]);
        }
      }
    }
  }
}

int getClientIndex(WiFiClient *client) {
  for (int i = 0; i < 5; i++) {
    if (telnetClients[i] == *client) {
      return i;
    }
  }
  return -1;
}

void processTelnetCommand(WiFiClient &client, String cmd) {
  cmd.trim();
  String cmdLower = cmd;
  cmdLower.toLowerCase();
  
  // Find which client this is for logging
  String username = "unknown";
  for (int i = 0; i < 5; i++) {
    if (telnetClients[i] == client) {
      username = telnetUsernames[i];
      break;
    }
  }
  
  Serial.printf("[TELNET] %s@%s: %s\n", username.c_str(), client.remoteIP().toString().c_str(), cmd.c_str());
  
  // Check for privilege escalation commands first
  handlePrivilegeEscalation(client, cmd, getClientIndex(&client));
  
  if (cmdLower == "exit" || cmdLower == "quit" || cmdLower == "logout") {
    Serial.printf("[TELNET] %s disconnecting\n", username.c_str());
    client.println("Goodbye!");
    client.stop();
    return;
  }
  
  if (cmdLower == "help") {
    client.println("Available commands:");
    for (int i = 0; i < ALLOWED_COMMANDS_COUNT; i++) {
      client.println("  " + allowedCommands[i]);
    }
    client.println("  exit/quit - Close connection");
    return;
  }
  
  if (cmdLower == "ls" || cmdLower == "dir") {
    client.println("Files:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
      client.printf("  %-30s %8d bytes\n", file.name(), file.size());
      file = root.openNextFile();
    }
    return;
  }
  
  if (cmdLower == "pwd") {
    client.println("/");
    return;
  }
  
  if (cmdLower == "whoami") {
    // Find which client this is
    for (int i = 0; i < 5; i++) {
      if (telnetClients[i] == client) {
        client.println(telnetUsernames[i]);
        return;
      }
    }
    client.println("unknown");
    return;
  }
  
  if (cmdLower == "id") {
    if (username == "root") {
      client.println("uid=0(root) gid=0(root) groups=0(root)");
      client.println("✅ Running as ROOT - full system access!");
    } else if (username == "admin") {
      client.println("uid=1000(admin) gid=1000(admin) groups=1000(admin),27(sudo)");
      client.println("HINT: You have sudo group - try 'sudo -l'");
    } else {
      client.println("uid=1002(" + username + ") gid=1002(" + username + ") groups=1002(" + username + ")");
      client.println("HINT: Find SUID binaries: find / -perm -4000 2>/dev/null");
    }
    return;
  }
  
  if (cmdLower == "ps") {
    client.println("PID   CMD");
    client.println("1     init");
    client.println("123   webserver");
    client.println("124   telnet");
    client.println("125   websocket");
    return;
  }
  
  if (cmdLower == "netstat") {
    client.println("Active connections:");
    client.printf("HTTP Server:   %s:%d\n", getLocalIP().c_str(), HTTP_PORT);
    client.printf("Telnet Server: %s:%d\n", getLocalIP().c_str(), TELNET_PORT);
    client.printf("Active clients: %d\n", activeConnections);
    return;
  }
  
  if (cmdLower == "ifconfig") {
    client.printf("IP Address:  %s\n", getLocalIP().c_str());
    client.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
    client.printf("SSID:        %s\n", WiFi.SSID().c_str());
    client.printf("Signal:      %d dBm\n", WiFi.RSSI());
    return;
  }
  
  if (cmdLower == "date") {
    client.println(getCurrentTimestamp());
    return;
  }
  
  if (cmdLower == "uptime") {
    unsigned long uptime = millis() / 1000;
    client.printf("Uptime: %lu seconds (%lu minutes)\n", uptime, uptime / 60);
    return;
  }
  
  if (cmdLower == "free") {
    client.println("Memory:");
    client.printf("Total: %u bytes\n", ESP.getHeapSize());
    client.printf("Free:  %u bytes\n", ESP.getFreeHeap());
    client.printf("Used:  %u bytes\n", ESP.getHeapSize() - ESP.getFreeHeap());
    return;
  }
  
  if (cmdLower == "df") {
    client.println("Filesystem:");
    client.printf("Total: %u bytes\n", LittleFS.totalBytes());
    client.printf("Used:  %u bytes\n", LittleFS.usedBytes());
    client.printf("Free:  %u bytes\n", LittleFS.totalBytes() - LittleFS.usedBytes());
    return;
  }
  
  if (cmdLower.startsWith("cat ")) {
    String filename = cmd.substring(4);
    filename.trim();
    
    // Intentional vulnerability: Path traversal
    if (VULN_PATH_TRAVERSAL || !filename.startsWith("..")) {
      String content = readFile(filename);
      if (content != "") {
        client.println(content);
      } else {
        client.println("cat: " + filename + ": No such file or directory");
      }
    } else {
      client.println("cat: Permission denied");
    }
    return;
  }
  
  if (cmdLower.startsWith("echo ")) {
    String text = cmd.substring(5);
    client.println(text);
    return;
  }
  
  // Check if command is allowed
  bool isAllowed = false;
  for (int i = 0; i < ALLOWED_COMMANDS_COUNT; i++) {
    if (cmdLower.startsWith(allowedCommands[i])) {
      isAllowed = true;
      break;
    }
  }
  
  if (!isAllowed && VULN_COMMAND_INJECTION) {
    client.println("[VULNERABILITY] Command injection possible!");
    client.println("In production, this would execute: " + cmd);
    return;
  }
  
  client.println("Command not found: " + cmd);
}

void sendTelnetPrompt(WiFiClient &client) {
  client.print("esp32@hack:~$ ");
}

void disconnectTelnet(WiFiClient &client) {
  for (int i = 0; i < 5; i++) {
    if (telnetClients[i] == client) {
      telnetClients[i].stop();
      telnetAuthenticated[i] = false;
      telnetUsernames[i] = "";
      Serial.printf("[TELNET] Client #%d disconnected\n", i);
      break;
    }
  }
}
