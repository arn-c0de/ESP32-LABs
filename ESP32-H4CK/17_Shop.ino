/*
 * Shop & E-Commerce Module
 *
 * Provides product catalog, shopping cart, and order management.
 * Includes intentional vulnerabilities for penetration testing:
 * - IDOR on orders (access any order without auth)
 * - Order manipulation (change shipping address/status)
 * - Race conditions on checkout
 * - Price manipulation
 */

void setupShopRoutes() {
  // Public product endpoints
  server.on("/api/shop/products", HTTP_GET, handleGetProducts);
  server.on("/api/shop/product", HTTP_GET, handleGetProduct);
  
  // Cart endpoints (auth required)
  server.on("/api/shop/cart", HTTP_GET, handleGetCart);
  server.on("/api/shop/cart/add", HTTP_POST, handleAddToCart);
  server.on("/api/shop/cart/update", HTTP_POST, handleUpdateCart);
  server.on("/api/shop/cart/remove", HTTP_POST, handleRemoveFromCart);
  server.on("/api/shop/cart/clear", HTTP_POST, handleClearCart);
  
  // Checkout & Orders (vulnerable to IDOR)
  server.on("/api/shop/checkout", HTTP_POST, handleCheckout);
  server.on("/api/shop/orders", HTTP_GET, handleGetOrders);
  server.on("/api/shop/order", HTTP_GET, handleGetOrder);
  server.on("/api/shop/order/update", HTTP_PUT, handleUpdateOrder);
  server.on("/api/shop/order/delete", HTTP_DELETE, handleDeleteOrder);
  
  // Admin product management
  server.on("/api/admin/shop/product/add", HTTP_POST, handleAdminAddProduct);
  server.on("/api/admin/shop/product/update", HTTP_PUT, handleAdminUpdateProduct);
  server.on("/api/admin/shop/product/delete", HTTP_DELETE, handleAdminDeleteProduct);
  server.on("/api/admin/shop/orders", HTTP_GET, handleAdminGetAllOrders);
  server.on("/api/admin/shop/order-stats", HTTP_GET, handleAdminOrderStats);
  
  Serial.println("[SHOP] Shop routes registered");
}

// ========================================
// PUBLIC PRODUCT ENDPOINTS
// ========================================

void handleGetProducts(AsyncWebServerRequest *request) {
  String products = getAllProducts();
  
  request->send(200, "application/json", products);
}

void handleGetProduct(AsyncWebServerRequest *request) {
  if (!request->hasParam("id")) {
    request->send(400, "application/json", "{\"error\":\"Missing product id\"}");
    return;
  }
  
  String productId = request->getParam("id")->value();
  String product = getProduct(productId);
  
  if (product == "{}") {
    request->send(404, "application/json", "{\"error\":\"Product not found\"}");
    return;
  }
  
  request->send(200, "application/json", product);
}

// ========================================
// CART ENDPOINTS (AUTH REQUIRED)
// ========================================

void handleGetCart(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  String username = getRequestUsername(request);
  
  String cart = getCart(username);
  
  // Calculate detailed cart info
  DynamicJsonDocument cartDoc(8192);
  DeserializationError error = deserializeJson(cartDoc, cart);
  
  if (!error && cartDoc.containsKey("items")) {
    JsonArray items = cartDoc["items"];
    DynamicJsonDocument response(8192);
    JsonArray detailedItems = response.createNestedArray("items");
    
    float totalPrice = 0;
    int totalItems = 0;
    
    for (JsonObject item : items) {
      String productId = item["product_id"].as<String>();
      int qty = item["qty"].as<int>();
      
      // Get product details
      String productJson = getProduct(productId);
      DynamicJsonDocument productDoc(2048);
      deserializeJson(productDoc, productJson);
      
      if (!productDoc.isNull()) {
        JsonObject detailedItem = detailedItems.createNestedObject();
        detailedItem["product_id"] = productId;
        detailedItem["name"] = productDoc["name"];
        detailedItem["price_per_unit"] = productDoc["price"];
        detailedItem["qty"] = qty;
        detailedItem["subtotal"] = productDoc["price"].as<float>() * qty;
        
        totalPrice += productDoc["price"].as<float>() * qty;
        totalItems += qty;
      }
    }
    
    response["total_items"] = totalItems;
    response["total_price"] = totalPrice;
    
    String output;
    serializeJson(response, output);
    request->send(200, "application/json", output);
  } else {
    request->send(200, "application/json", "{\"items\":[],\"total_items\":0,\"total_price\":0}");
  }
}

