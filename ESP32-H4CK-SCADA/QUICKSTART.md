# ESP32-H4CK-SCADA - Quick Start Guide

**Version:** 2.0 | **Platform:** ESP32 | **Framework:** Arduino | **Lab Type:** Red Team (ICS/SCADA)

## ✅ Implementation Complete

All core SCADA modules, physics simulation, exploit paths, and defense system are deployed and ready for hands-on cybersecurity training.

## 📁 Project Structure

```
ESP32-H4CK-SCADA/
├── ESP32-H4CK-SCADA.ino     ✓ Main sketch (setup/loop)
├── 01_Config.ino             ✓ Configuration & globals
├── 02_WiFi.ino               ✓ WiFi management
├── 03_WebServer.ino          ✓ HTTP server setup
├── 03_HttpHelpers.ino        ✓ HTTP helper functions
├── 04_Auth.ino               ✓ Authentication & authorization
├── 05_Database.ino           ✓ LittleFS storage
├── 06_API_SCADA.ino          ✓ SCADA-specific API endpoints
├── 06_Physics.ino            ✓ Sensor physics simulation (4 production lines)
├── 07_Sensors.ino            ✓ Sensor management (20+ sensors)
├── 07_WebSocket.ino          ✓ WebSocket for real-time updates
├── 08_Actuators.ino          ✓ Actuator control system
├── 08_Telnet.ino             ✓ Telnet service
├── 09_Alarms.ino             ✓ Alarm management & history
├── 09_Vulnerabilities.ino    ✓ Exploit endpoints (6 paths)
├── 10_Crypto.ino             ✓ Cryptography utilities
├── 10_Safety.ino             ✓ Safety interlocks & constraints
├── 11_Incidents.ino          ✓ Incident generation & spawning
├── 11_Utils.ino              ✓ Helper functions
├── 12_Debug.ino              ✓ Logging & monitoring
├── 13_PrivEsc.ino            ✓ Privilege escalation vectors
├── 14_AdvancedVulns.ino      ✓ Advanced vulnerabilities
├── 15_Defense.ino            ✓ 🎮 Defense system (IDS/WAF/IP blocking)
├── partitions.csv            ✓ Partition table (1.2MB LittleFS)
├── data/                     ✓ Web assets (HTML/CSS/JS)
│   ├── html/
│   │   ├── index.html        ✓ Dashboard home
│   │   ├── dashboard.html    ✓ SCADA mimic panel & P&ID
│   │   ├── sensors.html      ✓ Sensor trends & data
│   │   ├── actuators.html    ✓ Actuator controls
│   │   ├── alarms.html       ✓ Alarm history & acknowledgment
│   │   ├── incidents.html    ✓ Incident tracking
│   │   └── vulnerabilities.html ✓ Vulnerability guides (EASY mode)
│   ├── css/
│   │   └── style.css         ✓ Stylesheets
│   └── js/
│       ├── mode.js           ✓ Dark mode toggle
│       ├── navbar.js         ✓ Navigation bar
│       └── auth-sync.js      ✓ Authentication sync
└── README.md                 ✓ Full documentation
```

## 🚀 Next Steps

### 1. Configure WiFi Settings

Edit `01_Config.ino` or create `.env` file:

```cpp
// In 01_Config.ino (lines ~10-15)
String WIFI_SSID = "YourNetworkName";      // Change this
String WIFI_PASSWORD = "YourPassword";     // Change this
String AP_SSID = "ESP32-SCADA-LAB";
String AP_PASSWORD = "vulnerable";
bool STATION_MODE = false;                 // true = connect to network, false = AP mode
```

### Configuration Parameters 🔧

You can configure the device at build time (`.env` + `./build.sh`), by editing source defaults in `01_Config.ino`, or at runtime via the API.

**Common parameters:**

