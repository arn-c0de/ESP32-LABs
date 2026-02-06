// ============================================================
// 04_Auth.ino — JWT/Session + Role-Based Access Control
// ============================================================

#include "include/common.h"

// Roles & Session are defined in include/common.h

#define MAX_SESSIONS 16
Session sessions[MAX_SESSIONS];
int sessionCount = 0;

// ===== Default users =====
struct User {
  const char* username;
  const char* password;
  UserRole    role;
};

User defaultUsers[] = {
  {"admin",       "admin123",       ROLE_ADMIN},
  {"operator",    "changeme",       ROLE_OPERATOR},
  {"maintenance", "m4int3n@nc3",    ROLE_MAINTENANCE},
  {"viewer",      "viewer",         ROLE_VIEWER},
  {"scada",       "scada2026",      ROLE_OPERATOR}
};
const int NUM_DEFAULT_USERS = 5;

void authInit() {
  Serial.println("[AUTH] Initializing authentication...");
  memset(sessions, 0, sizeof(sessions));
  sessionCount = 0;
  Serial.printf("[AUTH] %d default users configured.\n", NUM_DEFAULT_USERS);
}

// ===== Role to string =====
const char* roleToString(UserRole role) {
  switch (role) {
    case ROLE_ADMIN:       return "admin";
    case ROLE_OPERATOR:    return "operator";
    case ROLE_MAINTENANCE: return "maintenance";
    case ROLE_VIEWER:      return "viewer";
    default:               return "none";
  }
}

UserRole stringToRole(const String& str) {
  if (str == "admin")       return ROLE_ADMIN;
  if (str == "operator")    return ROLE_OPERATOR;
  if (str == "maintenance") return ROLE_MAINTENANCE;
  if (str == "viewer")      return ROLE_VIEWER;
  return ROLE_NONE;
}

// ===== Authenticate user =====
UserRole authenticateUser(const String& username, const String& password) {
  for (int i = 0; i < NUM_DEFAULT_USERS; i++) {
    if (username == defaultUsers[i].username && password == defaultUsers[i].password) {
      return defaultUsers[i].role;
    }
  }
  return ROLE_NONE;
}

// ===== Create session =====
String createSession(const String& username, UserRole role, const String& ip) {
  // Find empty slot or reuse oldest
  int slot = -1;
  unsigned long oldest = ULONG_MAX;
  int oldestIdx = 0;

  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (!sessions[i].active) { slot = i; break; }
    if (sessions[i].createdAt < oldest) {
      oldest = sessions[i].createdAt;
      oldestIdx = i;
    }
  }
  if (slot < 0) slot = oldestIdx;

  sessions[slot].sessionId    = generateSessionId();
  sessions[slot].username     = username;
  sessions[slot].role         = role;
  sessions[slot].ip           = ip;
  sessions[slot].createdAt    = millis();
  sessions[slot].lastActivity = millis();
  sessions[slot].active       = true;
  if (sessionCount < MAX_SESSIONS) sessionCount++;

  debugLogf("AUTH", "Session created: %s user=%s role=%s ip=%s",
    sessions[slot].sessionId.c_str(), username.c_str(),
    roleToString(role), ip.c_str());

  return sessions[slot].sessionId;
}

// ===== Validate session =====
Session* getSession(const String& sessionId) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].active && sessions[i].sessionId == sessionId) {
      sessions[i].lastActivity = millis();
      return &sessions[i];
    }
  }
  return nullptr;
}

// ===== Destroy session =====
void destroySession(const String& sessionId) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].active && sessions[i].sessionId == sessionId) {
      sessions[i].active = false;
      debugLogf("AUTH", "Session destroyed: %s", sessionId.c_str());
      return;
    }
  }
}

// ===== Destroy sessions by IP =====
void destroySessionsByIP(const String& ip) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].active && sessions[i].ip == ip) {
      sessions[i].active = false;
    }
  }
  debugLogf("AUTH", "Sessions destroyed for IP: %s", ip.c_str());
}

