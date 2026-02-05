# Wallet & Shop System Implementation Summary

## Overview
This document summarizes the complete implementation of the **Wallet Banking System** and **E-Commerce Shop System** for the ESP32-H4CK penetration testing platform. Both systems are designed with intentional vulnerabilities for security training and ethical hacking practice.

---

## 🏦 Wallet System (16_Wallet.ino)

### Features Implemented
- **Per-User Credit Balances**: Each user has their own wallet with credit balance tracked in the database
- **Transaction History**: Complete audit trail of all deposits, withdrawals, and transfers
- **Peer-to-Peer Transfers**: Users can send credits to other users
- **Admin Controls**: Administrative endpoints for balance management and transaction oversight
- **Dashboard UI**: Modern web interface showing balance, recent transactions, and quick actions

### API Endpoints

#### User Endpoints
| Endpoint | Method | Description | Vulnerability |
|----------|--------|-------------|---------------|
| `/api/wallet/balance` | GET | Get user balance with optional IDOR via `user_id` param | ✅ IDOR |
| `/api/wallet/deposit` | POST | Deposit credits (params: `amount`) | ⚠️ Negative amounts |
| `/api/wallet/withdraw` | POST | Withdraw credits (params: `amount`) | ⚠️ Race condition |
| `/api/wallet/transfer` | POST | Transfer credits (params: `to_username`, `amount`) | ⚠️ Race condition |
| `/api/wallet/transactions` | GET | Get transaction history with IDOR via `user_id` | ✅ IDOR |

#### Admin Endpoints
| Endpoint | Method | Description | Vulnerability |
|----------|--------|-------------|---------------|
| `/api/wallet/admin/set-balance` | POST | Set any user's balance (params: `username`, `amount`) | ⚠️ Weak auth check |
| `/api/wallet/admin/all-balances` | GET | Get all users' balances | ⚠️ Info disclosure |
| `/api/wallet/admin/all-transactions` | GET | Get all transactions system-wide | ⚠️ Info disclosure |

### Database Schema
**File**: `/db/transactions.json`
```json
{
  "transactions": [
    {
      "id": "TX123456",
      "username": "user1",
      "type": "transfer",
      "amount": 50.00,
      "from": "user1",
      "to": "user2",
      "timestamp": 1234567890,
      "balance_after": 450.00
    }
  ]
}
```

### Frontend Pages
- [dashboard.html](data/dashboard.html) - Main wallet dashboard with balance overview and quick actions
- [transactions.html](data/transactions.html) - Detailed transaction history with filtering
- [transfer.html](data/transfer.html) - P2P credit transfer interface
- [profile.html](data/profile.html) - User profile management

### Intentional Vulnerabilities
1. **IDOR (Insecure Direct Object Reference)**: `/api/wallet/balance?user_id=X` allows viewing any user's balance
2. **IDOR on Transactions**: `/api/wallet/transactions?user_id=X` exposes any user's transaction history
3. **Race Condition**: Multiple simultaneous transfers can bypass balance checks (no transaction locking)
4. **Negative Deposits**: Weak validation allows `amount=-100` to subtract credits instead of adding
5. **Session Fixation**: Predictable session IDs can be hijacked
6. **Weak Authentication**: Admin endpoints have bypassable auth when `VULN_WEAK_AUTH` is enabled

### Testing Examples
```bash
# IDOR - View admin's balance
curl "http://esp32.local/api/wallet/balance?user_id=admin" -H "Cookie: session=user123"

# Race Condition - Overdraft attack (send 2 transfers simultaneously)
curl -X POST "http://esp32.local/api/wallet/transfer" -d "to_username=attacker&amount=500" &
curl -X POST "http://esp32.local/api/wallet/transfer" -d "to_username=attacker&amount=500" &

# Negative Deposit - Steal credits from wallet
curl -X POST "http://esp32.local/api/wallet/deposit" -d "amount=-1000"
```

---

## 🛒 E-Commerce Shop System (17_Shop.ino)

### Features Implemented
- **Product Catalog**: Browsable product inventory with categories, prices, and stock levels
- **Shopping Cart**: Multi-item cart with quantity management
- **Checkout Process**: Complete order placement with shipping address collection
- **Order History**: Per-user order tracking with detailed order information
- **Admin Product Management**: CRUD operations for products and order oversight
- **Credit Integration**: Purchases deduct from wallet balance

### API Endpoints

#### Public Endpoints
| Endpoint | Method | Description | Vulnerability |
|----------|--------|-------------|---------------|
| `/api/shop/products` | GET | Get all products with inventory | None |
| `/api/shop/product` | GET | Get single product by `id` | None |

