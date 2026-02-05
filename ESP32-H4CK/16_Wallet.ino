/*
 * Wallet API Endpoints Module
 *
 * Banking-style wallet system with intentional vulnerabilities:
 * - IDOR: Accept user_id param without ownership check
 * - Race Condition: No locking on balance operations
 * - Session Fixation: Predictable session IDs
 * - Negative deposit: Weak validation allows negative amounts
 */

// Helper: Extract username from request (cookie or JWT)
String getRequestUsername(AsyncWebServerRequest *request) {
  // Check session cookie first
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

  // Check Authorization header (JWT)
  if (request->hasHeader("Authorization")) {
    String authHeader = request->header("Authorization");
    if (authHeader.startsWith("Bearer ")) {
      String token = authHeader.substring(7);

      // Handle weak JWT (alg:none)
      if (VULN_WEAK_AUTH && token.indexOf(".unsigned") > 0) {
        int dotPos = token.indexOf('.');
        if (dotPos > 0) {
          String payloadB64 = token.substring(dotPos + 1, token.indexOf(".", dotPos + 1));
          String decoded = base64Decode(payloadB64);
          DynamicJsonDocument doc(256);
          if (!deserializeJson(doc, decoded)) {
            return doc["username"].as<String>();
          }
        }
      }

      // Standard JWT
      int dotPos = token.indexOf('.');
      if (dotPos > 0) {
        String payload = token.substring(0, dotPos);
        String decoded = base64Decode(payload);
        DynamicJsonDocument doc(256);
        if (!deserializeJson(doc, decoded)) {
          return doc["username"].as<String>();
        }
      }
    }
  }

  // Fallback: check auth_user cookie
  if (request->hasHeader("Cookie")) {
    String cookies = request->header("Cookie");
    int pos = cookies.indexOf("auth_user=");
    if (pos >= 0) {
      int endPos = cookies.indexOf(';', pos);
      return cookies.substring(pos + 10, endPos > 0 ? endPos : cookies.length());
    }
  }

  return "";
}

// Use centralized getRequestRole() implementation in 04_Auth.ino to avoid duplicate symbol
// String getRequestRole(AsyncWebServerRequest *request);