- `WIFI_SSID` / `WIFI_PASSWORD` — Station (client) WiFi credentials. Set in `.env` or `01_Config.ino` defaults.
- `AP_SSID` / `AP_PASSWORD` — Access Point SSID/password when running AP mode.
- `STATION_MODE` — `true` to connect to existing WiFi (station), `false` for AP-only. **Default:** `false`
- `DIFFICULTY` — Exploit visibility: `EASY` (hints visible), `NORMAL` (hints via API), `HARD` (zero hints). **Default:** `NORMAL`
- `ENABLE_EXPLOIT_*` — Toggle individual exploit paths (IDOR, INJECTION, RACE, PHYSICS, FORENSICS, WEAK_AUTH). **Default:** `true`
- `DEFENSE_ENABLED` — Enable IDS/WAF/IP blocking. **Default:** `true` when DIFFICULTY >= NORMAL
- `ENABLE_INCIDENTS` — Auto-generate realistic incidents. **Default:** `true`
- `JWT_SECRET` — JWT signing secret (weak by default for lab).
- `DEBUG_MODE` — Verbose logging. **Default:** `true`

**How to change values:**

1. **Build time (Recommended for WiFi/secrets):** Create `.env` file (copy from `.env.example`), then run `./build.sh` and `./upload.sh`
2. **Edit source:** Modify `01_Config.ino` and recompile
3. **Runtime API (Admin only):** Use `/api/admin/config` endpoint to update settings persistently

**Recommended `.env` snippet:**

```dotenv
WIFI_SSID=YourLabNetworkSSID
WIFI_PASSWORD=YourNetworkPassword
AP_SSID=ESP32-SCADA-LAB
AP_PASSWORD=vulnerable
STATION_MODE=false
DIFFICULTY=NORMAL
JWT_SECRET=lab_secret_key_scada
DEBUG_MODE=true
```

> ⚠️ **Security note:** This lab intentionally uses weak defaults. Never connect to production networks, use real credentials, or expose to the internet.

### 2. Install Required Libraries

Open **Arduino IDE** > **Sketch** > **Include Library** > **Manage Libraries**

Search and install:
- **ESPAsyncWebServer** (by Me-No-Dev)
- **AsyncTCP** (by Me-No-Dev)
- **ArduinoJson** (by Benoit Blanchon)

### 3. Configure Arduino IDE

