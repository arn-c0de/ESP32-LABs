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

  // Initialize shop database (products, carts, orders)
  initShopDatabase();

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

// ========================================
// SHOP DATABASE FUNCTIONS
// ========================================

const char* PRODUCTS_FILE_PATH = "/db/products.json";
const char* CARTS_FILE_PATH = "/db/carts.json";
const char* ORDERS_FILE_PATH = "/db/orders.json";

void initShopDatabase() {
  // Initialize products
  if (!fileExists(PRODUCTS_FILE_PATH)) {
    DynamicJsonDocument doc(4096);
    JsonArray products = doc.createNestedArray("products");
    
    // Default products
    JsonObject p1 = products.createNestedObject();
    p1["id"] = "PROD001";
    p1["name"] = "IoT Gateway Pro";
    p1["description"] = "Enterprise-grade IoT gateway with secure connectivity";
    p1["price"] = 500.0;
    p1["inventory"] = 50;
    p1["category"] = "Hardware";
    p1["created_date"] = millis();
    
    JsonObject p2 = products.createNestedObject();
    p2["id"] = "PROD002";
    p2["name"] = "Management Platform License";
    p2["description"] = "1-year license for centralized device management";
    p2["price"] = 1500.0;
    p2["inventory"] = 100;
    p2["category"] = "Software";
    p2["created_date"] = millis();
    
    JsonObject p3 = products.createNestedObject();
    p3["id"] = "PROD003";
    p3["name"] = "Security Suite Enterprise";
    p3["description"] = "Complete security solution with threat detection and prevention";
    p3["price"] = 2000.0;
    p3["inventory"] = 25;
    p3["category"] = "Software";
    p3["created_date"] = millis();
    
    String output;
    serializeJson(doc, output);
    writeFile(PRODUCTS_FILE_PATH, output);
    Serial.println("[SHOP] Products database initialized");
  }
  
  // Initialize carts
  if (!fileExists(CARTS_FILE_PATH)) {
    writeFile(CARTS_FILE_PATH, "{\"carts\":[]}");
    Serial.println("[SHOP] Carts database initialized");
  }
  
  // Initialize orders
  if (!fileExists(ORDERS_FILE_PATH)) {
    writeFile(ORDERS_FILE_PATH, "{\"orders\":[]}");
    Serial.println("[SHOP] Orders database initialized");
  }
}

// Product functions
String getAllProducts() {
  return readFile(PRODUCTS_FILE_PATH);
}

String getProduct(String productId) {
  String content = readFile(PRODUCTS_FILE_PATH);
  if (content == "") return "{}";
  
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return "{}";
  
  JsonArray products = doc["products"];
  for (JsonObject product : products) {
    if (product["id"].as<String>() == productId) {
      String result;
      serializeJson(product, result);
      return result;
    }
  }
  return "{}";
}

bool addProduct(String id, String name, String description, float price, int inventory, String category) {
  String content = readFile(PRODUCTS_FILE_PATH);
  if (content == "") return false;
  
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;
  
  JsonArray products = doc["products"];
  JsonObject newProduct = products.createNestedObject();
  newProduct["id"] = id;
  newProduct["name"] = name;
  newProduct["description"] = description;
  newProduct["price"] = price;
  newProduct["inventory"] = inventory;
  newProduct["category"] = category;
  newProduct["created_date"] = millis();
  
  String output;
  serializeJson(doc, output);
  writeFile(PRODUCTS_FILE_PATH, output);
  return true;
}

bool updateProduct(String productId, String name, String description, float price, int inventory, String category) {
  String content = readFile(PRODUCTS_FILE_PATH);
  if (content == "") return false;
  
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;
  
  JsonArray products = doc["products"];
  bool found = false;
  
  for (JsonObject product : products) {
    if (product["id"].as<String>() == productId) {
      if (name != "") product["name"] = name;
      if (description != "") product["description"] = description;
      if (price >= 0) product["price"] = price;
      if (inventory >= 0) product["inventory"] = inventory;
      if (category != "") product["category"] = category;
      found = true;
      break;
    }
  }
  
  if (found) {
    String output;
    serializeJson(doc, output);
    writeFile(PRODUCTS_FILE_PATH, output);
  }
  return found;
}

