/*
 * Authentication Module
 * 
 * Handles user authentication, session management, and JWT tokens.
 * Contains intentional vulnerabilities for educational purposes.
 */

// JwtPayload and AuthCacheEntry structs are now defined in main .ino file
static const int AUTH_CACHE_SIZE = 6;
static AuthCacheEntry authCache[AUTH_CACHE_SIZE];

static bool decodeJwtPayload(const String &payloadB64, JwtPayload &out) {
  String decoded = base64Decode(payloadB64);
  if (DEBUG_MODE) {
    Serial.printf("[AUTH] Decoded payload: %s\n", decoded.c_str());
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, decoded);
  if (error) {
    if (DEBUG_MODE) {
      Serial.printf("[AUTH] JSON parse error: %s\n", error.c_str());
    }
    return false;
  }

  out.username = doc["username"].as<String>();
  out.role = doc["role"].as<String>();
  out.exp = doc["exp"] | 0UL;
  if (DEBUG_MODE) {
    Serial.printf("[AUTH] Parsed role from JSON: %s\n", out.role.c_str());
  }
  return true;
}

static bool getCachedJwt(const String &token, JwtPayload &out) {
  unsigned long now = millis();
  for (int i = 0; i < AUTH_CACHE_SIZE; i++) {
    if (authCache[i].token == token && authCache[i].token.length() > 0) {
      if (authCache[i].payload.exp != 0 && now > authCache[i].payload.exp) {
        authCache[i].token = "";
        return false;
      }
      authCache[i].lastUsed = now;
      out = authCache[i].payload;
      return true;
    }
  }
  return false;
}

static void putCachedJwt(const String &token, const JwtPayload &payload) {
  if (token.length() == 0) return;
  unsigned long now = millis();
  int slot = 0;
  unsigned long oldest = authCache[0].lastUsed;
  for (int i = 0; i < AUTH_CACHE_SIZE; i++) {
    if (authCache[i].token.length() == 0) {
      slot = i;
      break;
    }
    if (authCache[i].lastUsed < oldest) {
      oldest = authCache[i].lastUsed;
      slot = i;
    }
  }
  authCache[slot].token = token;
  authCache[slot].payload = payload;
  authCache[slot].lastUsed = now;
}

static bool parseJwtToken(const String &token, JwtPayload &out) {
  if (token.length() == 0) return false;
  if (getCachedJwt(token, out)) return true;

  String payload;

  if (VULN_WEAK_AUTH && token.indexOf(".unsigned") > 0) {
    int firstDot = token.indexOf('.');
    int lastDot = token.lastIndexOf('.');
    if (firstDot >= 0 && lastDot > firstDot) {
      payload = token.substring(firstDot + 1, lastDot);
      if (DEBUG_MODE) {
        Serial.printf("[AUTH] Extracted payload from weak auth token (length: %d)\n", payload.length());
      }
    }
  } else {
    int dotPos = token.indexOf('.');
    if (dotPos <= 0) return false;
    String payloadB64 = token.substring(0, dotPos);
    String signature = token.substring(dotPos + 1);

    String expectedSignature = sha256Hash(payloadB64 + JWT_SECRET_STR);
    if (signature != expectedSignature) {
      return false;
    }
    payload = payloadB64;
    if (DEBUG_MODE) {
      Serial.printf("[AUTH] Extracted payload from secure token (length: %d)\n", payload.length());
    }
  }

  if (payload.length() == 0) return false;
  if (!decodeJwtPayload(payload, out)) return false;
  putCachedJwt(token, out);
  return true;
}

static bool getSessionFromRequest(AsyncWebServerRequest *request, Session &sessionOut) {
  if (!request->hasHeader("Cookie")) return false;
  String cookies = request->header("Cookie");
  int sessionPos = cookies.indexOf("session=");
  if (sessionPos < 0) return false;
  int endPos = cookies.indexOf(';', sessionPos);
  String sessionId = cookies.substring(sessionPos + 8, endPos > 0 ? endPos : cookies.length());
  if (activeSessions.find(sessionId) == activeSessions.end()) return false;

  Session &session = activeSessions[sessionId];
  if (millis() - session.lastActivity > SESSION_TIMEOUT) {
    activeSessions.erase(sessionId);
    return false;
  }
  session.lastActivity = millis();
  sessionOut = session;
  return true;
}

