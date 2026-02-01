# ESP32-H4CK - "SecureNet Solutions" Vulnerable Lab

## Project Overview

ESP32-H4CK is an intentionally vulnerable web application designed for cybersecurity training and penetration testing education. It runs entirely on an ESP32 microcontroller and provides multiple services with various security vulnerabilities for students to discover and exploit in a controlled lab environment.

⚠️ **WARNING**: This system contains intentional security vulnerabilities. Deploy ONLY in isolated lab environments. DO NOT expose to production networks or the internet.

## Features

### Services
- **HTTP/HTTPS Web Server** (Port 80/443)
  - Admin and Guest user roles
  - Session management
  - RESTful API
  
- **WebSocket Shell** (ws://device/shell)
  - Interactive command execution
  - Real-time communication
  
- **Telnet Service** (Port 23)
  - Remote shell access
  - Multiple concurrent connections

### Intentional Vulnerabilities

This lab implements various OWASP Top 10 vulnerabilities:

1. **SQL Injection** - `/vuln/search?q=`
2. **Cross-Site Scripting (XSS)** - `/vuln/comments`
3. **Path Traversal** - `/vuln/download?file=`
4. **Command Injection** - `/vuln/ping?host=`
5. **CSRF** - `/vuln/transfer`
6. **Broken Authentication** - Weak password policies, default credentials
7. **Security Misconfiguration** - Debug endpoints exposed
8. **Information Disclosure** - `/debug`, `/api/config`
9. **Insecure Direct Object References** - `/vuln/user?id=`
10. **Weak Cryptography** - Plaintext password storage (in vulnerable mode)

## Hardware Requirements

- **ESP32 Development Board** (recommended: ESP32-WROVER with PSRAM)
- **Minimum 4MB Flash** (8-16MB recommended)
- **PSRAM** (optional but recommended for better performance)
- **USB Cable** for programming
- **WiFi Network** or use Access Point mode

## Software Requirements

- **Arduino IDE** 1.8.x or 2.x
- **ESP32 Board Support** (via Board Manager)
- **Required Libraries**:
  - ESPAsyncWebServer
  - AsyncTCP
  - ArduinoJson
  - LittleFS (included in ESP32 core)

## Installation

### 1. Install Arduino IDE and ESP32 Support

1. Download and install Arduino IDE from https://www.arduino.cc/
2. Add ESP32 board support:
   - Go to File > Preferences
   - Add to "Additional Board Manager URLs": 
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
   - Go to Tools > Board > Boards Manager
   - Search for "ESP32" and install

### 2. Install Required Libraries

Install via Arduino Library Manager (Sketch > Include Library > Manage Libraries):

- **ESPAsyncWebServer** by Me-No-Dev
- **AsyncTCP** by Me-No-Dev
- **ArduinoJson** by Benoit Blanchon

### 3. Configure Board Settings

1. Connect your ESP32 board
2. Go to Tools menu:
   - Board: "ESP32 Dev Module" (or your specific board)
   - Flash Size: "16MB" (or your board's size)
   - Partition Scheme: "Custom" and select `partitions.csv`
   - PSRAM: "Enabled" (if available)
   - Upload Speed: "921600"
   - Port: Select your ESP32's port

### 4. Upload Filesystem

1. Install **ESP32 Sketch Data Upload** plugin
2. Place web files in `data/` folder
3. Go to Tools > ESP32 Sketch Data Upload
4. Wait for upload to complete

### 5. Upload Code

1. Open `ESP32-H4CK.ino`
2. Configure WiFi credentials in `01_Config.ino`:
   ```cpp
   String WIFI_SSID = "YourNetworkName";
   String WIFI_PASSWORD = "YourPassword";
   ```
3. Click Upload button
4. Open Serial Monitor (115200 baud) to see device IP

## Configuration

### WiFi Modes

**Station Mode** (default):
```cpp
bool STATION_MODE = true;
```
Connects to existing WiFi network.

**Access Point Mode**:
```cpp
bool STATION_MODE = false;
```
Creates its own WiFi network (SSID: ESP32-H4CK-AP).

### Feature Flags

In `01_Config.ino`:

```cpp
bool ENABLE_VULNERABILITIES = true;  // Enable vulnerable endpoints
bool DEBUG_MODE = true;              // Enable debug logging
bool SSL_ENABLED = false;            // SSL/TLS support
bool ENABLE_TELNET = true;           // Enable telnet service
bool ENABLE_WEBSOCKET = true;        // Enable WebSocket shell
```

### Default Credentials

- **Admin**: admin / admin
- **Root**: root / root
- **Guest**: guest / guest
- **Test**: test / test

## Usage

### Accessing the Lab

1. Power on the ESP32
2. Wait for WiFi connection (check Serial Monitor for IP)
3. Open browser to `http://<device-ip>/`
4. Login with default credentials
5. Explore vulnerable endpoints

### Web Endpoints

- `/` - Home page with endpoint listing
- `/login` - Login page
- `/admin` - Admin panel (requires auth)
- `/api/info` - System information
- `/api/users` - User management API
- `/shell.html` - WebSocket shell
- `/debug` - Debug information (vulnerability)
- `/vuln/*` - Various vulnerable endpoints

### Telnet Access

```bash
telnet <device-ip> 23
```

### WebSocket Shell

Connect via browser at `http://<device-ip>/shell.html` or use WebSocket client:
```javascript
ws://<device-ip>/shell
```

## File Structure

```
ESP32-H4CK1/
├── ESP32-H4CK.ino          # Main sketch
├── 01_Config.ino            # Configuration
├── 02_WiFi.ino              # WiFi management
├── 03_WebServer.ino         # Web server setup
├── 04_Auth.ino              # Authentication
├── 05_Database.ino          # Database operations
├── 06_API_REST.ino          # REST API
├── 07_WebSocket.ino         # WebSocket handler
├── 08_Telnet.ino            # Telnet service
├── 09_Vulnerabilities.ino   # Vulnerable endpoints
├── 10_Crypto.ino            # Cryptography functions
├── 11_Utils.ino             # Utility functions
├── 12_Debug.ino             # Debug & logging
├── partitions.csv           # Partition table
├── data/                    # Web assets
│   ├── index.html
│   ├── login.html
│   ├── admin.html
│   └── shell.html
└── README.md
```

## Security Considerations for Lab Deployment

### Network Isolation

1. **Use dedicated VLAN** for lab devices
2. **Firewall rules** to block internet access
3. **Isolated WiFi network** (separate SSID)
4. **No connection to production networks**

### Physical Security

1. Lock down USB ports when not in use
2. Clearly label devices as "VULNERABLE LAB EQUIPMENT"
3. Secure physical access to lab area

### Legal & Ethical

1. **Signed participant agreement** before lab access
2. **Clear usage policies** and acceptable use guidelines
3. **Logging and monitoring** of all lab activities
4. **Incident response plan** in place

## Troubleshooting

### Device won't connect to WiFi
- Check SSID and password in `01_Config.ino`
- Try Access Point mode instead
- Check Serial Monitor for error messages

### Out of memory errors
- Enable PSRAM if available
- Reduce concurrent connections
- Disable unused features

### Upload fails
- Check correct port is selected
- Try lower upload speed (115200)
- Press BOOT button during upload
- Check USB cable

### Web pages not loading
- Verify filesystem was uploaded (ESP32 Sketch Data Upload)
- Check Serial Monitor for file system errors
- Re-upload filesystem data

## Learning Exercises

### Beginner Level
1. Discover and exploit default credentials
2. Test XSS vulnerability in comment system
3. Information disclosure via debug endpoints
4. Session hijacking with predictable session IDs

### Intermediate Level
1. SQL injection to bypass authentication
2. Path traversal to read system files
3. CSRF attack on user management
4. Command injection in ping endpoint

### Advanced Level
1. Chain multiple vulnerabilities
2. Write automated exploitation scripts
3. Develop patches for vulnerabilities
4. Implement proper security controls

## Development

### Adding New Vulnerabilities

1. Add vulnerability flag in `01_Config.ino`
2. Implement endpoint in `09_Vulnerabilities.ino`
3. Document vulnerability type and exploitation method
4. Add to web interface in `data/index.html`

### Extending Functionality

Each module (01-12) is independent. To add features:
1. Create new `.ino` file with numeric prefix
2. Add forward declarations in main sketch
3. Call initialization in `setup()`
4. Add any loop operations to `loop()`

## Contributing

This is an educational project. Contributions welcome:
- Additional vulnerabilities
- Better documentation
- Bug fixes
- UI improvements

## License

Educational Use Only - Not for production deployment

## Credits

Created for cybersecurity education and penetration testing training.

## Support

For issues and questions, please check:
- Serial Monitor output for debug information
- ESP32 documentation
- Arduino forums

---

**Remember**: This is an intentionally vulnerable system. Use responsibly and only in controlled lab environments!