bool deleteProduct(String productId) {
  String content = readFile(PRODUCTS_FILE_PATH);
  if (content == "") return false;
  
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;
  
  JsonArray products = doc["products"];
  int index = -1;
  
  for (int i = 0; i < products.size(); i++) {
    if (products[i]["id"].as<String>() == productId) {
      index = i;
      break;
    }
  }
  
  if (index >= 0) {
    products.remove(index);
    String output;
    serializeJson(doc, output);
    writeFile(PRODUCTS_FILE_PATH, output);
    return true;
  }
  return false;
}

// Cart functions
String getCart(String username) {
  String content = readFile(CARTS_FILE_PATH);
  if (content == "") return "{\"items\":[],\"total_items\":0,\"total_price\":0}";
  
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return "{\"items\":[],\"total_items\":0,\"total_price\":0}";
  
  JsonArray carts = doc["carts"];
  for (JsonObject cart : carts) {
    if (cart["username"].as<String>() == username) {
      String result;
      serializeJson(cart, result);
      return result;
    }
  }
  return "{\"items\":[],\"total_items\":0,\"total_price\":0}";
}

bool addToCart(String username, String productId, int quantity) {
  String content = readFile(CARTS_FILE_PATH);
  if (content == "") return false;
  
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;
  
  JsonArray carts = doc["carts"];
  JsonObject userCart;
  
  // Find existing cart
  for (JsonObject cart : carts) {
    if (cart["username"].as<String>() == username) {
      userCart = cart;
      break;
    }
  }
  
  // Create new cart if not exists
  if (userCart.isNull()) {
    userCart = carts.createNestedObject();
    userCart["username"] = username;
    userCart.createNestedArray("items");
    userCart["created_at"] = millis();
  }
  
  JsonArray items = userCart["items"];
  bool found = false;
  
  // Update quantity if item exists
  for (JsonObject item : items) {
    if (item["product_id"].as<String>() == productId) {
      item["qty"] = item["qty"].as<int>() + quantity;
      found = true;
      break;
    }
  }
  
  // Add new item
  if (!found) {
    JsonObject newItem = items.createNestedObject();
    newItem["product_id"] = productId;
    newItem["qty"] = quantity;
  }
  
  userCart["last_modified"] = millis();
  
  String output;
  serializeJson(doc, output);
  writeFile(CARTS_FILE_PATH, output);
  return true;
}

bool updateCartItem(String username, String productId, int quantity) {
  String content = readFile(CARTS_FILE_PATH);
  if (content == "") return false;
  
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;
  
  JsonArray carts = doc["carts"];
  bool updated = false;
  
  for (JsonObject cart : carts) {
    if (cart["username"].as<String>() == username) {
      JsonArray items = cart["items"];
      for (JsonObject item : items) {
        if (item["product_id"].as<String>() == productId) {
          item["qty"] = quantity;
          cart["last_modified"] = millis();
          updated = true;
          break;
        }
      }
      break;
    }
  }
  
  if (updated) {
    String output;
    serializeJson(doc, output);
    writeFile(CARTS_FILE_PATH, output);
  }
  return updated;
}

bool removeFromCart(String username, String productId) {
  String content = readFile(CARTS_FILE_PATH);
  if (content == "") return false;
  
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;
  
  JsonArray carts = doc["carts"];
  bool removed = false;
  
  for (JsonObject cart : carts) {
    if (cart["username"].as<String>() == username) {
      JsonArray items = cart["items"];
      int index = -1;
      for (int i = 0; i < items.size(); i++) {
        if (items[i]["product_id"].as<String>() == productId) {
          index = i;
          break;
        }
      }
      if (index >= 0) {
        items.remove(index);
        cart["last_modified"] = millis();
        removed = true;
      }
      break;
    }
  }
  
  if (removed) {
    String output;
    serializeJson(doc, output);
    writeFile(CARTS_FILE_PATH, output);
  }
  return removed;
}

bool clearCart(String username) {
  String content = readFile(CARTS_FILE_PATH);
  if (content == "") return false;
  
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;
  
  JsonArray carts = doc["carts"];
  int index = -1;
  
  for (int i = 0; i < carts.size(); i++) {
    if (carts[i]["username"].as<String>() == username) {
      index = i;
      break;
    }
  }
  
  if (index >= 0) {
    carts.remove(index);
    String output;
    serializeJson(doc, output);
    writeFile(CARTS_FILE_PATH, output);
    return true;
  }
  return false;
}