void initAuth() {
  // Clear any existing sessions
  activeSessions.clear();
  
  Serial.println("[AUTH] Authentication system initialized");
  Serial.printf("[AUTH] Session timeout: %d seconds\n", SESSION_TIMEOUT / 1000);
  Serial.printf("[AUTH] JWT expiry: %d seconds\n", JWT_EXPIRY);
}

bool authenticateUser(String username, String password, String *roleOut) {
  // Intentional vulnerability: No rate limiting on failed attempts
  failedLoginAttempts++;
  
  // Check default users (plaintext passwords - intentional vulnerability)
  for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
    if (defaultUsers[i].username == username) {
      if (defaultUsers[i].password == password) {
        logInfo("Successful login: " + username);
        failedLoginAttempts = 0;
        if (roleOut) *roleOut = defaultUsers[i].role;
        return true;
      } else {
        Serial.printf("[AUTH] Default user '%s' provided wrong password\n", username.c_str());
        // continue to allow checking DB users as well
      }
    }
  }
  
  // Check database users
  String userData = getUserByUsername(username);
  if (userData != "") {
    // Parse user data and verify password
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, userData);
    
    if (!error) {
      String storedPassword = doc["password"].as<String>();
      String storedRole = doc["role"].as<String>();
      
      // Intentional vulnerability: Simple string comparison (no hashing in default mode)
      if (VULN_WEAK_AUTH) {
        if (password == storedPassword) {
          logInfo("Successful login from DB: " + username);
          failedLoginAttempts = 0;
          if (roleOut && storedRole.length() > 0) *roleOut = storedRole;
          return true;
        } else {
          Serial.printf("[AUTH] DB user '%s' provided wrong password (weak auth)\n", username.c_str());
        }
      }
      
      // Secure path: Use password hashing (when vulnerability mode is off)
      if (!VULN_WEAK_AUTH) {
        if (verifyPassword(password, storedPassword)) {
          logInfo("Successful login (hashed): " + username);
          failedLoginAttempts = 0;
          if (roleOut && storedRole.length() > 0) *roleOut = storedRole;
          return true;
        } else {
          Serial.printf("[AUTH] DB user '%s' failed password verify (hashed)\n", username.c_str());
        }
      }
    }
  }
  
  logError("Failed login attempt: " + username);
  lastFailedLogin = millis();
  return false;
}

bool authenticateUser(String username, String password) {
  return authenticateUser(username, password, nullptr);
}

String generateJWT(String username, String role) {
  // Intentionally simple JWT implementation (vulnerable)
  DynamicJsonDocument payload(256);
  payload["username"] = username;
  payload["role"] = role;
  payload["exp"] = millis() + (JWT_EXPIRY * 1000);
  payload["iat"] = millis();
  
  String payloadStr;
  serializeJson(payload, payloadStr);
  
  // Intentional vulnerability: Weak signature (just base64, no real crypto)
  if (VULN_WEAK_AUTH) {
    String header = base64Encode("{\"alg\":\"none\",\"typ\":\"JWT\"}");
    String body = base64Encode(payloadStr);
    return header + "." + body + ".unsigned";  // No signature!
  }
  
  // Slightly better (but still weak for demonstration)
  String token = base64Encode(payloadStr);
  String signature = sha256Hash(token + JWT_SECRET_STR);
  return token + "." + signature;
}

bool validateJWT(String token) {
  if (token == "") return false;
  JwtPayload payload;
  if (!parseJwtToken(token, payload)) return false;
  if (payload.exp != 0 && millis() > payload.exp) return false;
  return true;
}

bool isAuthenticated(AsyncWebServerRequest *request) {
  // Check for Authorization header first
  if (request->hasHeader("Authorization")) {
    String authHeader = request->header("Authorization");
    if (authHeader.startsWith("Bearer ")) {
      String token = authHeader.substring(7);
      if (validateJWT(token)) {
        return true;
      }
    }
  }

  // Check for session cookie
  Session session;
  if (getSessionFromRequest(request, session)) {
    return true;
  }

  return false;
}

String getUserRole(String token) {
  JwtPayload payload;
  if (!parseJwtToken(token, payload)) return "guest";
  if (payload.exp != 0 && millis() > payload.exp) return "guest";
  return payload.role.length() ? payload.role : "guest";
}

