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
    if (defaultUsers[i].username == username && 
        defaultUsers[i].password == password) {
      
      logInfo("Successful login: " + username);
      failedLoginAttempts = 0;
      return true;
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
      if (VULN_WEAK_AUTH && password == storedPassword) {
        logInfo("Successful login from DB: " + username);
        failedLoginAttempts = 0;
        return true;
      }
      
      // Secure path: Use password hashing (when vulnerability mode is off)
      if (!VULN_WEAK_AUTH && verifyPassword(password, storedPassword)) {
        logInfo("Successful login (hashed): " + username);
        failedLoginAttempts = 0;
        return true;
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
  String decoded = base64Decode(token);
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, decoded);
  
  if (!error) {
    return doc["role"].as<String>();
  }
  
  return "guest";
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

void handleLogin(AsyncWebServerRequest *request) {
  totalRequests++;
  
  String clientIP = request->client()->remoteIP().toString();
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
      String redirectUrl = (role == "admin") ? "/admin" : "/";
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