// ===== Simple JWT generation =====
String generateJWT(const String& username, UserRole role) {
  // Header
  String header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
  String headerB64 = base64Encode(header);

  // Payload
  JsonDocument doc;
  doc["sub"]  = username;
  doc["role"] = roleToString(role);
  doc["iat"]  = millis() / 1000;
  doc["exp"]  = millis() / 1000 + 3600;
  String payload;
  serializeJson(doc, payload);
  String payloadB64 = base64Encode(payload);

  // Signature
  String sigInput = headerB64 + "." + payloadB64;
  String sig = hmacSHA256(sigInput, String(JWT_SECRET));

  return headerB64 + "." + payloadB64 + "." + sig;
}

// ===== Validate JWT =====
bool validateJWT(const String& token, String& username, UserRole& role) {
  int dot1 = token.indexOf('.');
  int dot2 = token.indexOf('.', dot1 + 1);
  if (dot1 < 0 || dot2 < 0) return false;

  String headerB64  = token.substring(0, dot1);
  String payloadB64 = token.substring(dot1 + 1, dot2);
  String sig        = token.substring(dot2 + 1);

  // Verify signature
  String sigInput = headerB64 + "." + payloadB64;
  String expected = hmacSHA256(sigInput, String(JWT_SECRET));
  if (sig != expected) return false;

  // Decode payload
  String payload = base64Decode(payloadB64);
  JsonDocument doc;
  if (deserializeJson(doc, payload)) return false;

  username = doc["sub"].as<String>();
  role     = stringToRole(doc["role"].as<String>());
  return true;
}

// ===== Extract auth from request =====
UserRole getRequestRole(AsyncWebServerRequest* request) {
  // Check cookie session
  if (request->hasHeader("Cookie")) {
    String cookies = request->header("Cookie");
    int idx = cookies.indexOf("session=");
    if (idx >= 0) {
      String sessId = cookies.substring(idx + 8);
      int end = sessId.indexOf(';');
      if (end > 0) sessId = sessId.substring(0, end);
      Session* sess = getSession(sessId);
      if (sess) return sess->role;
    }
  }

  // Check Authorization header (Bearer JWT)
  if (request->hasHeader("Authorization")) {
    String auth = request->header("Authorization");
    if (auth.startsWith("Bearer ")) {
      String token = auth.substring(7);
      String user;
      UserRole role;
      if (validateJWT(token, user, role)) return role;
    }
  }

  // Check X-Session header
  if (request->hasHeader("X-Session")) {
    Session* sess = getSession(request->header("X-Session"));
    if (sess) return sess->role;
  }

  return ROLE_NONE;
}

// ===== Get session from request =====
Session* getRequestSession(AsyncWebServerRequest* request) {
  if (request->hasHeader("Cookie")) {
    String cookies = request->header("Cookie");
    int idx = cookies.indexOf("session=");
    if (idx >= 0) {
      String sessId = cookies.substring(idx + 8);
      int end = sessId.indexOf(';');
      if (end > 0) sessId = sessId.substring(0, end);
      return getSession(sessId);
    }
  }
  if (request->hasHeader("X-Session")) {
    return getSession(request->header("X-Session"));
  }
  return nullptr;
}

// ===== Auth status JSON =====
String authStatusJson() {
  JsonDocument doc;
  doc["active_sessions"] = sessionCount;
  JsonArray arr = doc["sessions"].to<JsonArray>();
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].active) {
      JsonObject s = arr.add<JsonObject>();
      s["id"]       = sessions[i].sessionId;
      s["user"]     = sessions[i].username;
      s["role"]     = roleToString(sessions[i].role);
      s["ip"]       = sessions[i].ip;
      s["age_sec"]  = (millis() - sessions[i].createdAt) / 1000;
    }
  }
  String out;
  serializeJson(doc, out);
  return out;
}