String getRequestRole(AsyncWebServerRequest *request) {
  // Check Authorization header (JWT) first
  if (request->hasHeader("Authorization")) {
    String authHeader = request->header("Authorization");
    if (authHeader.startsWith("Bearer ")) {
      String token = authHeader.substring(7);
      if (DEBUG_MODE) {
        Serial.printf("[AUTH] JWT token length: %d\n", token.length());
      }
      JwtPayload payload;
      if (parseJwtToken(token, payload) && (payload.exp == 0 || millis() <= payload.exp)) {
        String role = payload.role.length() ? payload.role : "guest";
        if (DEBUG_MODE) {
          Serial.printf("[AUTH] Role from JWT: %s\n", role.c_str());
        }
        return role;
      }
    }
  }

  // Check session cookie
  Session session;
  if (getSessionFromRequest(request, session)) {
    if (DEBUG_MODE) {
      Serial.printf("[AUTH] Role from session: %s\n", session.role.c_str());
    }
    return session.role;
  }
  
  // Check role cookie
  if (request->hasHeader("Cookie")) {
    String cookies = request->header("Cookie");
    int pos = cookies.indexOf("auth_role=");
    if (pos >= 0) {
      int endPos = cookies.indexOf(';', pos);
      String role = cookies.substring(pos + 10, endPos > 0 ? endPos : cookies.length());
      if (DEBUG_MODE) {
        Serial.printf("[AUTH] Role from cookie: %s\n", role.c_str());
      }
      return role;
    }
  }
  
  if (DEBUG_MODE) {
    Serial.println("[AUTH] No role found, defaulting to guest");
  }
  return "guest";
}

bool isAdmin(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
    return false;
  }
  
  String role = getRequestRole(request);
  return (role == "admin");
}

bool requireAdmin(AsyncWebServerRequest *request) {
  // Honor global toggle: allow disabling admin checks for lab scenarios
  if (!PROTECT_ADMIN_ENDPOINTS) {
    Serial.printf("[AUTH] WARNING: Admin endpoint protection DISABLED - allowing access (INSECURE)\n");
    return true;
  }

  if (!isAuthenticated(request)) {
    Serial.printf("[AUTH] Admin access denied - not authenticated\n");
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return false;
  }
  
  if (!isAdmin(request)) {
    String role = getRequestRole(request);
    Serial.printf("[AUTH] Admin access denied - insufficient privileges (role: %s)\n", role.c_str());
    request->send(403, "application/json", "{\"error\":\"Admin privileges required\"}");
    return false;
  }
  
  Serial.printf("[AUTH] Admin access granted\n");
  return true;
}

void createSession(String username, String role, String ipAddress) {
  String sessionId = generateRandomToken(SESSION_ID_LENGTH);
  
  Session newSession;
  newSession.sessionId = sessionId;
  newSession.username = username;
  newSession.role = role;
  newSession.createdAt = millis();
  newSession.lastActivity = millis();
  newSession.ipAddress = ipAddress;
  
  activeSessions[sessionId] = newSession;
  
  logInfo("Session created for: " + username + " (ID: " + sessionId + ")");
}

void destroySession(String sessionId) {
  if (activeSessions.find(sessionId) != activeSessions.end()) {
    String username = activeSessions[sessionId].username;
    activeSessions.erase(sessionId);
    logInfo("Session destroyed for: " + username);
  }
}