void setupWalletEndpoints() {

  // ===== GET /api/wallet/balance =====
  // IDOR Vulnerability: Accepts user_id param to view ANY user's balance
  server.on("/api/wallet/balance", HTTP_GET, [](AsyncWebServerRequest *request) {
    totalRequests++;
    String clientIP = request->client()->remoteIP().toString();

    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }

    String targetUser = getRequestUsername(request);

    // IDOR: Accept user_id parameter without ownership check
    if (request->hasParam("user_id")) {
      String userId = request->getParam("user_id")->value();
      Serial.printf("[WALLET] IDOR: Balance query for user_id=%s from %s\n",
                    userId.c_str(), clientIP.c_str());

      // Map user_id to username (sequential IDs)
      String content = readFile(DB_FILE_PATH);
      DynamicJsonDocument doc(4096);
      if (!deserializeJson(doc, content)) {
        JsonArray users = doc["users"];
        int id = userId.toInt();
        if (id >= 1 && id <= (int)users.size()) {
          targetUser = users[id - 1]["username"].as<String>();
        }
      }
    }

    float balance = getUserBalance(targetUser);
    String userData = getUserByUsername(targetUser);

    DynamicJsonDocument resp(512);
    resp["username"] = targetUser;
    resp["balance"] = balance;
    resp["currency"] = "credits";

    if (userData != "") {
      DynamicJsonDocument userDoc(512);
      if (!deserializeJson(userDoc, userData)) {
        resp["email"] = userDoc["email"];
        resp["role"] = userDoc["role"];
      }
    }

    String output;
    serializeJson(resp, output);

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // ===== GET /api/wallet/transactions =====
  // IDOR Vulnerability: Accepts user_id to view ANY user's transactions
  server.on("/api/wallet/transactions", HTTP_GET, [](AsyncWebServerRequest *request) {
    totalRequests++;

    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }

    String targetUser = getRequestUsername(request);

    // IDOR: Accept user_id without ownership validation
    if (request->hasParam("user_id")) {
      String userId = request->getParam("user_id")->value();
      Serial.printf("[WALLET] IDOR: Transaction history for user_id=%s\n", userId.c_str());

      String content = readFile(DB_FILE_PATH);
      DynamicJsonDocument doc(4096);
      if (!deserializeJson(doc, content)) {
        JsonArray users = doc["users"];
        int id = userId.toInt();
        if (id >= 1 && id <= (int)users.size()) {
          targetUser = users[id - 1]["username"].as<String>();
        }
      }
    }

    int limit = 50;
    if (request->hasParam("limit")) {
      limit = request->getParam("limit")->value().toInt();
      if (limit <= 0 || limit > 200) limit = 50;
    }

    String txns = getTransactionHistory(targetUser, limit);

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", txns);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // ===== POST /api/wallet/transfer =====
  // IDOR + Race Condition: Accepts from_user param, no locking
  server.on("/api/wallet/transfer", HTTP_POST, [](AsyncWebServerRequest *request) {
    totalRequests++;
    String clientIP = request->client()->remoteIP().toString();

    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }

    String loggedInUser = getRequestUsername(request);

    // IDOR: Accept from_user param instead of enforcing logged-in user
    String fromUser = loggedInUser;
    if (request->hasParam("from_user", true)) {
      fromUser = request->getParam("from_user", true)->value();
      if (fromUser != loggedInUser) {
        Serial.printf("[WALLET] IDOR: %s impersonating %s in transfer!\n",
                      loggedInUser.c_str(), fromUser.c_str());
      }
    }

    String toUser = "";
    float amount = 0;

    if (request->hasParam("to_user", true)) {
      toUser = request->getParam("to_user", true)->value();
    }
    if (request->hasParam("amount", true)) {
      amount = request->getParam("amount", true)->value().toFloat();
    }

    if (toUser == "" || amount <= 0) {
      request->send(400, "application/json", "{\"error\":\"Missing to_user or invalid amount\"}");
      return;
    }

    if (fromUser == toUser) {
      request->send(400, "application/json", "{\"error\":\"Cannot transfer to yourself\"}");
      return;
    }

    // Verify recipient exists
    if (getUserByUsername(toUser) == "") {
      request->send(404, "application/json", "{\"error\":\"Recipient not found\"}");
      return;
    }

    Serial.printf("[WALLET] Transfer: %s -> %s: %.2f credits (requested by %s)\n",
                  fromUser.c_str(), toUser.c_str(), amount, loggedInUser.c_str());

    // Race condition: transferFunds has intentional delay
    if (transferFunds(fromUser, toUser, amount)) {
      DynamicJsonDocument resp(256);
      resp["success"] = true;
      resp["message"] = "Transfer completed";
      resp["from"] = fromUser;
      resp["to"] = toUser;
      resp["amount"] = amount;
      resp["new_balance"] = getUserBalance(fromUser);

      String output;
      serializeJson(resp, output);

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    } else {
      request->send(400, "application/json", "{\"error\":\"Transfer failed - insufficient balance\"}");
    }
  });

  // ===== POST /api/wallet/deposit =====
  // Vulnerability: Weak validation allows negative deposits (withdraw via deposit)
  server.on("/api/wallet/deposit", HTTP_POST, [](AsyncWebServerRequest *request) {
    totalRequests++;

    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }

    String username = getRequestUsername(request);

    if (!request->hasParam("amount", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing amount\"}");
      return;
    }

    float amount = request->getParam("amount", true)->value().toFloat();

    // Intentional vulnerability: No check for negative amounts!
    // Negative deposit = withdrawal without withdrawal authorization
    Serial.printf("[WALLET] Deposit: %s += %.2f credits\n", username.c_str(), amount);

    float currentBalance = getUserBalance(username);
    float newBalance = currentBalance + amount;

    if (updateBalance(username, newBalance)) {
      logTransaction("system", username, amount, "deposit");

      DynamicJsonDocument resp(256);
      resp["success"] = true;
      resp["message"] = "Deposit completed";
      resp["amount"] = amount;
      resp["new_balance"] = newBalance;

      String output;
      serializeJson(resp, output);

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    } else {
      request->send(500, "application/json", "{\"error\":\"Deposit failed\"}");
    }
  });

  // ===== POST /api/wallet/withdraw =====
  // Race condition: No locking, delay between check and update
  server.on("/api/wallet/withdraw", HTTP_POST, [](AsyncWebServerRequest *request) {
    totalRequests++;

    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }

    String username = getRequestUsername(request);

    if (!request->hasParam("amount", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing amount\"}");
      return;
    }

    float amount = request->getParam("amount", true)->value().toFloat();
    if (amount <= 0) {
      request->send(400, "application/json", "{\"error\":\"Invalid amount\"}");
      return;
    }

    float balance = getUserBalance(username);
    Serial.printf("[WALLET] Withdraw: %s (balance: %.2f, amount: %.2f)\n",
                  username.c_str(), balance, amount);

    // Race condition: delay between balance check and update
    delay(100);

    if (balance >= amount) {
      float newBalance = balance - amount;
      if (updateBalance(username, newBalance)) {
        logTransaction(username, "system", amount, "withdraw");

        DynamicJsonDocument resp(256);
        resp["success"] = true;
        resp["withdrawn"] = amount;
        resp["new_balance"] = newBalance;
        resp["hint"] = "Send concurrent requests to exploit race condition";

        String output;
        serializeJson(resp, output);

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
      } else {
        request->send(500, "application/json", "{\"error\":\"Withdraw failed\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Insufficient balance\"}");
    }
  });

  // ===== GET /api/wallet/export =====
  // IDOR: Accepts user_id to export ANY user's transactions as CSV
  server.on("/api/wallet/export", HTTP_GET, [](AsyncWebServerRequest *request) {
    totalRequests++;

    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }

    String targetUser = getRequestUsername(request);

    // IDOR: export any user's data
    if (request->hasParam("user_id")) {
      String userId = request->getParam("user_id")->value();
      Serial.printf("[WALLET] IDOR: CSV export for user_id=%s\n", userId.c_str());

      String content = readFile(DB_FILE_PATH);
      DynamicJsonDocument doc(4096);
      if (!deserializeJson(doc, content)) {
        JsonArray users = doc["users"];
        int id = userId.toInt();
        if (id >= 1 && id <= (int)users.size()) {
          targetUser = users[id - 1]["username"].as<String>();
        }
      }
    }

    String txns = getTransactionHistory(targetUser, 200);
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, txns);

    String csv = "id,date,type,from,to,amount\n";
    JsonArray arr = doc["transactions"];
    for (JsonObject tx : arr) {
      csv += tx["id"].as<String>() + ",";
      csv += tx["date"].as<String>() + ",";
      csv += tx["type"].as<String>() + ",";
      csv += tx["from_user"].as<String>() + ",";
      csv += tx["to_user"].as<String>() + ",";
      csv += String(tx["amount"].as<float>(), 2) + "\n";
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csv);
    response->addHeader("Content-Disposition", "attachment; filename=\"transactions_" + targetUser + ".csv\"");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // ===== GET /api/profile =====
  // Returns current user profile info
  server.on("/api/profile", HTTP_GET, [](AsyncWebServerRequest *request) {
    totalRequests++;

    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }

    String username = getRequestUsername(request);

    // IDOR: also accept username param
    if (request->hasParam("username")) {
      username = request->getParam("username")->value();
    }

    String userData = getUserByUsername(username);
    if (userData == "") {
      // Check default users
      for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
        if (defaultUsers[i].username == username) {
          DynamicJsonDocument resp(512);
          resp["username"] = defaultUsers[i].username;
          resp["role"] = defaultUsers[i].role;
          resp["balance"] = defaultUsers[i].balance;
          resp["email"] = defaultUsers[i].email;
          resp["first_name"] = defaultUsers[i].first_name;
          resp["last_name"] = defaultUsers[i].last_name;

          String output;
          serializeJson(resp, output);
          request->send(200, "application/json", output);
          return;
        }
      }
      request->send(404, "application/json", "{\"error\":\"User not found\"}");
      return;
    }

    // Return user data (including password in vuln mode!)
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", userData);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // ===== ADMIN: POST /api/admin/wallet/reset =====
  // Resets all user balances to defaults
  server.on("/api/admin/wallet/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    totalRequests++;

    // Require admin access
    if (!requireAdmin(request)) {
      return;
    }

    String content = readFile(DB_FILE_PATH);
    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, content)) {
      request->send(500, "application/json", "{\"error\":\"Database error\"}");
      return;
    }

    JsonArray users = doc["users"];
    for (JsonObject user : users) {
      String role = user["role"].as<String>();
      if (role == "admin") {
        user["balance"] = 5000.0;
      } else if (user["username"] == "operator") {
        user["balance"] = 2000.0;
      } else {
        user["balance"] = 1000.0;
      }
    }

    String output;
    serializeJson(doc, output);
    writeFile(DB_FILE_PATH, output);

    // Clear transactions
    writeFile(TX_FILE_PATH, "{\"transactions\":[]}");

    Serial.println("[WALLET] All balances reset to defaults");
    request->send(200, "application/json", "{\"success\":true,\"message\":\"All balances reset to defaults\"}");
  });

  // ===== ADMIN: GET /api/admin/wallet/stats =====
  // Information disclosure: shows total balances and top accounts
  server.on("/api/admin/wallet/stats", HTTP_GET, [](AsyncWebServerRequest *request) {
    totalRequests++;

    // Require admin access
    if (!requireAdmin(request)) {
      return;
    }

    String content = readFile(DB_FILE_PATH);
    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, content)) {
      request->send(500, "application/json", "{\"error\":\"Database error\"}");
      return;
    }

    float totalBalance = 0;
    int userCount = 0;
    float highestBalance = 0;
    String richestUser = "";

    JsonArray users = doc["users"];
    DynamicJsonDocument resp(2048);
    JsonArray accounts = resp.createNestedArray("accounts");

    for (JsonObject user : users) {
      float bal = user["balance"].as<float>();
      totalBalance += bal;
      userCount++;

      if (bal > highestBalance) {
        highestBalance = bal;
        richestUser = user["username"].as<String>();
      }

      JsonObject acct = accounts.createNestedObject();
      acct["username"] = user["username"];
      acct["balance"] = bal;
      acct["role"] = user["role"];
    }

    resp["total_balance"] = totalBalance;
    resp["user_count"] = userCount;
    resp["average_balance"] = userCount > 0 ? totalBalance / userCount : 0;
    resp["highest_balance"] = highestBalance;
    resp["richest_user"] = richestUser;

    String output;
    serializeJson(resp, output);

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // ===== ADMIN: POST /api/admin/wallet/adjust =====
  // Manually adjust a user's balance
  server.on("/api/admin/wallet/adjust", HTTP_POST, [](AsyncWebServerRequest *request) {
    totalRequests++;

    // Require admin access
    if (!requireAdmin(request)) {
      return;
    }

    String targetUser = "";
    float newBalance = 0;

    if (request->hasParam("username", true)) {
      targetUser = request->getParam("username", true)->value();
    }
    if (request->hasParam("balance", true)) {
      newBalance = request->getParam("balance", true)->value().toFloat();
    }

    if (targetUser == "") {
      request->send(400, "application/json", "{\"error\":\"Missing username\"}");
      return;
    }

    if (getUserByUsername(targetUser) == "") {
      request->send(404, "application/json", "{\"error\":\"User not found\"}");
      return;
    }

    float oldBalance = getUserBalance(targetUser);
    if (updateBalance(targetUser, newBalance)) {
      logTransaction("admin_adjust", targetUser, newBalance - oldBalance, "adjustment");

      DynamicJsonDocument resp(256);
      resp["success"] = true;
      resp["username"] = targetUser;
      resp["old_balance"] = oldBalance;
      resp["new_balance"] = newBalance;

      String output;
      serializeJson(resp, output);
      request->send(200, "application/json", output);
    } else {
      request->send(500, "application/json", "{\"error\":\"Failed to update balance\"}");
    }
  });

  Serial.println("[WALLET] Wallet endpoints configured");
}