**Tools Menu Settings:**
- Board: `ESP32 Dev Module` (or your specific board)
- Upload Speed: `921600`
- CPU Frequency: `240MHz`
- Flash Size: `4MB` or larger
- Flash Mode: `QIO`
- Partition Scheme: `Default 4MB with spiffs` (or use custom partitions.csv)
- PSRAM: `Enabled` (if available)
- Core Debug Level: `None` (production)
- Port: (Select your ESP32's COM port)

### 4. Upload Filesystem

**IMPORTANT:** Web assets must be uploaded to LittleFS before first use.

1. **Install Arduino ESP32 filesystem uploader:**
   - Download: https://github.com/me-no-dev/arduino-esp32fs-plugin
   - Extract to `Arduino/tools/ESP32FS/tool/esp32fs.jar`
   - Restart Arduino IDE

2. **Upload filesystem:**
   - Click **Tools** > **ESP32 Sketch Data Upload**
   - Wait for "SPIFFS Image Uploaded" message

### 5. Compile and Upload

1. Click **Verify** (checkmark) to compile
2. Check for errors in console
3. Click **Upload** (arrow) to flash to ESP32
4. Open **Serial Monitor** (115200 baud) to monitor boot sequence

### 6. Access Your SCADA Lab

After successful upload, Serial Monitor will display:
```
========================================
  ESP32-H4CK-SCADA Lab v2.0
========================================
[SYSTEM] Boot sequence started...
[WIFI] Access Point Mode: SSID=ESP32-SCADA-LAB
[WIFI] IP Address: 192.168.4.1
[DATABASE] Database initialized
[PHYSICS] Sensor physics simulation active
[DEFENSE] Defense system ready
[SYSTEM] System Ready! 🚀
```

Open browser to: `http://192.168.4.1/` (or your device IP if in station mode)

## 🔑 Default Credentials

<details>
  <summary><strong>⚠️ SPOILER: Default Credentials - Click to reveal</strong></summary>

- **admin** / **admin123** (full access, flag submission)
- **operator** / **operator** (limited access, read-only)
- **guest** / **guest** (monitoring only)

</details>

## 🌐 Available Services

<details>
  <summary><strong>⚠️ SPOILER: Available Services & Pages - Click to reveal</strong></summary>

| Service | URL | Description |
|---------|-----|-------------|
| Dashboard | `http://<ip>/` | SCADA mimic panel with P&ID |
| Sensors | `http://<ip>/sensors` | Real-time sensor trends |
| Actuators | `http://<ip>/actuators` | Motor & valve controls |
| Alarms | `http://<ip>/alarms` | Alarm acknowledgment panel |
| Incidents | `http://<ip>/incidents` | Incident tracking & timeline |
| Vulnerabilities | `http://<ip>/vulnerabilities` | Exploit guides (EASY mode only) |
| REST API | `http://<ip>/api/*` | JSON API endpoints |
| WebSocket | `ws://<ip>/` | Real-time updates |
| Telnet | `telnet <ip> 23` | Remote terminal |

</details>

## 🐛 Exploit Paths (6 Independent Vectors)

<details>
  <summary><strong>⚠️ SPOILER: Contains Vulnerable Endpoints - Click to reveal</strong></summary>

| Path # | Vulnerability | Endpoint(s) | OWASP |
|--------|----------------|------------|-------|
| **1** | IDOR (Insecure Direct Object Reference) | `GET /api/sensor/reading?sensor_id=SENSOR-*` | A01 |
| **2** | Command Injection | `POST /api/actuators/control` (unsanitized params) | A03 |
| **3** | Race Condition | `POST /api/test/race?actuator=*&count=N` | A04 |
| **4** | Physics-Based Analysis | Sensor anomaly detection & cross-correlation | Logic Flaw |
| **5** | Forensics | Sensitive data in logs & maintenance records | A02 |
| **6** | Weak Authentication | Hardcoded credentials in maintenance endpoint | A07 |

**Learning Focus:**
- Path 1 (IDOR): Authorization bypass, privilege escalation
- Path 2 (Injection): Input validation, command execution
- Path 3 (Race): Concurrency exploits, state manipulation
- Path 4 (Physics): Anomaly detection, system behavior understanding
- Path 5 (Forensics): Log analysis, pattern recognition
- Path 6 (Auth): Credential discovery, social engineering

</details>

## 🎯 Difficulty Levels

### EASY Mode (Beginners)
- All vulnerabilities **explicitly visible**
- Step-by-step hints in web UI
- Defense **disabled**
- Predictable incident patterns
- **Use Case:** Security awareness, introductory training

### NORMAL Mode (Balanced)
- Vulnerabilities require **basic discovery**
- Hints available via `/api/hints` (must request)
- Defense **active**
- 1-2 realistic incidents per session
- **Use Case:** CTF-style labs, mid-level training

### HARD Mode (Advanced)
- Vulnerabilities **hidden** (discovery required)
- **Zero hints** available
- Defense **fully active** with all countermeasures
- 3+ cascading incidents
- Forensic evidence minimal
- **Use Case:** Red team training, expert challenges

## 🛠️ Troubleshooting

### Compilation Errors

**Error: ESPAsyncWebServer.h not found**
- Install library via Library Manager

**Error: No such file or directory**
- Check all .ino files are in same folder
- Folder name must match main .ino filename

**Error: Partition size too small**
- Select larger partition scheme (min 4MB)
- Or use custom partitions.csv

### Upload Errors

**Serial port not found**
- Check USB cable connection
- Install CP210x or CH340 drivers (common with ESP32 boards)
- Check Device Manager for port number

**Failed to connect to ESP32**
- Hold **BOOT** button while uploading
- Try different USB cable
- Reduce upload speed to 115200

**Brownout detector triggered**
- Use quality USB power supply (≥2A recommended)
- Add capacitor (100µF) across power pins

### Runtime Issues

**WiFi won't connect**
- Check SSID/password in `.env` or `01_Config.ino`
- Try AP mode: Set `STATION_MODE = false` (default)
- Monitor Serial output: look for `[WIFI]` messages

**Out of memory**
- Enable PSRAM in Tools menu (if available)
- Reduce concurrent WebSocket connections
- Lower feature flags in `01_Config.ino`

**Web pages 404 or missing UI**
- **Upload filesystem** with "ESP32 Sketch Data Upload"
- Check Serial Monitor: "Filesystem mounted"
- Verify `data/` folder structure exists

**Sensors not updating / Physics frozen**
- Check Serial Monitor for `[PHYSICS]` errors
- Verify `ENABLE_PHYSICS = true` in `01_Config.ino`
- Restart device and check boot sequence

**Defense blocking legitimate traffic**
- Use Serial Monitor to list IP blocks: `iptables -L` (simulated)
- Clear blocks: `iptables -F` (simulated)
- Reduce defense aggressiveness in config

## 📊 Memory Requirements

| Component | RAM Usage | Flash Usage |
|-----------|-----------|-------------|
| Core System | ~100KB | ~1.2MB |
| SCADA Physics Engine | ~60KB | ~150KB |
| Sensor Manager (20+) | ~40KB | ~50KB |
| WebSocket Real-time | ~30KB/client | - |
| Web Interface | - | ~150KB |
| Defense System | ~25KB | ~100KB |
| Database | ~5KB + data | Variable |

**Recommended:** ESP32 with PSRAM and ≥4MB flash

## 🔒 Security Warnings

⚠️ **CRITICAL:** This is an **INTENTIONALLY VULNERABLE** SCADA simulator!

**DO NOT:**
- Connect to production ICS/SCADA systems or networks
- Expose to the internet or untrusted networks
- Use real company data or credentials
- Enable outside of controlled lab environments
- Run alongside critical infrastructure

**DO:**
- Use **isolated lab network** (dedicated VLAN, air-gapped preferred)
- Implement network firewall rules
- Document all lab activities and findings
- Get **written permission** before penetration testing
- Have incident response and recovery procedures
- Brief students/participants on lab scope and rules

## 📚 Learning Path

### Week 1: Reconnaissance & SCADA Fundamentals
- Port scanning with nmap
- Service fingerprinting
- SCADA protocol discovery
- Sensor list enumeration: `GET /api/sensors/list`

### Week 2: Authentication & Authorization
- Default credentials discovery
- JWT token analysis
- Cross-line access control bypass (IDOR)
- Role-based access testing

### Week 3: IDOR Exploitation (Path 1)
- Identify sensor ID patterns
- Access unauthorized sensor data: `GET /api/sensor/reading?sensor_id=SENSOR-L2-03`
- Extract historical trends
- Cross-line data exfiltration

### Week 4: Command Injection (Path 2)
- Actuator parameter fuzzing
- Payload encoding detection
- Simulate shell command execution
- Control production lines maliciously

### Week 5: Race Condition Attacks (Path 3)
- Concurrency testing endpoints
- State corruption techniques
- Triggering safety bypass
- Sensor value manipulation

### Week 6: Physics-Based Analysis (Path 4)
- Learn sensor physics behavior
- Cross-correlation analysis
- Anomaly pattern detection
- Inference-based system understanding

### Week 7: Forensics & Log Analysis (Path 5)
- Examine system logs
- Extract maintenance records
- Find hardcoded secrets
- Timeline reconstruction

### Week 8: Weak Authentication (Path 6)
- Discover weak credentials
- Maintenance endpoint access
- Privilege escalation to admin
- Persistence techniques

### Week 9: Defense & Incident Response
- Understand defense system constraints
- IP blocking and rate limiting
- Session management
- Resource-aware decision making

### Week 10: Competition & Leaderboard
- Multi-path exploitation coordination
- Speed-running challenges
- Blue team defense strategy
- Advanced evasion techniques

## 🎮 Defense System Quick Reference

The defense system simulates real ICS/SCADA monitoring with resource management:

**Serial Commands (115200 baud):**
```bash
# IP Blocking
iptables -A INPUT -s 192.168.4.100 -j DROP --duration 120
iptables -D INPUT -s 192.168.4.100 -j DROP
iptables -L

# Rate Limiting
tc qdisc add rate-limit --src 192.168.4.0/24 --duration 60
tc qdisc del rate-limit
tc qdisc show

# Session Management
session reset --ip 192.168.4.101 --reason "breach_detected"
session list

# Status & Monitoring
defense status
defense config show
defense config set dp=100 ap=10 sensitivity=high
```

**Resource System (Defaults):**
- IP Block: DP=15, AP=1, 60s cooldown, 120s duration
- Rate Limit: DP=10, AP=1, 30s cooldown, 60s duration
- Session Reset: DP=25, AP=1, 90s cooldown, 5s duration

**Learning Objectives:**
- Understand defense resource constraints
- Practice cost/benefit analysis of countermeasures
- Learn realistic ICS defense command syntax
- Experience side-effects of aggressive blocking
- Adapt techniques under defensive pressure

## 🏆 Scoring & Leaderboard

**Metric:** `(exploits_found, time_to_solve)`

- **Primary:** Unique exploit paths discovered (0-6)
- **Secondary:** Time elapsed in minutes (lower = better)
- **Bonus:** Sub-flags, incidents resolved, defense evasions

**Formula:**
```
score = 100 +
        (exploit_paths × 500) +           # PRIMARY
        max(0, 3000 - time_min × 5) +     # SECONDARY
        (sub_flags × 50) +
        (incidents_resolved × 25) +
        (defense_evasions × 100) -
        (hints_requested × 10)
```

**Leaderboard Example:**
```
Rank | Name    | Difficulty | Exploits | Time (min) | Score
-----|---------|------------|----------|------------|-------
1    | Alice   | HARD       | 6/6      | 45         | 5050
2    | Bob     | NORMAL     | 5/6      | 38         | 3850
3    | Charlie | EASY       | 6/6      | 22         | 4100
```

## 📊 Testing Examples

<details>
  <summary><strong>⚠️ SPOILER: Contains Exploit Examples - Lab Solutions - Click to reveal</strong></summary>

### Test 1: IDOR (Path 1)
```bash
# List available sensors
curl -s 'http://192.168.4.1/api/sensors/list' | jq .

# Access sensor data (IDOR vector)
curl -s 'http://192.168.4.1/api/sensor/reading?sensor_id=SENSOR-L2-03&limit=50' | jq .
```

### Test 2: Command Injection (Path 2)
```bash
curl -X POST 'http://192.168.4.1/api/actuators/control' \
  -H 'Content-Type: application/json' \
  -d '{"id":"MOTOR-L2-01","cmd":"set","params":{"speed":"80;simulate:cat_/data/db"}}'
```

### Test 3: Race Condition (Path 3)
```bash
curl -s 'http://192.168.4.1/api/test/race?actuator=MOTOR-L2-01&count=50'
```

### Test 4: Request Hints (NORMAL/EASY modes only)
```bash
curl -s 'http://192.168.4.1/api/hints?endpoint=GET_/api/sensor/reading&level=1'
```

### Test 5: Physics Anomaly Detection
```bash
# Monitor sensor trends for anomalies
curl -s 'http://192.168.4.1/api/sensor/reading?sensor_id=TEMP-L1-01' | jq '.data | map(.value)'
```

### Test 6: Access Maintenance Log (Weak Auth)
```bash
curl -s 'http://192.168.4.1/api/maintenance/log'
```

</details>

## ✅ Verification Checklist

Before deploying students, verify:

- [ ] All .ino files compile without errors
- [ ] Filesystem uploaded successfully (1.2MB+ LittleFS)
- [ ] Serial Monitor shows full boot sequence
- [ ] System displays "System Ready! 🚀" message
- [ ] `[PHYSICS]` module initialized
- [ ] `[DEFENSE]` system initialized
- [ ] Can access web interface (Dashboard loads)
- [ ] Can login with default credentials (admin/admin123)
- [ ] Dashboard displays P&ID and sensor values
- [ ] Sensor trends graph working
- [ ] Actuator controls responsive
- [ ] Alarm panel populated
- [ ] WebSocket real-time updates working
- [ ] Telnet accepts connections: `telnet <ip> 23`
- [ ] `/api/sensors/list` returns valid JSON
- [ ] `/api/actuators/list` returns actuators
- [ ] Defense commands work via serial
- [ ] LAB_MODE switching works (EASY/NORMAL/HARD)
- [ ] Leaderboard endpoint accessible
- [ ] Network is **isolated** from production
- [ ] Lab rules and scope documented
- [ ] Students briefed on objectives
- [ ] Incident response plan prepared

## 📝 Version Information

- **Version:** 2.0
- **Release:** February 2026
- **Platform:** ESP32 (Arduino Framework)
- **Lab Type:** Red Team (ICS/SCADA)
- **License:** Educational Use Only

**Key Features:**
- 6 independent exploit paths
- 4 production lines with physics simulation
- 20+ realistic sensors
- Difficulty-based hint system
- Dynamic defense system (IDS/WAF/IP blocking)
- Competitive leaderboard with scoring
- Forensic incident tracking
- WebSocket real-time updates

---

**Ready to deploy!** Flash the firmware and begin your SCADA penetration testing training! 🚀

For detailed documentation, see [README.md](README.md)