#### Authenticated Endpoints (Cart)
| Endpoint | Method | Description | Vulnerability |
|----------|--------|-------------|---------------|
| `/api/shop/cart` | GET | Get current user's cart | None |
| `/api/shop/cart/add` | POST | Add product to cart (params: `product_id`, `quantity`) | None |
| `/api/shop/cart/update` | POST | Update cart item quantity (params: `product_id`, `quantity`) | None |
| `/api/shop/cart/remove` | POST | Remove item from cart (params: `product_id`) | None |
| `/api/shop/cart/clear` | POST | Clear entire cart | None |

#### Order Endpoints
| Endpoint | Method | Description | Vulnerability |
|----------|--------|-------------|---------------|
| `/api/shop/checkout` | POST | Place order (params: `shipping_address`, `shipping_city`, `shipping_zip`) | ⚠️ Race condition |
| `/api/shop/orders` | GET | Get user orders with IDOR via `username` param | ✅ IDOR |
| `/api/shop/order` | GET | Get order details with no auth (params: `order_id`) | ✅ IDOR |
| `/api/shop/order/update` | POST | Update order shipping/status (params: `order_id`, `shipping_address`, `status`) | ✅ IDOR |
| `/api/shop/order/delete` | POST | Delete order without refund (params: `order_id`) | ✅ IDOR |

#### Admin Endpoints
| Endpoint | Method | Description | Vulnerability |
|----------|--------|-------------|---------------|
| `/api/shop/admin/product/add` | POST | Add new product (params: `name`, `price`, `description`, `inventory`, `category`) | ⚠️ Weak auth |
| `/api/shop/admin/product/update` | POST | Update product (params: `product_id`, `name`, `price`, `inventory`) | ⚠️ Weak auth |
| `/api/shop/admin/product/delete` | POST | Delete product (params: `product_id`) | ⚠️ Weak auth |
| `/api/shop/admin/orders` | GET | Get all orders system-wide | ⚠️ Info disclosure |
| `/api/shop/admin/order-stats` | GET | Get order statistics | ⚠️ Info disclosure |

### Database Schema

#### Products
**File**: `/db/products.json`
```json
{
  "products": [
    {
      "id": "PROD001",
      "name": "Raspberry Pi 4",
      "description": "4GB RAM single-board computer",
      "price": 55.00,
      "inventory": 25,
      "category": "Hardware",
      "created_at": 1234567890
    }
  ]
}
```

#### Shopping Carts
**File**: `/db/carts.json`
```json
{
  "carts": [
    {
      "username": "user1",
      "items": [
        {
          "product_id": "PROD001",
          "qty": 2,
          "added_at": 1234567890
        }
      ],
      "created_at": 1234567890
    }
  ]
}
```

#### Orders
**File**: `/db/orders.json`
```json
{
  "orders": [
    {
      "order_id": "ORD12345",
      "username": "user1",
      "items": [
        {
          "product_id": "PROD001",
          "product_name": "Raspberry Pi 4",
          "qty": 2,
          "price": 55.00
        }
      ],
      "total": 110.00,
      "status": "pending",
      "shipping_address": "123 Main St",
      "shipping_city": "City",
      "shipping_zip": "12345",
      "created_at": 1234567890
    }
  ]
}
```

### Frontend Pages
- [shop.html](data/shop.html) - Product catalog with search, category filters, and cart preview
- [cart.html](data/cart.html) - Shopping cart management with quantity controls
- [checkout.html](data/checkout.html) - Order placement form with shipping address input
- [orders.html](data/orders.html) - Order history with IDOR testing panel
- [admin.html](data/admin.html) - Enhanced with shop management section (products & orders)

### Intentional Vulnerabilities

#### 1. IDOR on Orders
```javascript
// Access ANY user's orders without authorization
GET /api/shop/orders?username=admin

// View ANY order by ID with no auth check
GET /api/shop/order?order_id=ORD12345
```

#### 2. Order Manipulation
```javascript
// Change shipping address of ANY order (no ownership check)
POST /api/shop/order/update
{
  "order_id": "ORD12345",
  "shipping_address": "Attacker Address 456",
  "status": "shipped"
}
```

#### 3. Race Condition on Checkout
```bash
# Exploit: Multiple checkouts with same cart before balance deduction completes
# 100ms delay between balance check and deduction allows double-spending
curl -X POST "http://esp32.local/api/shop/checkout" -d "shipping_address=Addr1" &
curl -X POST "http://esp32.local/api/shop/checkout" -d "shipping_address=Addr2" &
```

