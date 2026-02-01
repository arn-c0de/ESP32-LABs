/*
 * WebSocket Module
 * 
 * Provides WebSocket-based interactive shell functionality.
 * Allows real-time command execution through the browser.
 */

void initWebSocket() {
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  
  Serial.println("[WEBSOCKET] WebSocket handler initialized on /shell");
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                       AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("[WEBSOCKET] Client #%u connected from %s\n", 
                    client->id(), client->remoteIP().toString().c_str());
      client->text("ESP32-H4CK Shell v1.0\nType 'help' for available commands\n$ ");
      activeConnections++;
      break;
      
    case WS_EVT_DISCONNECT:
      Serial.printf("[WEBSOCKET] Client #%u disconnected\n", client->id());
      activeConnections--;
      break;
      
    case WS_EVT_DATA: {
      AwsFrameInfo *info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char*)data;
        message.trim();
        
        logDebug("WebSocket command: " + message);
        handleWebSocketMessage(client, message);
      }
      break;
    }
      
    case WS_EVT_ERROR:
      Serial.printf("[WEBSOCKET] Error from client #%u\n", client->id());
      break;
      
    default:
      break;
  }
}

void handleWebSocketMessage(AsyncWebSocketClient *client, String message) {
  if (message.length() == 0) {
    client->text("$ ");
    return;
  }
  
  // Execute command
  executeShellCommand(client, message);
  
  // Send prompt
  client->text("$ ");
}

void executeShellCommand(AsyncWebSocketClient *client, String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  
  // Check if command is allowed
  bool isAllowed = false;
  for (int i = 0; i < ALLOWED_COMMANDS_COUNT; i++) {
    if (cmd.startsWith(allowedCommands[i])) {
      isAllowed = true;
      break;
    }
  }
  
  if (!isAllowed && !VULN_COMMAND_INJECTION) {
    client->text("Error: Command not allowed\n");
    return;
  }
  
  // Process commands
  if (cmd == "help") {
    String help = "Available commands:\n";
    for (int i = 0; i < ALLOWED_COMMANDS_COUNT; i++) {
      help += "  " + allowedCommands[i] + "\n";
    }
    client->text(help);
  }
  else if (cmd == "ls" || cmd == "dir") {
    String output = "Files:\n";
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
      output += "  " + String(file.name()) + " (" + String(file.size()) + " bytes)\n";
      file = root.openNextFile();
    }
    client->text(output);
  }
  else if (cmd == "pwd") {
    client->text("/\n");
  }
  else if (cmd == "whoami") {
    client->text("esp32-user\n");
  }
  else if (cmd == "id") {
    client->text("uid=1000(esp32-user) gid=1000(esp32-user) groups=1000(esp32-user)\n");
  }
  else if (cmd == "ps") {
    client->text("PID   CMD\n");
    client->text("1     init\n");
    client->text("123   webserver\n");
    client->text("124   telnet\n");
    client->text("125   websocket\n");
  }
  else if (cmd == "netstat") {
    String output = "Active Internet connections:\n";
    output += "Proto Local Address           State\n";
    output += "tcp   " + getLocalIP() + ":80     LISTEN\n";
    output += "tcp   " + getLocalIP() + ":23     LISTEN\n";
    output += "tcp   " + getLocalIP() + ":8080   LISTEN\n";
    client->text(output);
  }
  else if (cmd == "ifconfig") {
    String output = "eth0: ";
    output += "IP=" + getLocalIP();
    output += " MAC=" + WiFi.macAddress();
    output += "\n";
    client->text(output);
  }
  else if (cmd == "date") {
    client->text(getCurrentTimestamp() + "\n");
  }
  else if (cmd == "uptime") {
    unsigned long uptime = millis() / 1000;
    String output = "Uptime: " + String(uptime) + " seconds\n";
    client->text(output);
  }
  else if (cmd == "free") {
    String output = "Memory:\n";
    output += "Total: " + String(ESP.getHeapSize()) + " bytes\n";
    output += "Free:  " + String(ESP.getFreeHeap()) + " bytes\n";
    output += "Used:  " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes\n";
    client->text(output);
  }
  else if (cmd == "df") {
    String output = "Filesystem statistics:\n";
    output += "Total: " + String(LittleFS.totalBytes()) + " bytes\n";
    output += "Used:  " + String(LittleFS.usedBytes()) + " bytes\n";
    output += "Free:  " + String(LittleFS.totalBytes() - LittleFS.usedBytes()) + " bytes\n";
    client->text(output);
  }
  else if (cmd.startsWith("cat ")) {
    String filename = cmd.substring(4);
    filename.trim();
    
    // Intentional vulnerability: Path traversal
    if (VULN_PATH_TRAVERSAL || !filename.startsWith("..")) {
      String content = readFile(filename);
      if (content != "") {
        client->text(content + "\n");
      } else {
        client->text("cat: " + filename + ": No such file or directory\n");
      }
    } else {
      client->text("cat: Permission denied\n");
    }
  }
  else if (cmd.startsWith("echo ")) {
    String text = cmd.substring(5);
    client->text(text + "\n");
  }
  else if (VULN_COMMAND_INJECTION) {
    // Intentional vulnerability: Command injection
    client->text("Executing: " + cmd + "\n");
    client->text("[VULNERABILITY] Command injection possible here!\n");
    client->text("In a real system, this would execute arbitrary commands.\n");
  }
  else {
    client->text("Command not found: " + cmd + "\n");
  }
}

void broadcastMessage(String message) {
  ws.textAll(message);
}
