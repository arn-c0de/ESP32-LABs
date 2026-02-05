# ESP32-H4CK - Quick Start Guide

**Version:** 1.0.3 | **Platform:** ESP32 | **Framework:** Arduino

## ✅ Implementation Complete

All core modules, wallet system, e-commerce shop, and lab mode controls are deployed and ready for penetration testing exercises.

## 📁 Project Structure

```
ESP32-H4CK1/
├── ESP32-H4CK.ino          ✓ Main sketch (setup/loop)
├── 01_Config.ino            ✓ Configuration & globals
├── 02_WiFi.ino              ✓ WiFi management
├── 03_WebServer.ino         ✓ HTTP server setup
├── 04_Auth.ino              ✓ Authentication & sessions
├── 05_Database.ino          ✓ File-based database
├── 06_API_REST.ino          ✓ RESTful API endpoints
├── 07_WebSocket.ino         ✓ WebSocket shell
├── 08_Telnet.ino            ✓ Telnet service
├── 09_Vulnerabilities.ino   ✓ Intentional vulnerabilities
├── 10_Crypto.ino            ✓ Cryptography utilities
├── 11_Utils.ino             ✓ Helper functions
├── 12_Debug.ino             ✓ Logging & monitoring
├── 13_PrivEsc.ino           ✓ Privilege escalation simulation
├── 14_AdvancedVulns.ino     ✓ Advanced vulnerabilities
├── 15_Defense.ino           ✓ 🎮 Defense & Gameplay system
├── 16_Wallet.ino            ✓ Wallet & credit system
├── 17_Shop.ino              ✓ E-commerce shop system
├── partitions.csv           ✓ Partition table (1.2MB LittleFS)
├── data/                    ✓ Web assets
│   ├── index.html           ✓ Home page
│   ├── login.html           ✓ Login page
│   ├── admin.html           ✓ Admin panel
│   └── shell.html           ✓ WebSocket shell UI
└── README.md                ✓ Full documentation
```

## 🚀 Next Steps

### 1. Configure WiFi Settings

Edit `01_Config.ino` lines 10-11:
```cpp
String WIFI_SSID = "YourNetworkName";      // Change this
String WIFI_PASSWORD = "YourPassword";     // Change this
```

### Configuration Parameters & Runtime Flags 🔧

You can configure the device at build time (using `.env` + `./build.sh`), by editing source defaults in `ESP32-H4CK.ino`/`01_Config.ino`, or at runtime via the device's configuration API and persistent Preferences storage.

**Common parameters**

- `WIFI_SSID` / `WIFI_PASSWORD` — Station (client) WiFi credentials. Set in `.env` or `ESP32-H4CK.ino` defaults.
- `AP_SSID` / `AP_PASSWORD` — Access Point SSID/password when running in AP mode.
- `STATION_MODE` — `true` to connect to an existing WiFi network (station), `false` to run AP-only. Default: `false`.
- `LAB_MODE` — Controls vulnerability visibility: `testing` (show hints), `pentest` (hide hints), `realism` (maximum security). Default: `testing`.
- `JWT_SECRET` — JWT signing secret (weak by default for lab). Set in `.env` or via compile-time define.
- `ENABLE_VULNERABILITIES` — `true` or `false` to enable/disable vulnerable endpoints. Default: `true` (lab mode).
- `DEBUG_MODE` — Enables verbose logging (default: `true`).
- `SSL_ENABLED` — Enable HTTPS (default: `false`).
- `ENABLE_TELNET` / `ENABLE_WEBSOCKET` — Toggle Telnet and WebSocket services.
- Ports: `HTTP_PORT`, `HTTPS_PORT`, `TELNET_PORT`, `WEBSOCKET_PORT`, `API_PORT` — Change in `ESP32-H4CK.ino` if needed.
- Telnet credentials: `TELNET_ADMIN_PASSWORD`, `TELNET_GUEST_PASSWORD`, `TELNET_ROOT_PASSWORD` — Defaults come from `.env`/`ESP32-H4CK.ino`.

**How to change values**

1. Edit `.env` (copy from `.env.example`) and run `./build.sh` — recommended for WiFi credentials and secrets.
2. Edit `ESP32-H4CK.ino` or `01_Config.ino` and rebuild to change compile-time defaults.
3. Change at runtime using the admin API (`/api/admin/config-update`) or by calling the device's configuration endpoints; settings are saved to flash by `saveConfigToFS()` and loaded on boot by `loadConfigFromFS()`.

**Recommended `.env` snippet**

```dotenv
WIFI_SSID=YourNetworkName
WIFI_PASSWORD=YourNetworkPassword
AP_SSID=ESP32-H4CK-AP
AP_PASSWORD=vulnerable
STATION_MODE=false
LAB_MODE=testing
JWT_SECRET=weak_secret_key_123
```

