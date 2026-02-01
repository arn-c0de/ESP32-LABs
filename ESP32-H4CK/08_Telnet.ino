/*
 * Telnet Module
 * 
 * Provides telnet service for remote shell access.
 * Intentionally vulnerable for training purposes (cleartext, weak auth).
 */

WiFiClient telnetClients[5];  // Support up to 5 concurrent telnet clients
bool telnetAuthenticated[5] = {false, false, false, false, false};
String telnetUsernames[5];
String telnetLineBuffer[5];       // Per-client line buffer for character-by-character input
bool telnetInEscSeq[5] = {false}; // Per-client escape sequence tracking
int telnetEscLen[5] = {0};        // Length of current escape sequence
bool telnetReadingPassword[5] = {false}; // Password input mode (no echo)

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

// Send telnet negotiation to enable character mode
void sendTelnetNegotiation(WiFiClient &client) {
  // IAC WILL ECHO - server will handle echoing
  uint8_t willEcho[] = {0xFF, 0xFB, 0x01};
  client.write(willEcho, 3);
  // IAC WILL SUPPRESS-GO-AHEAD
  uint8_t willSGA[] = {0xFF, 0xFB, 0x03};
  client.write(willSGA, 3);
  // IAC DO SUPPRESS-GO-AHEAD
  uint8_t doSGA[] = {0xFF, 0xFD, 0x03};
  client.write(doSGA, 3);
  // IAC DONT LINEMODE
  uint8_t dontLinemode[] = {0xFF, 0xFE, 0x22};
  client.write(dontLinemode, 3);
}

// Process a completed line for a client slot
void processCompletedLine(int i) {
  String line = telnetLineBuffer[i];
  telnetLineBuffer[i] = "";

  if (!telnetAuthenticated[i]) {
    // Handle authentication
    if (!telnetReadingPassword[i]) {
      // Reading username
      telnetUsernames[i] = line;
      telnetReadingPassword[i] = true;
      telnetClients[i].print("\r\nPassword: ");
    } else {
      // Reading password
      telnetReadingPassword[i] = false;
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
        telnetClients[i].println("\r\nLogin successful!");
        Serial.printf("[TELNET] ✅ User '%s' authenticated (slot %d)\n", username.c_str(), i);
        sendTelnetPrompt(telnetClients[i]);
      } else {
        telnetClients[i].println("\r\nLogin failed!");
        Serial.printf("[TELNET] ❌ Failed login for '%s' (slot %d)\n", username.c_str(), i);
        telnetClients[i].stop();
      }
    }
  } else {
    telnetClients[i].print("\r\n");
    // Process command
    if (line.length() > 0) {
      processTelnetCommand(telnetClients[i], line);
    }
    if (telnetClients[i].connected()) {
      sendTelnetPrompt(telnetClients[i]);
    }
  }
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
        telnetLineBuffer[i] = "";
        telnetInEscSeq[i] = false;
        telnetEscLen[i] = 0;
        telnetReadingPassword[i] = false;
        freeSlot = i;

        Serial.printf("[TELNET] ✅ New client #%d from %s\n",
                      i, telnetClients[i].remoteIP().toString().c_str());

        sendTelnetNegotiation(telnetClients[i]);
        delay(50);

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

  // Handle existing clients - character-by-character processing
  for (int i = 0; i < 5; i++) {
    if (telnetClients[i] && telnetClients[i].connected()) {
      while (telnetClients[i].available()) {
        uint8_t c = telnetClients[i].read();

        // Handle telnet IAC commands (0xFF)
        if (c == 0xFF) {
          // Read and discard the 2-3 byte telnet command
          if (telnetClients[i].available()) telnetClients[i].read();
          if (telnetClients[i].available()) telnetClients[i].read();
          continue;
        }

        // Handle escape sequences (arrow keys, etc.)
        if (c == 0x1B) {  // ESC
          telnetInEscSeq[i] = true;
          telnetEscLen[i] = 0;
          continue;
        }
        if (telnetInEscSeq[i]) {
          telnetEscLen[i]++;
          // Consume escape sequence (typically ESC [ X for arrow keys)
          if (telnetEscLen[i] >= 2 || (c >= 'A' && c <= 'z')) {
            telnetInEscSeq[i] = false;
          }
          continue;
        }

        // Handle backspace (0x7F DEL or 0x08 BS)
        if (c == 0x7F || c == 0x08) {
          if (telnetLineBuffer[i].length() > 0) {
            telnetLineBuffer[i].remove(telnetLineBuffer[i].length() - 1);
            // Send backspace-space-backspace to erase character on screen
            if (!telnetReadingPassword[i]) {
              telnetClients[i].write("\b \b", 3);
            }
          }
          continue;
        }

        // Handle Enter (CR or LF)
        if (c == '\r' || c == '\n') {
          // Ignore LF after CR
          if (c == '\r') {
            // Peek for LF and consume it
            delay(2);
            if (telnetClients[i].available()) {
              uint8_t next = telnetClients[i].peek();
              if (next == '\n') telnetClients[i].read();
            }
          }
          if (c == '\n' && telnetLineBuffer[i].length() == 0) {
            // Might be trailing LF, send prompt
            if (telnetAuthenticated[i]) {
              telnetClients[i].print("\r\n");
              sendTelnetPrompt(telnetClients[i]);
            }
            continue;
          }
          processCompletedLine(i);
          continue;
        }

        // Ignore other control characters
        if (c < 0x20) continue;

        // Normal printable character - add to buffer and echo
        telnetLineBuffer[i] += (char)c;
        if (!telnetReadingPassword[i]) {
          telnetClients[i].write(c);  // Echo character back
        } else {
          telnetClients[i].write('*');  // Mask password
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
  String username = "esp32";
  for (int i = 0; i < 5; i++) {
    if (telnetClients[i] == client && telnetUsernames[i].length() > 0) {
      username = telnetUsernames[i];
      break;
    }
  }
  client.print(username + "@hack:~$ ");
}

void disconnectTelnet(WiFiClient &client) {
  for (int i = 0; i < 5; i++) {
    if (telnetClients[i] == client) {
      telnetClients[i].stop();
      telnetAuthenticated[i] = false;
      telnetUsernames[i] = "";
      telnetLineBuffer[i] = "";
      telnetInEscSeq[i] = false;
      telnetEscLen[i] = 0;
      telnetReadingPassword[i] = false;
      Serial.printf("[TELNET] Client #%d disconnected\n", i);
      break;
    }
  }
}