void handleLoginJSON(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t total) {
  totalRequests++;
  
  String clientIP = request->client()->remoteIP().toString();
  if (rejectIfLowHeap(request)) {
    return;
  }
  if (rejectIfBodyTooLarge(request, total)) {
    return;
  }
  if (!tryReserveConnection(clientIP)) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  ConnectionGuard guard(true, clientIP);
  
  if (isIpBlocked(clientIP)) {
    request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
    return;
  }
  if (!checkRateLimit(clientIP)) {
    sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
    return;
  }
  if (!checkLoginBackoff(clientIP)) {
    request->send(429, "application/json", "{\"error\":\"Login backoff active\"}");
    return;
  }
  
  Serial.printf("[AUTH] Login attempt from %s\n", clientIP.c_str());
  
  String username = "";
  String password = "";
  
  // Parse JSON body
  if (ESP.getFreeHeap() < MIN_FREE_HEAP) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, (char*)data, len);
  
  if (error) {
    Serial.printf("[AUTH] JSON parse error: %s\n", error.c_str());
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  username = doc["username"].as<String>();
  password = doc["password"].as<String>();
  
  Serial.printf("[AUTH] User: %s, Pass: %s from %s\n", username.c_str(), password.c_str(), clientIP.c_str());
  
  // Authenticate
  String role = "guest";
  if (authenticateUser(username, password, &role)) {
    resetLoginFailures(clientIP);
    
    // Create session
    String ipAddress = request->client()->remoteIP().toString();
    createSession(username, role, ipAddress);
    Serial.printf("[AUTH] ✅ Login SUCCESS: %s (role: %s) from %s\n", username.c_str(), role.c_str(), ipAddress.c_str());
    
    // Generate JWT token
    String token = generateJWT(username, role);
    
    // Generate session cookie
    String sessionId = activeSessions.rbegin()->first;

    // API/fetch request - return JSON
    DynamicJsonDocument response(512);
    response["success"] = true;
    response["token"] = token;
    response["username"] = username;
    response["role"] = role;
    response["message"] = "Login successful";

    String responseStr;
    serializeJson(response, responseStr);

    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", responseStr);
    resp->addHeader("Set-Cookie", "session=" + sessionId + "; Path=/");
    resp->addHeader("Set-Cookie", "auth_token=" + token + "; Path=/");
    resp->addHeader("Set-Cookie", "auth_user=" + username + "; Path=/");
    resp->addHeader("Set-Cookie", "auth_role=" + role + "; Path=/");
    request->send(resp);
  } else {
    recordLoginFailure(clientIP);
    DynamicJsonDocument response(256);
    response["success"] = false;
    response["error"] = "Invalid credentials";
    response["failed_attempts"] = failedLoginAttempts;

    String responseStr;
    serializeJson(response, responseStr);
    request->send(401, "application/json", responseStr);
  }
}

void handleLogin(AsyncWebServerRequest *request) {
  totalRequests++;
  
  String clientIP = request->client()->remoteIP().toString();
  if (rejectIfLowHeap(request)) {
    return;
  }
  if (!tryReserveConnection(clientIP)) {
    request->send(503, "application/json", "{\"error\":\"Server busy\"}");
    return;
  }
  ConnectionGuard guard(true, clientIP);
  if (isIpBlocked(clientIP)) {
    request->send(403, "application/json", "{\"error\":\"Access Denied\"}");
    return;
  }
  if (!checkRateLimit(clientIP)) {
    sendRateLimited(request, "application/json", "{\"error\":\"Rate limit exceeded\"}");
    return;
  }
  if (!checkLoginBackoff(clientIP)) {
    request->send(429, "application/json", "{\"error\":\"Login backoff active\"}");
    return;
  }
  Serial.printf("[AUTH] Login attempt from %s\n", clientIP.c_str());
  
  if (request->method() != HTTP_POST) {
    request->send(405, "application/json", "{\"error\":\"Method not allowed\"}");
    return;
  }
  
  String username = "";
  String password = "";
  
  // Parse POST parameters
  if (request->hasParam("username", true)) {
    username = request->getParam("username", true)->value();
  }
  if (request->hasParam("password", true)) {
    password = request->getParam("password", true)->value();
  }
  
  Serial.printf("[AUTH] User: %s, Pass: %s from %s\n", username.c_str(), password.c_str(), clientIP.c_str());
  
  // Authenticate
  String role = "guest";
  if (authenticateUser(username, password, &role)) {
    resetLoginFailures(clientIP);
    
    // Create session
    String ipAddress = request->client()->remoteIP().toString();
    createSession(username, role, ipAddress);
    Serial.printf("[AUTH] ✅ Login SUCCESS: %s (role: %s) from %s\n", username.c_str(), role.c_str(), ipAddress.c_str());
    
    // Generate JWT token
    String token = generateJWT(username, role);
    
    // Generate session cookie
    String sessionId = activeSessions.rbegin()->first;

    // Check if this is a browser form submit (not fetch/XHR)
    bool isBrowserForm = false;
    if (request->hasHeader("Accept")) {
      String accept = request->header("Accept");
      isBrowserForm = accept.indexOf("text/html") >= 0 && accept.indexOf("application/json") < 0;
    }

    if (isBrowserForm) {
      // Browser form submit - redirect with session cookie and store token in cookie
      String redirectUrl = "/dashboard";
      AsyncWebServerResponse *resp = request->beginResponse(302, "text/html", "Redirecting...");
      resp->addHeader("Location", redirectUrl);
      resp->addHeader("Set-Cookie", "session=" + sessionId + "; Path=/");
      resp->addHeader("Set-Cookie", "auth_token=" + token + "; Path=/");
      resp->addHeader("Set-Cookie", "auth_user=" + username + "; Path=/");
      resp->addHeader("Set-Cookie", "auth_role=" + role + "; Path=/");
      request->send(resp);
    } else {
      // API/fetch request - return JSON
      DynamicJsonDocument response(512);
      response["success"] = true;
      response["token"] = token;
      response["username"] = username;
      response["role"] = role;
      response["message"] = "Login successful";

      String responseStr;
      serializeJson(response, responseStr);

      AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", responseStr);
      resp->addHeader("Set-Cookie", "session=" + sessionId + "; Path=/");
      resp->addHeader("Set-Cookie", "auth_token=" + token + "; Path=/");
      resp->addHeader("Set-Cookie", "auth_user=" + username + "; Path=/");
      resp->addHeader("Set-Cookie", "auth_role=" + role + "; Path=/");
      request->send(resp);
    }
  } else {
    recordLoginFailure(clientIP);
    // Check if browser form submit
    bool isBrowserForm = false;
    if (request->hasHeader("Accept")) {
      String accept = request->header("Accept");
      isBrowserForm = accept.indexOf("text/html") >= 0 && accept.indexOf("application/json") < 0;
    }

    if (isBrowserForm) {
      // Redirect back to login page with error
      AsyncWebServerResponse *resp = request->beginResponse(302, "text/html", "Redirecting...");
      resp->addHeader("Location", "/login?error=1");
      request->send(resp);
    } else {
      DynamicJsonDocument response(256);
      response["success"] = false;
      response["error"] = "Invalid credentials";
      response["failed_attempts"] = failedLoginAttempts;

      String responseStr;
      serializeJson(response, responseStr);
      request->send(401, "application/json", responseStr);
    }
  }
}

