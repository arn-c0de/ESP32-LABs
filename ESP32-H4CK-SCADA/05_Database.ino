/*
 * Database Module
 *
 * Simple file-based database using LittleFS and JSON.
 * Provides basic CRUD operations for user management.
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

  // Find default user for email defaults
  String defaultEmail = username + "@scada-lab.local";
  for (int i = 0; i < DEFAULT_USERS_COUNT; i++) {
    if (defaultUsers[i].username == username) {
      defaultEmail = defaultUsers[i].email;
      break;
    }
  }

  // Add new user
  JsonObject newUser = users.createNestedObject();
  newUser["username"] = username;
  newUser["password"] = password;  // Intentionally storing plaintext in vulnerable mode
  newUser["role"] = role;
  newUser["email"] = defaultEmail;
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