void handleAddToCart(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  String username = getRequestUsername(request);
  
  if (!request->hasParam("product_id") || !request->hasParam("quantity")) {
    request->send(400, "application/json", "{\"error\":\"Missing product_id or quantity\"}");
    return;
  }
  
  String productId = request->getParam("product_id")->value();
  int quantity = request->getParam("quantity")->value().toInt();
  
  if (quantity <= 0) {
    request->send(400, "application/json", "{\"error\":\"Quantity must be positive\"}");
    return;
  }
  
  // Check if product exists
  String product = getProduct(productId);
  if (product == "{}") {
    request->send(404, "application/json", "{\"error\":\"Product not found\"}");
    return;
  }
  
  if (addToCart(username, productId, quantity)) {
    String cart = getCart(username);
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Item added to cart\",\"cart\":" + cart + "}");
  } else {
    request->send(500, "application/json", "{\"error\":\"Failed to add item to cart\"}");
  }
}

void handleUpdateCart(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  String username = getRequestUsername(request);
  
  if (!request->hasParam("product_id") || !request->hasParam("quantity")) {
    request->send(400, "application/json", "{\"error\":\"Missing product_id or quantity\"}");
    return;
  }
  
  String productId = request->getParam("product_id")->value();
  int quantity = request->getParam("quantity")->value().toInt();
  
  if (quantity < 0) {
    request->send(400, "application/json", "{\"error\":\"Quantity cannot be negative\"}");
    return;
  }
  
  if (quantity == 0) {
    // Remove item if quantity is 0
    removeFromCart(username, productId);
  } else {
    updateCartItem(username, productId, quantity);
  }
  
  String cart = getCart(username);
  request->send(200, "application/json", "{\"success\":true,\"message\":\"Cart updated\",\"cart\":" + cart + "}");
}

void handleRemoveFromCart(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  String username = getRequestUsername(request);
  
  if (!request->hasParam("product_id")) {
    request->send(400, "application/json", "{\"error\":\"Missing product_id\"}");
    return;
  }
  
  String productId = request->getParam("product_id")->value();
  
  if (removeFromCart(username, productId)) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Item removed from cart\"}");
  } else {
    request->send(500, "application/json", "{\"error\":\"Failed to remove item\"}");
  }
}

void handleClearCart(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  String username = getRequestUsername(request);
  
  if (clearCart(username)) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Cart cleared\"}");
  } else {
    request->send(500, "application/json", "{\"error\":\"Failed to clear cart\"}");
  }
}

// ========================================
// CHECKOUT & ORDER ENDPOINTS (VULNERABLE)
// ========================================

void handleCheckout(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  String username = getRequestUsername(request);
  
  if (!request->hasParam("shipping_address")) {
    request->send(400, "application/json", "{\"error\":\"Missing shipping_address\"}");
    return;
  }
  
  String shippingAddress = request->getParam("shipping_address")->value();
  String shippingCity = request->hasParam("shipping_city") ? request->getParam("shipping_city")->value() : "";
  String shippingZip = request->hasParam("shipping_zip") ? request->getParam("shipping_zip")->value() : "";
  
  // Get user's cart
  String cartJson = getCart(username);
  DynamicJsonDocument cartDoc(8192);
  DeserializationError error = deserializeJson(cartDoc, cartJson);
  
  if (error || !cartDoc.containsKey("items")) {
    request->send(400, "application/json", "{\"error\":\"Cart is empty or invalid\"}");
    return;
  }
  
  JsonArray cartItems = cartDoc["items"];
  if (cartItems.size() == 0) {
    request->send(400, "application/json", "{\"error\":\"Cart is empty\"}");
    return;
  }
  
  // Calculate total and build order items
  DynamicJsonDocument orderDoc(8192);
  JsonArray orderItems = orderDoc.createNestedArray("items");
  float totalAmount = 0;
  
  for (JsonObject item : cartItems) {
    String productId = item["product_id"].as<String>();
    int qty = item["qty"].as<int>();
    
    // Get product price
    String productJson = getProduct(productId);
    DynamicJsonDocument productDoc(2048);
    deserializeJson(productDoc, productJson);
    
    if (!productDoc.isNull()) {
      float price = productDoc["price"].as<float>();
      String name = productDoc["name"].as<String>();
      
      JsonObject orderItem = orderItems.createNestedObject();
      orderItem["product_id"] = productId;
      orderItem["name"] = name;
      orderItem["qty"] = qty;
      orderItem["price_at_order"] = price;
      orderItem["subtotal"] = price * qty;
      
      totalAmount += price * qty;
    }
  }
  
  // VULNERABILITY: Race condition - delay between balance check and deduction
  float currentBalance = getUserBalance(username);
  delay(100);  // Race window!
  
  if (currentBalance < totalAmount) {
    request->send(400, "application/json", "{\"error\":\"Insufficient balance\",\"required\":" + String(totalAmount) + ",\"available\":" + String(currentBalance) + "}");
    return;
  }
  
  // Deduct balance
  if (!updateBalance(username, currentBalance - totalAmount)) {
    request->send(500, "application/json", "{\"error\":\"Failed to deduct balance\"}");
    return;
  }
  
  // Create order
  String orderId = createOrder(username, orderItems, totalAmount, shippingAddress, shippingCity, shippingZip);
  
  if (orderId == "") {
    // Refund on failure
    updateBalance(username, currentBalance);
    request->send(500, "application/json", "{\"error\":\"Failed to create order\"}");
    return;
  }
  
  // Log transaction
  logTransaction(username, "SHOP", totalAmount, "purchase");
  
  // Clear cart
  clearCart(username);
  
  DynamicJsonDocument response(512);
  response["success"] = true;
  response["message"] = "Order placed successfully";
  response["order_id"] = orderId;
  response["total_amount"] = totalAmount;
  response["new_balance"] = currentBalance - totalAmount;
  
  String output;
  serializeJson(response, output);
  request->send(200, "application/json", output);
  
  Serial.printf("[SHOP] Order %s placed by %s, total: %.2f credits\n", orderId.c_str(), username.c_str(), totalAmount);
}

