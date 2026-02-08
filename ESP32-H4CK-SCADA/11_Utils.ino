/*
 * Utilities Module
 * 
 * General-purpose utility functions for string manipulation,
 * file operations, and common tasks.
 */

// Forward declarations
char hexToChar(char c);
char charToHex(char c);

String urlDecode(String input) {
  String decoded = "";
  char c;
  char code0;
  char code1;
  
  for (int i = 0; i < input.length(); i++) {
    c = input.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%') {
      i++;
      code0 = input.charAt(i);
      i++;
      code1 = input.charAt(i);
      c = (hexToChar(code0) << 4) | hexToChar(code1);
      decoded += c;
    } else {
      decoded += c;
    }
  }
  
  return decoded;
}

String urlEncode(String input) {
  String encoded = "";
  char c;
  
  for (int i = 0; i < input.length(); i++) {
    c = input.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      encoded += '%';
      encoded += charToHex(c >> 4);
      encoded += charToHex(c & 0x0F);
    }
  }
  
  return encoded;
}

char hexToChar(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

char charToHex(char c) {
  if (c < 10) return '0' + c;
  return 'A' + (c - 10);
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/pdf";
  else if (filename.endsWith(".zip")) return "application/zip";
  else if (filename.endsWith(".gz")) return "application/gzip";
  else if (filename.endsWith(".txt")) return "text/plain";
  return "application/octet-stream";
}

bool fileExists(String path) {
  return LittleFS.exists(path);
}

String readFile(String path) {
  if (!fileExists(path)) {
    return "";
  }
  
  File file = LittleFS.open(path, "r");
  if (!file) {
    return "";
  }
  
  // Safety limit: max 16KB file read
  size_t fileSize = file.size();
  if (fileSize > 16384) {
    Serial.printf("[UTILS] File too large: %s (%d bytes)\n", path.c_str(), fileSize);
    file.close();
    return "";
  }
  
  // Check heap before allocation
  if (ESP.getFreeHeap() < (fileSize + 5000)) {
    Serial.println("[UTILS] Insufficient heap for file read");
    file.close();
    return "";
  }
  
  String content = "";
  content.reserve(fileSize + 1);
  
  size_t bytesRead = 0;
  while (file.available() && bytesRead < fileSize) {
    content += (char)file.read();
    bytesRead++;
    // Yield every 256 bytes to prevent watchdog
    if (bytesRead % 256 == 0) yield();
  }
  
  file.close();
  return content;
}

bool writeFile(String path, String content) {
  File file = LittleFS.open(path, "w");
  if (!file) {
    return false;
  }
  
  file.print(content);
  file.close();
  
  return true;
}

bool appendFile(String path, String content) {
  File file = LittleFS.open(path, "a");
  if (!file) {
    return false;
  }
  
  file.print(content);
  file.close();
  
  return true;
}

bool deleteFile(String path) {
  if (!fileExists(path)) {
    return false;
  }
  
  return LittleFS.remove(path);
}

String listFiles(String path) {
  String fileList = "";
  File root = LittleFS.open(path);
  
  if (!root || !root.isDirectory()) {
    return fileList;
  }
  
  File file = root.openNextFile();
  while (file) {
    fileList += file.name();
    fileList += " (";
    fileList += String(file.size());
    fileList += " bytes)\n";
    file = root.openNextFile();
  }
  
  return fileList;
}

String getCurrentTimestamp() {
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  
  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  
  char timestamp[32];
  sprintf(timestamp, "%lu days, %02lu:%02lu:%02lu", days, hours, minutes, seconds);
  
  return String(timestamp);
}

String generateUUID() {
  // Simple UUID v4 generator (not cryptographically secure)
  String uuid = "";
  
  for (int i = 0; i < 32; i++) {
    if (i == 8 || i == 12 || i == 16 || i == 20) {
      uuid += "-";
    }
    uuid += String(random(0, 16), HEX);
  }
  
  return uuid;
}

String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < 1024 * 1024) {
    return String(bytes / 1024.0, 2) + " KB";
  } else if (bytes < 1024 * 1024 * 1024) {
    return String(bytes / (1024.0 * 1024.0), 2) + " MB";
  } else {
    return String(bytes / (1024.0 * 1024.0 * 1024.0), 2) + " GB";
  }
}

String formatTime(unsigned long milliseconds) {
  unsigned long seconds = milliseconds / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  
  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  
  String result = "";
  if (days > 0) result += String(days) + "d ";
  if (hours > 0) result += String(hours) + "h ";
  if (minutes > 0) result += String(minutes) + "m ";
  result += String(seconds) + "s";
  
  return result;
}

int getFreeHeapPercentage() {
  return (ESP.getFreeHeap() * 100) / ESP.getHeapSize();
}

int getFSUsagePercentage() {
  size_t total = LittleFS.totalBytes();
  if (total == 0) return 0;
  return (LittleFS.usedBytes() * 100) / total;
}

// Memory Management Helpers
bool hasEnoughMemory() {
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < MIN_FREE_HEAP) {
    Serial.printf("[MEMORY] Low heap warning: %d bytes (min: %d)\n", freeHeap, MIN_FREE_HEAP);
    return false;
  }
  return true;
}

void logMemoryStatus() {
  Serial.printf("[MEMORY] Free heap: %d bytes, Largest block: %d bytes\n", 
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
}
