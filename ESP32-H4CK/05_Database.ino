/*
 * Database Module
 *
 * Simple file-based database using LittleFS and JSON.
 * Provides basic CRUD operations for user management.
 * Includes wallet/balance and transaction management.
 */

void initDatabase() {
  // Create database directory if it doesn't exist
  if (!LittleFS.exists("/db")) {
    LittleFS.mkdir("/db");
  }

  // Create logs directory
  if (!LittleFS.exists("/logs")) {
    LittleFS.mkdir("/logs");
  }

  // Initialize user table
  createUserTable();

  // Migrate existing schema to add wallet fields
  migrateUserSchema();

  // Initialize transactions file
  if (!fileExists(TX_FILE_PATH)) {
    writeFile(TX_FILE_PATH, "{\"transactions\":[]}");
    Serial.println("[DATABASE] Transaction log created");
  }

  // Seed test data
  seedTestData();

  Serial.println("[DATABASE] Database initialized");
}

void createUserTable() {
  // Check if users file exists
  if (!fileExists(DB_FILE_PATH)) {
    // Create empty users JSON file
    DynamicJsonDocument doc(1024);
    JsonArray users = doc.createNestedArray("users");

    String output;
    serializeJson(doc, output);
    writeFile(DB_FILE_PATH, output);

    Serial.println("[DATABASE] User table created");
  }
}

void migrateUserSchema() {
  String content = readFile(DB_FILE_PATH);
  if (content == "") return;

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return;

  JsonArray users = doc["users"];
  bool needsWrite = false;

  for (JsonObject user : users) {
    // Add balance if missing
    if (!user.containsKey("balance")) {
      String role = user["role"].as<String>();
      if (role == "admin") {
        user["balance"] = 5000.0;
      } else if (user["username"] == "operator") {
        user["balance"] = 2000.0;
      } else {
        user["balance"] = 1000.0;
      }
      needsWrite = true;
    }
    // Add email if missing
    if (!user.containsKey("email")) {
      String uname = user["username"].as<String>();
      user["email"] = uname + "@securenet-solutions.local";
      needsWrite = true;
    }
    // Add first_name/last_name if missing
    if (!user.containsKey("first_name")) {
      user["first_name"] = "";
      user["last_name"] = "";
      needsWrite = true;
    }
  }

  if (needsWrite) {
    String output;
    serializeJson(doc, output);
    writeFile(DB_FILE_PATH, output);
    Serial.println("[DATABASE] Schema migrated (added balance/email fields)");
  }
}

bool insertUser(String username, String password, String role) {
  // Read existing users
  String content = readFile(DB_FILE_PATH);
  if (content == "") {
    createUserTable();
    content = readFile(DB_FILE_PATH);
  }

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);

  if (error) {
    logError("Failed to parse users file");
    return false;
  }

  // Check if user already exists
  JsonArray users = doc["users"];
  for (JsonObject user : users) {
    if (user["username"] == username) {
      logError("User already exists: " + username);
      return false;
    }
  }

  // Find default user for balance/email defaults
  float defaultBalance = 1000.0;
  String defaultEmail = username + "@securenet-solutions.local";
  String firstName = "";
  String lastName = "";
  for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
    if (defaultUsers[i].username == username) {
      defaultBalance = defaultUsers[i].balance;
      defaultEmail = defaultUsers[i].email;
      firstName = defaultUsers[i].first_name;
      lastName = defaultUsers[i].last_name;
      break;
    }
  }
  if (role == "admin") defaultBalance = 5000.0;

  // Add new user
  JsonObject newUser = users.createNestedObject();
  newUser["username"] = username;
  newUser["password"] = password;  // Intentionally storing plaintext in vulnerable mode
  newUser["role"] = role;
  newUser["balance"] = defaultBalance;
  newUser["email"] = defaultEmail;
  newUser["first_name"] = firstName;
  newUser["last_name"] = lastName;
  newUser["created"] = millis();

  // Write back to file
  String output;
  serializeJson(doc, output);

  if (writeFile(DB_FILE_PATH, output)) {
    logInfo("User created: " + username);
    return true;
  }

  return false;
}

String getUserByUsername(String username) {
  String content = readFile(DB_FILE_PATH);
  if (content == "") return "";

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);

  if (error) return "";

  JsonArray users = doc["users"];
  for (JsonObject user : users) {
    if (user["username"] == username) {
      String userStr;
      serializeJson(user, userStr);
      return userStr;
    }
  }

  return "";
}

String getAllUsers() {
  String content = readFile(DB_FILE_PATH);
  if (content == "") {
    return "{\"users\":[]}";
  }
  return content;
}

bool deleteUser(String username) {
  String content = readFile(DB_FILE_PATH);
  if (content == "") return false;

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);

  if (error) return false;

  JsonArray users = doc["users"];
  for (int i = 0; i < (int)users.size(); i++) {
    if (users[i]["username"] == username) {
      users.remove(i);

      String output;
      serializeJson(doc, output);

      if (writeFile(DB_FILE_PATH, output)) {
        logInfo("User deleted: " + username);
        return true;
      }
      return false;
    }
  }

  return false;
}

bool updateUser(String username, String newPassword, String newRole) {
  String content = readFile(DB_FILE_PATH);
  if (content == "") return false;

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);

  if (error) return false;

  JsonArray users = doc["users"];
  for (JsonObject user : users) {
    if (user["username"] == username) {
      if (newPassword != "") user["password"] = newPassword;
      if (newRole != "") user["role"] = newRole;
      user["modified"] = millis();

      String output;
      serializeJson(doc, output);

      if (writeFile(DB_FILE_PATH, output)) {
        logInfo("User updated: " + username);
        return true;
      }
      return false;
    }
  }

  return false;
}