void handleGetOrders(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  String username = getRequestUsername(request);
  
  // VULNERABILITY: IDOR - accept user_id or username parameter
  String targetUser = username;
  if (request->hasParam("user_id") || request->hasParam("username")) {
    if (VULN_IDOR) {
      targetUser = request->hasParam("username") ? 
                   request->getParam("username")->value() : 
                   request->getParam("user_id")->value();
      Serial.println("[VULN] IDOR triggered: accessing orders for user " + targetUser);
    }
  }
  
  String orders = getOrders(targetUser);
  request->send(200, "application/json", orders);
}

void handleGetOrder(AsyncWebServerRequest *request) {
  // VULNERABILITY: IDOR - no authentication check when vulnerabilities enabled
  if (!VULN_IDOR && !isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  if (!request->hasParam("order_id")) {
    request->send(400, "application/json", "{\"error\":\"Missing order_id\"}");
    return;
  }
  
  String orderId = request->getParam("order_id")->value();
  String order = getOrder(orderId);
  
  if (order == "{}") {
    request->send(404, "application/json", "{\"error\":\"Order not found\"}");
    return;
  }
  
  // VULNERABILITY: No ownership check
  if (VULN_IDOR) {
    Serial.println("[VULN] IDOR triggered: accessing order " + orderId + " without ownership check");
  }
  
  request->send(200, "application/json", order);
}

void handleUpdateOrder(AsyncWebServerRequest *request) {
  // VULNERABILITY: Weak/no authentication in vuln mode
  if (!VULN_WEAK_AUTH && !isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  if (!request->hasParam("order_id")) {
    request->send(400, "application/json", "{\"error\":\"Missing order_id\"}");
    return;
  }
  
  String orderId = request->getParam("order_id")->value();
  
  String shippingAddress = request->hasParam("shipping_address") ? request->getParam("shipping_address")->value() : "";
  String shippingCity = request->hasParam("shipping_city") ? request->getParam("shipping_city")->value() : "";
  String shippingZip = request->hasParam("shipping_zip") ? request->getParam("shipping_zip")->value() : "";
  String status = request->hasParam("status") ? request->getParam("status")->value() : "";
  
  // VULNERABILITY: No ownership validation - anyone can update any order
  if (VULN_IDOR) {
    Serial.println("[VULN] IDOR triggered: updating order " + orderId + " without ownership check");
  }
  
  if (updateOrder(orderId, shippingAddress, shippingCity, shippingZip, status)) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Order updated\"}");
  } else {
    request->send(500, "application/json", "{\"error\":\"Failed to update order\"}");
  }
}

void handleDeleteOrder(AsyncWebServerRequest *request) {
  // VULNERABILITY: Weak/no authentication
  if (!VULN_WEAK_AUTH && !isAuthenticated(request)) {
    request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
    return;
  }
  
  if (!request->hasParam("order_id")) {
    request->send(400, "application/json", "{\"error\":\"Missing order_id\"}");
    return;
  }
  
  String orderId = request->getParam("order_id")->value();
  
  // VULNERABILITY: No ownership check, no refund
  if (VULN_IDOR) {
    Serial.println("[VULN] IDOR triggered: deleting order " + orderId + " without ownership check");
  }
  
  if (deleteOrder(orderId)) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Order deleted (no refund)\"}");
  } else {
    request->send(500, "application/json", "{\"error\":\"Failed to delete order\"}");
  }
}

