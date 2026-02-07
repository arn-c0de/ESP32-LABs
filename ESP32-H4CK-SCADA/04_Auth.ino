/*
 * Authentication Module
 * 
 * Handles user authentication, session management, and JWT tokens.
 * Contains intentional vulnerabilities for educational purposes.
 */

void initAuth() {
  // Clear any existing sessions
  activeSessions.clear();
  
  Serial.println("[AUTH] Authentication system initialized");
  Serial.printf("[AUTH] Session timeout: %d seconds\n", SESSION_TIMEOUT / 1000);
  Serial.printf("[AUTH] JWT expiry: %d seconds\n", JWT_EXPIRY);
}

bool authenticateUser(String username, String password) {
  // Intentional vulnerability: No rate limiting on failed attempts
  failedLoginAttempts++;
  
  // Check default users (plaintext passwords - intentional vulnerability)
  for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
    if (defaultUsers[i].username == username) {
      if (defaultUsers[i].password == password) {
        logInfo("Successful login: " + username);
        failedLoginAttempts = 0;
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
      
      // Intentional vulnerability: Simple string comparison (no hashing in default mode)
      if (VULN_WEAK_AUTH) {
        if (password == storedPassword) {
          logInfo("Successful login from DB: " + username);
          failedLoginAttempts = 0;
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
  
  // Intentional vulnerability: Accept unsigned tokens
  if (VULN_WEAK_AUTH && token.indexOf(".unsigned") > 0) {
    return true;  // Accept any unsigned token!
  }
  
  // Basic validation
  int dotPos = token.indexOf('.');
  if (dotPos < 0) return false;
  
  String payload = token.substring(0, dotPos);
  String signature = token.substring(dotPos + 1);
  
  // Verify signature
  String expectedSignature = sha256Hash(payload + JWT_SECRET_STR);
  if (signature != expectedSignature) {
    return false;
  }
  
  // Decode and check expiry
  String decoded = base64Decode(payload);
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, decoded);
  
  if (error) return false;
  
  unsigned long exp = doc["exp"];
  if (millis() > exp) {
    return false;  // Token expired
  }
  
  return true;
}

bool isAuthenticated(AsyncWebServerRequest *request) {
  // Check for Authorization header
  if (request->hasHeader("Authorization")) {
    String authHeader = request->header("Authorization");
    if (authHeader.startsWith("Bearer ")) {
      String token = authHeader.substring(7);
      return validateJWT(token);
    }
  }
  
  // Check for session cookie
  if (request->hasHeader("Cookie")) {
    String cookies = request->header("Cookie");
    int sessionPos = cookies.indexOf("session=");
    if (sessionPos >= 0) {
      int endPos = cookies.indexOf(';', sessionPos);
      String sessionId = cookies.substring(sessionPos + 8, endPos > 0 ? endPos : cookies.length());
      
      // Check if session exists and is valid
      if (activeSessions.find(sessionId) != activeSessions.end()) {
        Session &session = activeSessions[sessionId];
        
        // Check if session expired
        if (millis() - session.lastActivity > SESSION_TIMEOUT) {
          activeSessions.erase(sessionId);
          return false;
        }
        
        // Update last activity
        session.lastActivity = millis();
        return true;
      }
    }
  }
  
  return false;
}

String getUserRole(String token) {
  // Extract payload part based on token format
  String payload;
  
  if (token.indexOf(".unsigned") > 0) {
    // VULN_WEAK_AUTH format: header.body.unsigned
    int firstDot = token.indexOf('.');
    int lastDot = token.lastIndexOf('.');
    if (firstDot >= 0 && lastDot > firstDot) {
      payload = token.substring(firstDot + 1, lastDot);
      Serial.printf("[AUTH] Extracted payload from weak auth token (length: %d)\n", payload.length());
    }
  } else {
    // Secure format: payload.signature
    int dotPos = token.indexOf('.');
    if (dotPos > 0) {
      payload = token.substring(0, dotPos);
      Serial.printf("[AUTH] Extracted payload from secure token (length: %d)\n", payload.length());
    } else {
      payload = token; // fallback
      Serial.printf("[AUTH] Using entire token as payload (fallback, length: %d)\n", payload.length());
    }
  }
  
  String decoded = base64Decode(payload);
  Serial.printf("[AUTH] Decoded payload: %s\n", decoded.c_str());
  
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, decoded);
  
  if (!error) {
    String role = doc["role"].as<String>();
    Serial.printf("[AUTH] Parsed role from JSON: %s\n", role.c_str());
    return role;
  } else {
    Serial.printf("[AUTH] JSON parse error: %s\n", error.c_str());
  }
  
  return "guest";
}

String getRequestRole(AsyncWebServerRequest *request) {
  // Check session cookie first
  if (request->hasHeader("Cookie")) {
    String cookies = request->header("Cookie");
    int sessionPos = cookies.indexOf("session=");
    if (sessionPos >= 0) {
      int endPos = cookies.indexOf(';', sessionPos);
      String sessionId = cookies.substring(sessionPos + 8, endPos > 0 ? endPos : cookies.length());
      
      if (activeSessions.find(sessionId) != activeSessions.end()) {
        String role = activeSessions[sessionId].role;
        Serial.printf("[AUTH] Role from session: %s\n", role.c_str());
        return role;
      }
    }
  }
  
  // Check Authorization header (JWT)
  if (request->hasHeader("Authorization")) {
    String authHeader = request->header("Authorization");
    if (authHeader.startsWith("Bearer ")) {
      String token = authHeader.substring(7);
      Serial.printf("[AUTH] JWT token length: %d\n", token.length());
      if (validateJWT(token)) {
        String role = getUserRole(token);
        Serial.printf("[AUTH] Role from JWT: %s\n", role.c_str());
        return role;
      } else {
        Serial.println("[AUTH] JWT validation failed");
      }
    }
  }
  
  // Check role cookie
  if (request->hasHeader("Cookie")) {
    String cookies = request->header("Cookie");
    int pos = cookies.indexOf("auth_role=");
    if (pos >= 0) {
      int endPos = cookies.indexOf(';', pos);
      String role = cookies.substring(pos + 10, endPos > 0 ? endPos : cookies.length());
      Serial.printf("[AUTH] Role from cookie: %s\n", role.c_str());
      return role;
    }
  }
  
  Serial.println("[AUTH] No role found, defaulting to guest");
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
  ConnectionGuard guard(true);
  
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
  if (authenticateUser(username, password)) {
    resetLoginFailures(clientIP);
    // Find user role
    String role = "guest";
    for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
      if (defaultUsers[i].username == username) {
        role = defaultUsers[i].role;
        break;
      }
    }
    
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
  ConnectionGuard guard(true);
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
  if (authenticateUser(username, password)) {
    resetLoginFailures(clientIP);
    // Find user role
    String role = "guest";
    for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
      if (defaultUsers[i].username == username) {
        role = defaultUsers[i].role;
        break;
      }
    }
    
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
      int dotPos = token.indexOf('.');
      if (dotPos > 0) {
        int secondDot = token.indexOf('.', dotPos + 1);
        if (secondDot > 0) {
          String payload = token.substring(dotPos + 1, secondDot);
          String decoded = base64Decode(payload);
          DynamicJsonDocument doc(256);
          DeserializationError error = deserializeJson(doc, decoded);
          if (!error) {
            return doc["username"].as<String>();
          }
        }
      }
    }
  }
  
  // Try session cookie
  if (request->hasHeader("Cookie")) {
    String cookies = request->header("Cookie");
    int sessionPos = cookies.indexOf("session=");
    if (sessionPos >= 0) {
      int endPos = cookies.indexOf(';', sessionPos);
      String sessionId = cookies.substring(sessionPos + 8, endPos > 0 ? endPos : cookies.length());
      if (activeSessions.find(sessionId) != activeSessions.end()) {
        return activeSessions[sessionId].username;
      }
    }
  }
  
  return "";
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
