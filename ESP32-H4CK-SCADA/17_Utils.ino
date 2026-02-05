// ============================================================
// 17_Utils.ino — Helpers (JSON, crypto, strings, time)
// ============================================================

// ===== HMAC-SHA256 =====
String hmacSHA256(const String& message, const String& key) {
  byte hmacResult[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key.c_str(), key.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)message.c_str(), message.length());
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);

  String hex = "";
  for (int i = 0; i < 32; i++) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02x", hmacResult[i]);
    hex += buf;
  }
  return hex;
}

// ===== Simple SHA256 hash =====
String sha256Hash(const String& input) {
  byte hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char*)input.c_str(), input.length());
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  String hex = "";
  for (int i = 0; i < 32; i++) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02x", hash[i]);
    hex += buf;
  }
  return hex;
}

// ===== Base64 Encode (simple) =====
static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String base64Encode(const String& input) {
  String out = "";
  int i = 0;
  int len = input.length();
  unsigned char a3[3], a4[4];

  while (len--) {
    a3[i++] = *(input.c_str() + (input.length() - len - 1));
    if (i == 3) {
      a4[0] = (a3[0] & 0xfc) >> 2;
      a4[1] = ((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4);
      a4[2] = ((a3[1] & 0x0f) << 2) | ((a3[2] & 0xc0) >> 6);
      a4[3] = a3[2] & 0x3f;
      for (i = 0; i < 4; i++) out += b64chars[a4[i]];
      i = 0;
    }
  }

  if (i) {
    for (int j = i; j < 3; j++) a3[j] = '\0';
    a4[0] = (a3[0] & 0xfc) >> 2;
    a4[1] = ((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4);
    a4[2] = ((a3[1] & 0x0f) << 2) | ((a3[2] & 0xc0) >> 6);
    for (int j = 0; j < i + 1; j++) out += b64chars[a4[j]];
    while (i++ < 3) out += '=';
  }
  return out;
}

// ===== Base64 Decode =====
String base64Decode(const String& input) {
  String out = "";
  int i = 0, in_len = input.length();
  unsigned char a3[3], a4[4];

  while (in_len-- && input[input.length() - in_len - 1] != '=') {
    char c = input[input.length() - in_len - 1];
    const char* p = strchr(b64chars, c);
    if (!p) break;
    a4[i++] = p - b64chars;
    if (i == 4) {
      a3[0] = (a4[0] << 2) | ((a4[1] & 0x30) >> 4);
      a3[1] = ((a4[1] & 0x0f) << 4) | ((a4[2] & 0x3c) >> 2);
      a3[2] = ((a4[2] & 0x03) << 6) | a4[3];
      for (i = 0; i < 3; i++) out += (char)a3[i];
      i = 0;
    }
  }
  if (i) {
    for (int j = i; j < 4; j++) a4[j] = 0;
    a3[0] = (a4[0] << 2) | ((a4[1] & 0x30) >> 4);
    a3[1] = ((a4[1] & 0x0f) << 4) | ((a4[2] & 0x3c) >> 2);
    for (int j = 0; j < i - 1; j++) out += (char)a3[j];
  }
  return out;
}

// ===== Random float in range =====
float randomFloat(float minVal, float maxVal) {
  return minVal + (float)random(10000) / 10000.0f * (maxVal - minVal);
}

// ===== Gaussian noise (Box-Muller) =====
float gaussianNoise(float mean, float stddev) {
  static bool hasSpare = false;
  static float spare;
  if (hasSpare) {
    hasSpare = false;
    return spare * stddev + mean;
  }
  hasSpare = true;
  float u, v, s;
  do {
    u = randomFloat(-1.0f, 1.0f);
    v = randomFloat(-1.0f, 1.0f);
    s = u * u + v * v;
  } while (s >= 1.0f || s == 0.0f);
  s = sqrtf(-2.0f * logf(s) / s);
  spare = v * s;
  return mean + stddev * u * s;
}

// ===== Timestamp (millis-based, formatted) =====
String getTimestamp() {
  unsigned long sec = millis() / 1000;
  int h = (sec / 3600) % 24;
  int m = (sec / 60) % 60;
  int s = sec % 60;
  char buf[20];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
  return String(buf);
}

// ===== ISO-like timestamp =====
String getISOTimestamp() {
  unsigned long sec = millis() / 1000;
  char buf[30];
  snprintf(buf, sizeof(buf), "2026-01-01T%02d:%02d:%02dZ",
    (int)((sec / 3600) % 24), (int)((sec / 60) % 60), (int)(sec % 60));
  return String(buf);
}

// ===== Generate unique ID =====
String generateId(const char* prefix) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%s-%04X%04X", prefix,
    (uint16_t)(esp_random() >> 16), (uint16_t)(esp_random() & 0xFFFF));
  return String(buf);
}

// ===== Generate session ID =====
String generateSessionId() {
  return generateId("sess");
}

// ===== URL decode =====
String urlDecode(const String& str) {
  String decoded = "";
  for (int i = 0; i < (int)str.length(); i++) {
    if (str[i] == '%' && i + 2 < (int)str.length()) {
      char hex[3] = {str[i+1], str[i+2], 0};
      decoded += (char)strtol(hex, NULL, 16);
      i += 2;
    } else if (str[i] == '+') {
      decoded += ' ';
    } else {
      decoded += str[i];
    }
  }
  return decoded;
}

// ===== String split =====
int stringSplit(const String& input, char delimiter, String* results, int maxResults) {
  int count = 0;
  int start = 0;
  for (int i = 0; i <= (int)input.length() && count < maxResults; i++) {
    if (i == (int)input.length() || input[i] == delimiter) {
      results[count++] = input.substring(start, i);
      start = i + 1;
    }
  }
  return count;
}

// ===== JSON error response =====
String jsonError(const String& message, int code) {
  JsonDocument doc;
  doc["error"]   = true;
  doc["message"] = message;
  doc["code"]    = code;
  String out;
  serializeJson(doc, out);
  return out;
}

// ===== JSON success response =====
String jsonSuccess(const String& message) {
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = message;
  String out;
  serializeJson(doc, out);
  return out;
}

// ===== Clamp value =====
float clampf(float val, float minVal, float maxVal) {
  if (val < minVal) return minVal;
  if (val > maxVal) return maxVal;
  return val;
}

// ===== Lerp =====
float lerpf(float a, float b, float t) {
  return a + (b - a) * clampf(t, 0.0f, 1.0f);
}

// ===== Debug print =====
void debugLog(const char* module, const char* message) {
  if (SERIAL_DEBUG) {
    Serial.printf("[%s] %s %s\n", module, getTimestamp().c_str(), message);
  }
}

void debugLogf(const char* module, const char* fmt, ...) {
  if (SERIAL_DEBUG) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.printf("[%s] %s %s\n", module, getTimestamp().c_str(), buf);
  }
}