void seedTestData() {
  // Seed default users into DB if not already present
  for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
    if (getUserByUsername(defaultUsers[i].username) == "") {
      insertUser(defaultUsers[i].username, defaultUsers[i].password, defaultUsers[i].role);
    }
  }

  // Add extra test users
  if (getUserByUsername("testuser") == "") {
    insertUser("testuser", "password123", "guest");
  }
  if (getUserByUsername("developer") == "") {
    insertUser("developer", "dev123", "admin");
  }
  if (getUserByUsername("hacker") == "") {
    insertUser("hacker", "' OR '1'='1", "guest");  // SQL injection test payload as password!
  }

  Serial.println("[DATABASE] Test data seeded");
}

// ===== WALLET DATABASE FUNCTIONS =====

float getUserBalance(String username) {
  // First check DB file
  String content = readFile(DB_FILE_PATH);
  if (content == "") return 0.0;

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return 0.0;

  JsonArray users = doc["users"];
  for (JsonObject user : users) {
    if (user["username"] == username) {
      return user["balance"].as<float>();
    }
  }

  // Check default users as fallback
  for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
    if (defaultUsers[i].username == username) {
      return defaultUsers[i].balance;
    }
  }

  return 0.0;
}

bool updateBalance(String username, float newBalance) {
  String content = readFile(DB_FILE_PATH);
  if (content == "") return false;

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;

  JsonArray users = doc["users"];
  for (JsonObject user : users) {
    if (user["username"] == username) {
      user["balance"] = newBalance;
      user["modified"] = millis();

      String output;
      serializeJson(doc, output);
      return writeFile(DB_FILE_PATH, output);
    }
  }

  return false;
}

bool logTransaction(String fromUser, String toUser, float amount, String type) {
  String content = readFile(TX_FILE_PATH);
  if (content == "") {
    content = "{\"transactions\":[]}";
  }

  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, content);
  if (error) {
    doc.clear();
    doc.createNestedArray("transactions");
  }

  JsonArray txns = doc["transactions"];

  // Trim old transactions if over limit
  while (txns.size() >= MAX_TRANSACTIONS) {
    txns.remove(0);
  }

  JsonObject tx = txns.createNestedObject();
  tx["id"] = generateUUID().substring(0, 8);
  tx["from_user"] = fromUser;
  tx["to_user"] = toUser;
  tx["amount"] = amount;
  tx["type"] = type;
  tx["timestamp"] = millis();
  tx["date"] = getCurrentTimestamp();

  String output;
  serializeJson(doc, output);
  return writeFile(TX_FILE_PATH, output);
}

String getTransactionHistory(String username, int limit) {
  String content = readFile(TX_FILE_PATH);
  if (content == "") return "{\"transactions\":[]}";

  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return "{\"transactions\":[]}";

  JsonArray allTxns = doc["transactions"];

  // Filter transactions for this user
  DynamicJsonDocument result(4096);
  JsonArray filtered = result.createNestedArray("transactions");

  int count = 0;
  // Iterate in reverse for most recent first
  for (int i = (int)allTxns.size() - 1; i >= 0 && count < limit; i--) {
    JsonObject tx = allTxns[i];
    String from = tx["from_user"].as<String>();
    String to = tx["to_user"].as<String>();

    if (from == username || to == username) {
      JsonObject entry = filtered.createNestedObject();
      entry["id"] = tx["id"];
      entry["from_user"] = from;
      entry["to_user"] = to;
      entry["amount"] = tx["amount"];
      entry["type"] = tx["type"];
      entry["timestamp"] = tx["timestamp"];
      entry["date"] = tx["date"];
      count++;
    }
  }

  result["total"] = count;

  String output;
  serializeJson(result, output);
  return output;
}

bool transferFunds(String fromUser, String toUser, float amount) {
  // Intentionally NO locking - race condition vulnerability!
  float fromBalance = getUserBalance(fromUser);

  // Race window: delay between check and update
  delay(100);

  if (fromBalance < amount) {
    return false;
  }

  float toBalance = getUserBalance(toUser);

  // Update balances
  if (!updateBalance(fromUser, fromBalance - amount)) return false;
  if (!updateBalance(toUser, toBalance + amount)) return false;

  // Log the transaction
  logTransaction(fromUser, toUser, amount, "transfer");

  Serial.printf("[WALLET] Transfer: %s -> %s: %.2f credits\n",
                fromUser.c_str(), toUser.c_str(), amount);
  return true;
}

bool executeQuery(String operation, String data) {
  // Simple query execution (intentionally vulnerable to injection)
  if (VULN_SQL_INJECTION) {
    // Intentionally dangerous: direct string concatenation
    logDebug("Executing query: " + operation + " " + data);
    return true;
  }

  // Safer implementation
  return false;
}

String selectQuery(String key) {
  // Intentionally vulnerable SELECT query
  if (VULN_SQL_INJECTION) {
    String query = "SELECT * FROM users WHERE username='" + key + "'";
    logDebug("Query: " + query);

    // Simulate SQL injection vulnerability
    if (key.indexOf("'") >= 0 || key.indexOf("OR") >= 0) {
      logError("VULNERABILITY TRIGGERED: SQL Injection detected!");
      // Return all users (simulating successful injection)
      return getAllUsers();
    }
  }

  return getUserByUsername(key);
}