#### 4. Order Deletion Without Refund
```javascript
// Delete any order - credits not refunded (financial loss)
POST /api/shop/order/delete
{ "order_id": "ORD12345" }
```

#### 5. Price Manipulation (Client-Side)
```javascript
// If frontend sends price instead of server calculating:
// Modify checkout request to set arbitrary price
// (Currently mitigated - server recalculates from DB)
```

### Testing Examples

#### IDOR Attack Chain
```bash
# 1. Create legitimate order as user1
curl -X POST "http://esp32.local/api/shop/checkout" \
  -H "Cookie: session=user1_session" \
  -d "shipping_address=123 Real St&shipping_city=NYC&shipping_zip=10001"

# 2. As attacker (user2), enumerate order IDs
for i in {1000..1100}; do
  curl "http://esp32.local/api/shop/order?order_id=ORD$i" -H "Cookie: session=user2_session"
done

# 3. Change shipping address of victim's order
curl -X POST "http://esp32.local/api/shop/order/update" \
  -H "Cookie: session=user2_session" \
  -d "order_id=ORD1045&shipping_address=Attacker Warehouse"
```

#### Race Condition Exploitation
```python
import requests
import threading

def checkout():
    requests.post("http://esp32.local/api/shop/checkout", 
                  cookies={"session": "user1_session"},
                  data={"shipping_address": "123 Main", "shipping_city": "NYC", "shipping_zip": "10001"})

# Launch 5 simultaneous checkouts with balance for only 1
threads = [threading.Thread(target=checkout) for _ in range(5)]
for t in threads: t.start()
```

---

## 🔧 Technical Implementation Details

### Authentication System
Both modules use shared authentication functions from [16_Wallet.ino](16_Wallet.ino):

```cpp
String getRequestUsername(AsyncWebServerRequest *request)
// Extracts username from session cookie or JWT token

String getRequestRole(AsyncWebServerRequest *request)  
// Returns user role (admin/user/guest) for authorization checks
```

### Vulnerability Flags
Controlled via [ESP32-H4CK.ino](ESP32-H4CK.ino) global flags:
```cpp
bool VULN_IDOR = true;              // Enable IDOR vulnerabilities
bool VULN_WEAK_AUTH = true;         // Bypass admin authentication
bool VULN_CSRF = true;              // No CSRF token validation
```

### Database Functions
Extended [05_Database.ino](05_Database.ino) with new functions:
- **Products**: `getAllProducts()`, `getProduct()`, `addProduct()`, `updateProduct()`, `deleteProduct()`
- **Carts**: `getCart()`, `addToCart()`, `updateCartItem()`, `removeFromCart()`, `clearCart()`
- **Orders**: `createOrder()`, `getOrders()`, `getOrder()`, `updateOrder()`, `deleteOrder()`, `getAllOrders()`, `getOrderStats()`

### Credit System Integration
```cpp
// Wallet balance check before checkout
float currentBalance = getUserBalance(username);
if (currentBalance < totalAmount) {
  request->send(400, "application/json", "{\"error\":\"Insufficient balance\"}");
  return;
}

// Deduct credits on successful order
withdrawFromWallet(username, totalAmount, "Order " + orderId);
```

---

## 📊 System Statistics

### Code Metrics
- **Total Lines Added**: ~2500 lines
  - 16_Wallet.ino: 624 lines
  - 17_Shop.ino: 583 lines
  - 05_Database.ino: +600 lines (shop functions)
  - HTML/CSS/JS: ~700 lines

- **API Endpoints**: 34 total
  - Wallet: 8 endpoints
  - Shop: 17 endpoints
  - Admin: 9 endpoints

- **Database Files**: 3 new JSON files
  - transactions.json
  - products.json
  - carts.json
  - orders.json

### Compilation Stats
```
Sketch uses 1,091,077 bytes (83%) of program storage
Global variables use 52,364 bytes (15%) of dynamic memory
Build successful ✅
```

---

## 🎯 Penetration Testing Scenarios

### Scenario 1: Financial Fraud via IDOR
**Objective**: Steal credits from other users
1. Register as low-privilege user "attacker"
2. Enumerate user IDs via `/api/wallet/balance?user_id=X`
3. Find wealthy user accounts (e.g., admin with 1000 credits)
4. Exploit race condition: Send multiple simultaneous transfers from admin to attacker
5. Verify attacker balance increased without proper authorization