> ⚠️ Security note: This lab intentionally uses weak defaults. Never use production credentials or expose this device to the internet. Use isolated lab networks only.

### 2. Install Required Libraries

Open Arduino IDE > Sketch > Include Library > Manage Libraries

Search and install:
- **ESPAsyncWebServer** (by Me-No-Dev)
- **AsyncTCP** (by Me-No-Dev)
- **ArduinoJson** (by Benoit Blanchon)

### 3. Configure Arduino IDE

**Tools Menu Settings:**
- Board: "ESP32 Dev Module" (or your specific board)
- Upload Speed: 921600
- CPU Frequency: 240MHz
- Flash Size: 4MB/8MB/16MB (match your board)
- Flash Mode: QIO
- Partition Scheme: "Default 4MB with spiffs" or use custom partitions.csv
- PSRAM: "Enabled" (if available)
- Core Debug Level: "None" (for production)
- Port: (Select your ESP32's COM port)

### 4. Upload Filesystem

**Important:** Web assets must be uploaded to LittleFS before first use.

1. Install **Arduino ESP32 filesystem uploader**:
   - Download: https://github.com/me-no-dev/arduino-esp32fs-plugin
   - Extract to `Arduino/tools/ESP32FS/tool/esp32fs.jar`
   - Restart Arduino IDE

2. Upload filesystem:
   - Tools > ESP32 Sketch Data Upload
   - Wait for "SPIFFS Image Uploaded" message

### 5. Compile and Upload

1. Click **Verify** button (checkmark) to compile
2. Check for any errors in console
3. Click **Upload** button (arrow) to flash to ESP32
4. Open **Serial Monitor** (115200 baud) to see output

### 6. Access Your Lab

After successful upload, Serial Monitor will show:
```
========================================
  ESP32-H4CK Vulnerable Lab v1.0.0
========================================
[WIFI] IP Address: 192.168.1.xxx
```

Open browser to: `http://192.168.1.xxx/`

## 🔑 Default Credentials

- **admin** / **admin** (admin role)
- **root** / **root** (admin role)
- **guest** / **guest** (guest role)
- **test** / **test** (guest role)

## 🌐 Available Services

| Service | Endpoint | Description |
|---------|----------|-------------|
| Web UI | `http://<ip>/` | Main interface with unified navbar |
| Dashboard | `http://<ip>/dashboard` | User wallet dashboard |
| Shop | `http://<ip>/shop` | E-commerce product catalog |
| Login | `http://<ip>/login` | Authentication |
| Admin | `http://<ip>/admin` | Admin panel (wallet & shop management) |
| REST API | `http://<ip>/api/*` | JSON API endpoints |
| WebSocket | `ws://<ip>/shell` | Interactive shell |
| Telnet | `telnet <ip> 23` | Remote shell |

## 🐛 Vulnerable Endpoints (for Testing)

| Endpoint | Vulnerability Type | OWASP |
|----------|-------------------|-------|
| `/vuln/search?q=` | SQL Injection | A03 |
| `/vuln/comments` | XSS (Stored) | A03 |
| `/vuln/download?file=` | Path Traversal | A01 |
| `/vuln/ping?host=` | Command Injection | A03 |
| `/api/wallet/balance?user_id=` | IDOR (Wallet) | A01 |
| `/api/shop/order?order_id=` | IDOR (Orders) | A01 |
| `/api/wallet/transfer` | Race Condition | A04 |
| `/api/shop/checkout` | Race Condition | A04 |
| `/api/config` | Sensitive Data Exposure | A02 |

## 🛠️ Troubleshooting

### Compilation Errors

**Error: ESPAsyncWebServer.h not found**
- Install library via Library Manager

**Error: No such file or directory**
- Check all .ino files are in same folder
- Folder name must match main .ino file

**Error: Partition size too small**
- Select larger partition scheme in Tools menu
- Or use custom partitions.csv

### Upload Errors

**Serial port not found**
- Check USB cable connection
- Install CP210x or CH340 drivers
- Check device manager for port number

**Failed to connect to ESP32**
- Hold BOOT button during upload
- Try different USB cable
- Reduce upload speed to 115200

**Brownout detector triggered**
- Use better USB power supply (2A recommended)
- Add capacitor across power pins

### Runtime Issues

**WiFi won't connect**
- Check SSID/password in 01_Config.ino
- Try AP mode: Set `STATION_MODE = false`
- Check Serial Monitor for connection status

**Out of memory**
- Enable PSRAM in Tools menu
- Reduce concurrent connections
- Lower feature flags in 01_Config.ino

**Web pages 404**
- Upload filesystem with ESP32 Sketch Data Upload
- Check Serial Monitor: "Filesystem mounted"
- Verify files in data/ folder

## 📊 Memory Requirements

| Component | RAM Usage | Flash Usage |
|-----------|-----------|-------------|
| Core System | ~80KB | ~1.2MB |
| Web Server | ~40KB | ~200KB |
| WebSocket | ~15KB/client | - |
| Telnet | ~10KB/client | - |
| Web Assets | - | ~50KB |
| Database | ~2KB + data | Variable |

**Recommended:** ESP32 with PSRAM for best performance.

## 🔒 Security Warnings

⚠️ **CRITICAL**: This is an INTENTIONALLY VULNERABLE system!

**DO NOT:**
- Connect to production networks
- Expose to the internet
- Use real/sensitive data
- Deploy outside of isolated labs

**DO:**
- Use isolated VLAN
- Implement firewall rules
- Document all lab activities
- Get written permission before testing
- Have incident response plan

## 📚 Testing Examples

### SQL Injection Test
```
http://<ip>/vuln/search?q=' OR '1'='1
```

### XSS Test
```
Post to /vuln/comment: <script>alert('XSS')</script>
```

### Path Traversal Test
```
http://<ip>/vuln/download?file=../../passwords.txt
```

### Command Injection Test
```
http://<ip>/vuln/ping?host=127.0.0.1;ls
```

## 🎓 Learning Path

### Week 1: Reconnaissance
- Port scanning with nmap
- Service fingerprinting
- Web application enumeration

### Week 2: Authentication Attacks
- Default credentials
- Brute force attacks
- Session hijacking

### Week 3: Injection Attacks
- SQL injection
- Command injection
- XSS attacks

### Week 4: E-Commerce & Financial Attacks
- Wallet IDOR exploitation
- Race condition attacks on transfers
- Order manipulation and hijacking
- Shopping cart vulnerabilities

### Week 5: Advanced Exploitation
- CSRF attacks
- Path traversal
- Chaining vulnerabilities

### 🎮 Week 6: Red/Blue Team Defense Game
- Understanding resource constraints (DP/AP/SS)
- IP blocking strategies and false positives
- Rate limiting configuration and tuning
- Session management and incident response
- Cost/benefit analysis of defensive measures
- Multi-device coordination exercises

## 🎮 Defense System Quick Reference

The defense system simulates application-level security measures with resource management:

**Serial Commands (115200 baud)**
```bash
# IP Blocking
iptables -A INPUT -s 192.168.4.100 -j DROP --duration 30
iptables -D INPUT -s 192.168.4.100 -j DROP
iptables -L

# Rate Limiting
tc qdisc add rate-limit --src 192.168.4.0/24 --duration 60
tc qdisc del rate-limit --src 192.168.4.0/24
tc qdisc show

# Session Management
session reset --ip 192.168.4.101

# Status & Config
defense status
defense config show
defense config set dp=100 ap=10 stability=100
```

**Resource Costs (Defaults)**
- IP Block: DP=15, AP=1, 60s cooldown
- Rate Limit: DP=10, AP=1, 30s cooldown
- Session Reset: DP=25, AP=1, 90s cooldown

**Learning Objectives**
- Understand trade-offs between security and usability
- Practice resource-constrained decision making
- Learn realistic system administration commands
- Experience side-effects of aggressive defensive measures

## 🤝 Support

- Check Serial Monitor (115200 baud) for debug output
- Review README.md for detailed documentation
- Check ESP32 forums for hardware issues
- Verify library versions are compatible

## ✅ Verification Checklist

Before going live, verify:

- [ ] All .ino files compile without errors
- [ ] Filesystem uploaded successfully (1.2MB LittleFS partition)
- [ ] Serial Monitor shows "System Ready!" and "[DATABASE] Database initialized"
- [ ] Defense system initialized (check for "[DEFENSE]" messages)
- [ ] Wallet system loaded (check for "[WALLET]" messages)
- [ ] Shop database initialized (check for "[SHOP]" messages)
- [ ] Can access web interface with unified navbar
- [ ] Can login with default credentials
- [ ] Dashboard shows balance and transactions
- [ ] Shop displays products correctly
- [ ] Cart add/remove operations work
- [ ] WebSocket shell connects
- [ ] Telnet service accepts connections
- [ ] Defense commands work via serial (try `defense status`)
- [ ] LAB_MODE controls work (test `pentest` mode)
- [ ] Network is isolated from production
- [ ] Lab documentation prepared
- [ ] Students briefed on scope

## 📝 Version Information

- **Version:** 1.0.3
- **Release:** February 2026
- **Platform:** ESP32 (Arduino Framework)
- **License:** Educational Use Only
- **New in 1.0.3:**
  - LAB_MODE configuration (testing/pentest/realism)
  - Unified navbar with dropdown across all pages
  - Wallet banking system with credit management
  - E-commerce shop with cart and checkout
  - Expanded LittleFS partition (1.2MB)
  - English UI translations
  - Cookie-to-localStorage authentication sync

---

**Ready to deploy!** Flash the firmware and start your penetration testing lab! 🚀