void handleLogout(AsyncWebServerRequest *request) {
  totalRequests++;
  
  // Destroy session
  if (request->hasHeader("Cookie")) {
    String cookies = request->header("Cookie");
    int sessionPos = cookies.indexOf("session=");
    if (sessionPos >= 0) {
      int endPos = cookies.indexOf(';', sessionPos);
      String sessionId = cookies.substring(sessionPos + 8, endPos > 0 ? endPos : cookies.length());
      destroySession(sessionId);
    }
  }
  
  DynamicJsonDocument response(128);
  response["success"] = true;
  response["message"] = "Logged out successfully";
  
  String responseStr;
  serializeJson(response, responseStr);
  request->send(200, "application/json", responseStr);
}

// Get username from request (from JWT or session)
String getRequestUsername(AsyncWebServerRequest *request) {
  // Try Authorization header (JWT)
  if (request->hasHeader("Authorization")) {
    String authHeader = request->header("Authorization");
    if (authHeader.startsWith("Bearer ")) {
      String token = authHeader.substring(7);
      JwtPayload payload;
      if (parseJwtToken(token, payload)) {
        if (payload.exp == 0 || millis() <= payload.exp) {
          return payload.username;
        }
      }
    }
  }

  // Try session cookie
  Session session;
  if (getSessionFromRequest(request, session)) {
    return session.username;
  }

  return "";
}

// Extract username and role from request (session or JWT)
bool extractSession(AsyncWebServerRequest *request, String &username, String &role) {
  username = getRequestUsername(request);
  role = getRequestRole(request);
  return (username != "" && role != "guest");
}

// Check if user has minimum required role
// Role hierarchy: viewer < maintenance < operator < admin
bool requireRole(AsyncWebServerRequest *request, const char* minRole) {
  if (!isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return false;
  }
  
  String userRole = getRequestRole(request);
  
  // Define role hierarchy
  int roleLevel = 0;
  if (userRole == "admin") roleLevel = 3;
  else if (userRole == "operator") roleLevel = 2;
  else if (userRole == "maintenance") roleLevel = 1;
  else roleLevel = 0; // viewer
  
  int minLevel = 0;
  String minRoleStr = String(minRole);
  if (minRoleStr == "admin") minLevel = 3;
  else if (minRoleStr == "operator") minLevel = 2;
  else if (minRoleStr == "maintenance") minLevel = 1;
  else minLevel = 0;
  
  if (roleLevel < minLevel) {
    Serial.printf("[AUTH] Insufficient role: %s (required: %s)\n", userRole.c_str(), minRole);
    request->send(403, "application/json", "{\"error\":\"Insufficient permissions\"}");
    return false;
  }
  
  return true;
}
