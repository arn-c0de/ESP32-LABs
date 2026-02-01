# ESP32-H4CK - Quick Start Guide

## ✅ Implementation Complete

All modules have been successfully created and are ready for deployment.

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
├── partitions.csv           ✓ Partition table (16MB)
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
| Web UI | `http://<ip>/` | Main interface |
| Login | `http://<ip>/login` | Authentication |
| Admin | `http://<ip>/admin` | Admin panel |
| REST API | `http://<ip>/api/*` | JSON API |
| WebSocket | `ws://<ip>/shell` | Interactive shell |
| Telnet | `telnet <ip> 23` | Remote shell |
| Debug | `http://<ip>/debug` | Info disclosure |

## 🐛 Vulnerable Endpoints (for Testing)

| Endpoint | Vulnerability Type | OWASP |
|----------|-------------------|-------|
| `/vuln/search?q=` | SQL Injection | A03 |
| `/vuln/comments` | XSS (Stored) | A03 |
| `/vuln/download?file=` | Path Traversal | A01 |
| `/vuln/ping?host=` | Command Injection | A03 |
| `/vuln/transfer` | CSRF | A01 |
| `/vuln/user?id=` | IDOR | A01 |
| `/debug` | Info Disclosure | A05 |
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

### Week 4: Advanced Exploitation
- CSRF attacks
- Path traversal
- Chaining vulnerabilities

## 🤝 Support

- Check Serial Monitor (115200 baud) for debug output
- Review README.md for detailed documentation
- Check ESP32 forums for hardware issues
- Verify library versions are compatible

## ✅ Verification Checklist

Before going live, verify:

- [ ] All .ino files compile without errors
- [ ] Filesystem uploaded successfully
- [ ] Serial Monitor shows "System Ready!"
- [ ] Can access web interface
- [ ] Can login with default credentials
- [ ] WebSocket shell connects
- [ ] Telnet service accepts connections
- [ ] Network is isolated from production
- [ ] Lab documentation prepared
- [ ] Students briefed on scope

## 📝 Version Information

- **Version:** 1.0.0
- **Created:** 2026
- **Platform:** ESP32 (Arduino Framework)
- **License:** Educational Use Only

---

**Ready to deploy!** Flash the firmware and start your penetration testing lab! 🚀