// ========================================
// ADMIN PRODUCT MANAGEMENT
// ========================================

void handleAdminAddProduct(AsyncWebServerRequest *request) {
  // Check admin auth (weak in vuln mode)
  if (!VULN_WEAK_AUTH) {
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
      return;
    }
    String role = getRequestRole(request);
    if (role != "admin") {
      request->send(403, "application/json", "{\"error\":\"Admin access required\"}");
      return;
    }
  }
  
  if (!request->hasParam("name") || !request->hasParam("price")) {
    request->send(400, "application/json", "{\"error\":\"Missing required fields\"}");
    return;
  }
  
  String name = request->getParam("name")->value();
  String description = request->hasParam("description") ? request->getParam("description")->value() : "";
  float price = request->getParam("price")->value().toFloat();
  int inventory = request->hasParam("inventory") ? request->getParam("inventory")->value().toInt() : 100;
  String category = request->hasParam("category") ? request->getParam("category")->value() : "General";
  
  // Generate product ID
  String productId = "PROD" + String(millis() % 10000);
  
  if (addProduct(productId, name, description, price, inventory, category)) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Product added\",\"product_id\":\"" + productId + "\"}");
  } else {
    request->send(500, "application/json", "{\"error\":\"Failed to add product\"}");
  }
}

void handleAdminUpdateProduct(AsyncWebServerRequest *request) {
  // Weak auth check in vuln mode
  if (!VULN_WEAK_AUTH) {
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
      return;
    }
    String role = getRequestRole(request);
    if (role != "admin") {
      request->send(403, "application/json", "{\"error\":\"Admin access required\"}");
      return;
    }
  }
  
  if (!request->hasParam("product_id")) {
    request->send(400, "application/json", "{\"error\":\"Missing product_id\"}");
    return;
  }
  
  String productId = request->getParam("product_id")->value();
  String name = request->hasParam("name") ? request->getParam("name")->value() : "";
  String description = request->hasParam("description") ? request->getParam("description")->value() : "";
  float price = request->hasParam("price") ? request->getParam("price")->value().toFloat() : -1;
  int inventory = request->hasParam("inventory") ? request->getParam("inventory")->value().toInt() : -1;
  String category = request->hasParam("category") ? request->getParam("category")->value() : "";
  
  if (updateProduct(productId, name, description, price, inventory, category)) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Product updated\"}");
  } else {
    request->send(404, "application/json", "{\"error\":\"Product not found\"}");
  }
}

void handleAdminDeleteProduct(AsyncWebServerRequest *request) {
  // Weak auth in vuln mode
  if (!VULN_WEAK_AUTH) {
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
      return;
    }
    String role = getRequestRole(request);
    if (role != "admin") {
      request->send(403, "application/json", "{\"error\":\"Admin access required\"}");
      return;
    }
  }
  
  if (!request->hasParam("product_id")) {
    request->send(400, "application/json", "{\"error\":\"Missing product_id\"}");
    return;
  }
  
  String productId = request->getParam("product_id")->value();
  
  if (deleteProduct(productId)) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Product deleted\"}");
  } else {
    request->send(404, "application/json", "{\"error\":\"Product not found\"}");
  }
}

void handleAdminGetAllOrders(AsyncWebServerRequest *request) {
  // Weak auth - info disclosure in vuln mode
  if (!VULN_WEAK_AUTH) {
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
      return;
    }
    String role = getRequestRole(request);
    if (role != "admin") {
      request->send(403, "application/json", "{\"error\":\"Admin access required\"}");
      return;
    }
  } else {
    Serial.println("[VULN] Weak auth: admin orders accessed without proper auth");
  }
  
  String orders = getAllOrders();
  request->send(200, "application/json", orders);
}

void handleAdminOrderStats(AsyncWebServerRequest *request) {
  // Info disclosure in vuln mode
  if (!VULN_WEAK_AUTH) {
    if (!isAuthenticated(request)) {
      request->send(401, "application/json", "{\"error\":\"Authentication required\"}");
      return;
    }
    String role = getRequestRole(request);
    if (role != "admin") {
      request->send(403, "application/json", "{\"error\":\"Admin access required\"}");
      return;
    }
  } else {
    Serial.println("[VULN] Info disclosure: order stats accessed without auth");
  }
  
  String stats = getOrderStats();
  request->send(200, "application/json", stats);
}
