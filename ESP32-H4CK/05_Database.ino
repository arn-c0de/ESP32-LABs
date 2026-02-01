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

bool insertUser(String username, String password, String role) {
  // Read existing users
  String content = readFile(DB_FILE_PATH);
  if (content == "") {
    createUserTable();
    content = readFile(DB_FILE_PATH);
  }
  
  DynamicJsonDocument doc(2048);
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
  
  // Add new user
  JsonObject newUser = users.createNestedObject();
  newUser["username"] = username;
  newUser["password"] = password;  // Intentionally storing plaintext in vulnerable mode
  newUser["role"] = role;
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
  
  DynamicJsonDocument doc(2048);
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
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, content);
  
  if (error) return false;
  
  JsonArray users = doc["users"];
  for (int i = 0; i < users.size(); i++) {
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
  
  DynamicJsonDocument doc(2048);
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
  // Add some test users to the database
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