// Order functions
String createOrder(String username, JsonArray items, float totalAmount, String shippingAddress, String shippingCity, String shippingZip) {
  String content = readFile(ORDERS_FILE_PATH);
  if (content == "") return "";
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return "";
  
  JsonArray orders = doc["orders"];
  
  // Generate order ID
  String orderId = generateRandomToken(8);
  
  JsonObject newOrder = orders.createNestedObject();
  newOrder["order_id"] = orderId;
  newOrder["username"] = username;
  newOrder["total_amount"] = totalAmount;
  newOrder["shipping_address"] = shippingAddress;
  newOrder["shipping_city"] = shippingCity;
  newOrder["shipping_zip"] = shippingZip;
  newOrder["status"] = "completed";
  newOrder["timestamp"] = millis();
  
  // Copy items array
  JsonArray orderItems = newOrder.createNestedArray("items");
  for (JsonVariant item : items) {
    orderItems.add(item);
  }
  
  // Limit to 500 orders
  if (orders.size() > 500) {
    orders.remove(0);
  }
  
  String output;
  serializeJson(doc, output);
  writeFile(ORDERS_FILE_PATH, output);
  
  Serial.printf("[SHOP] Order created: %s for user %s, total: %.2f\n", orderId.c_str(), username.c_str(), totalAmount);
  return orderId;
}

String getOrders(String username) {
  String content = readFile(ORDERS_FILE_PATH);
  if (content == "") return "{\"orders\":[]}";
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return "{\"orders\":[]}";
  
  JsonArray allOrders = doc["orders"];
  DynamicJsonDocument result(16384);
  JsonArray userOrders = result.createNestedArray("orders");
  
  for (JsonObject order : allOrders) {
    if (order["username"].as<String>() == username) {
      userOrders.add(order);
    }
  }
  
  String output;
  serializeJson(result, output);
  return output;
}

String getAllOrders() {
  return readFile(ORDERS_FILE_PATH);
}

String getOrder(String orderId) {
  String content = readFile(ORDERS_FILE_PATH);
  if (content == "") return "{}";
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return "{}";
  
  JsonArray orders = doc["orders"];
  for (JsonObject order : orders) {
    if (order["order_id"].as<String>() == orderId) {
      String result;
      serializeJson(order, result);
      return result;
    }
  }
  return "{}";
}

bool updateOrder(String orderId, String shippingAddress, String shippingCity, String shippingZip, String status) {
  String content = readFile(ORDERS_FILE_PATH);
  if (content == "") return false;
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;
  
  JsonArray orders = doc["orders"];
  bool updated = false;
  
  for (JsonObject order : orders) {
    if (order["order_id"].as<String>() == orderId) {
      if (shippingAddress != "") order["shipping_address"] = shippingAddress;
      if (shippingCity != "") order["shipping_city"] = shippingCity;
      if (shippingZip != "") order["shipping_zip"] = shippingZip;
      if (status != "") order["status"] = status;
      updated = true;
      break;
    }
  }
  
  if (updated) {
    String output;
    serializeJson(doc, output);
    writeFile(ORDERS_FILE_PATH, output);
  }
  return updated;
}

bool deleteOrder(String orderId) {
  String content = readFile(ORDERS_FILE_PATH);
  if (content == "") return false;
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return false;
  
  JsonArray orders = doc["orders"];
  int index = -1;
  
  for (int i = 0; i < orders.size(); i++) {
    if (orders[i]["order_id"].as<String>() == orderId) {
      index = i;
      break;
    }
  }
  
  if (index >= 0) {
    orders.remove(index);
    String output;
    serializeJson(doc, output);
    writeFile(ORDERS_FILE_PATH, output);
    return true;
  }
  return false;
}

String getOrderStats() {
  String content = readFile(ORDERS_FILE_PATH);
  if (content == "") return "{\"total_revenue\":0,\"order_count\":0,\"completed_orders\":0,\"pending_orders\":0}";
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, content);
  if (error) return "{\"total_revenue\":0,\"order_count\":0,\"completed_orders\":0,\"pending_orders\":0}";
  
  JsonArray orders = doc["orders"];
  float totalRevenue = 0;
  int completedOrders = 0;
  int pendingOrders = 0;
  
  for (JsonObject order : orders) {
    totalRevenue += order["total_amount"].as<float>();
    String status = order["status"].as<String>();
    if (status == "completed") completedOrders++;
    if (status == "pending") pendingOrders++;
  }
  
  DynamicJsonDocument result(512);
  result["total_revenue"] = totalRevenue;
  result["order_count"] = orders.size();
  result["completed_orders"] = completedOrders;
  result["pending_orders"] = pendingOrders;
  
  String output;
  serializeJson(result, output);
  return output;
}