### Scenario 2: E-Commerce Order Hijacking
**Objective**: Redirect victim's order to attacker's address
1. Victim places order for expensive product (e.g., laptop)
2. Attacker enumerates order IDs via `/api/shop/order?order_id=ORDXXXX`
3. Attacker finds victim's pending order
4. Exploit IDOR: `POST /api/shop/order/update` with attacker's shipping address
5. Order ships to attacker, victim loses credits

### Scenario 3: Privilege Escalation
**Objective**: Gain admin access to product/order management
1. Normal user bypasses auth checks when `VULN_WEAK_AUTH=true`
2. Add malicious product with negative price: `POST /api/shop/admin/product/add`
3. Purchase negative-price product to **gain** credits instead of spending
4. Access admin-only order statistics via `/api/shop/admin/orders`

### Scenario 4: Race Condition Double-Spend
**Objective**: Purchase products without sufficient balance
1. User has 100 credits, product costs 80 credits
2. Add product to cart
3. Launch 3 simultaneous checkout requests before balance update completes
4. Balance check passes for all 3 (each sees 100 credits available)
5. Result: 3 orders placed (240 credits spent) with only 100 credit balance

---

## 🛡️ Defense Mechanisms (For Learning)

### How to Fix IDOR
```cpp
// BEFORE (vulnerable)
String targetUser = request->getParam("user_id")->value();
float balance = getUserBalance(targetUser);

// AFTER (secure)
String loggedInUser = getRequestUsername(request);
if (!request->hasParam("user_id") || request->getParam("user_id")->value() != loggedInUser) {
  request->send(403, "application/json", "{\"error\":\"Access denied\"}");
  return;
}
float balance = getUserBalance(loggedInUser);
```

### How to Fix Race Conditions
```cpp
// Use mutex/semaphore for critical sections
SemaphoreHandle_t walletMutex = xSemaphoreCreateMutex();

void transfer(String from, String to, float amount) {
  xSemaphoreTake(walletMutex, portMAX_DELAY);
  
  float balance = getUserBalance(from);
  if (balance >= amount) {
    withdrawFromWallet(from, amount);
    depositToWallet(to, amount);
  }
  
  xSemaphoreGive(walletMutex);
}
```

### How to Fix Weak Authentication
```cpp
// Add proper role-based access control
String role = getRequestRole(request);
if (role != "admin") {
  request->send(403, "application/json", "{\"error\":\"Admin access required\"}");
  return;
}
```

---

## 📚 Additional Resources

### Related Documentation
- [QUICKSTART.md](QUICKSTART.md) - Initial setup and configuration
- [DEFENSE_SYSTEM.md](DEFENSE_SYSTEM.md) - Security controls reference
- [TESTING_DEFENSE.md](TESTING_DEFENSE.md) - Security testing guide
- [README.md](README.md) - Main project documentation

### Exploitation Tutorials
For detailed exploitation walkthroughs targeting these vulnerabilities:
1. OWASP Top 10 - A01:2021 Broken Access Control
2. PortSwigger Web Security Academy - IDOR Labs
3. HackTheBox - Web exploitation challenges

---

## ⚠️ Legal & Ethical Notice

**This system is designed exclusively for educational purposes and authorized security testing.**

- ✅ **Authorized Use**: Personal lab environments, CTF competitions, security training courses
- ❌ **Prohibited Use**: Unauthorized access to systems, production deployments, malicious exploitation

By using this code, you agree to:
1. Only test systems you own or have explicit written permission to test
2. Responsibly disclose any real-world vulnerabilities discovered
3. Comply with local laws and regulations regarding computer security

The authors assume no liability for misuse of this educational software.

---

## 🏆 Implementation Completed

**Date**: December 2024  
**Version**: ESP32-H4CK v2.0 (Wallet + Shop Update)  
**Build Status**: ✅ Compilation Successful (1,091,077 bytes / 83% flash usage)  
**Test Status**: ⚠️ Pending user validation

### Commits
- Previous: `bcb263f` - Full wallet system implementation
- Current: Pending commit - Shop system + wallet integration

---

## 🚀 Next Steps

1. **Flash to ESP32**: `./upload.sh` to deploy the system
2. **Initialize Database**: First boot will create `/db/` structure with sample data
3. **Test Vulnerabilities**: Use [orders.html](data/orders.html) testing panel
4. **Explore Frontend**: Navigate to `http://esp32.local/` for full UI
5. **Monitor Logs**: Use `./monitor.sh` to observe exploitation attempts

Happy hacking! 🎩🔓
