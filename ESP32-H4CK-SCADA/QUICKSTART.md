# ESP32-H4CK-SCADA - Quick Start Guide

**Version:** 2.0 | **Platform:** ESP32 | **Framework:** Arduino | **Lab Type:** Red Team (ICS/SCADA)

## ✅ Implementation Complete

All core SCADA modules, physics simulation, exploit paths, and defense system are deployed and ready for hands-on cybersecurity training.

---

## 🚀 Installation (Simple)

### Just Run This:

```bash
cd ESP32-H4CK-SCADA
./upload.sh
```

The script handles **everything**:
- ✅ Creates Python virtual environment
- ✅ Installs dependencies
- ✅ Compiles the sketch
- ✅ Builds LittleFS filesystem
- ✅ Uploads firmware to ESP32
- ✅ Uploads web assets (HTML/CSS/JS)

That's it! ✨

---

## 📋 Prerequisites

1. **Hardware:**
   - ESP32 development board (any variant, ≥4MB flash)
   - USB-A to Micro-USB cable
   - Connect to Linux/macOS/Windows machine with Python 3

2. **Software (from your OS package manager):**
   ```bash
   # Linux
   sudo apt-get install python3 python3-pip arduino-cli esptool

   # macOS
   brew install python3 arduino-cli esptool
   ```

3. **Arduino IDE or `arduino-cli` configured** (for esp32 board support):
   ```bash
   arduino-cli board list                    # Verify ESP32 board is detected
   arduino-cli core install esp32:esp32      # If not installed yet
   ```

---

## 🔌 Connect Hardware

1. Plug USB cable into ESP32
2. Run `./upload.sh`
3. Select your ESP32 port when prompted (e.g., `/dev/ttyUSB0`)
4. Watch the upload process in terminal

**Done!** The device will boot automatically.

---

## 📡 Access Your SCADA Lab

### Via Serial Monitor (Debug)

```bash
# Watch boot sequence
stty -F /dev/ttyUSB0 115200
cat /dev/ttyUSB0
```

You'll see:
```
========================================
  ESP32-H4CK-SCADA Lab v2.0
========================================
[SYSTEM] Boot sequence started...
[WIFI] Access Point Mode: SSID=ESP32-H4CK-AP password=vulnerable
[WIFI] IP Address: 192.168.4.1
[PHYSICS] Sensor physics simulation active
[DEFENSE] Defense system initialized
[SYSTEM] System Ready! 🚀
```

### Via Web Browser

**Default AP Mode:**
- SSID: `ESP32-H4CK-AP`
- Password: `vulnerable`
- URL: `http://192.168.4.1/`

**If connected to WiFi:**
- Find IP in Serial Monitor
- URL: `http://<your-ip>/`

---

## 🔑 Default Credentials

| Username | Password | Role | Access |
|----------|----------|------|--------|
| **admin** | **admin** | admin | Full access, all APIs |
| **operator** | **operator123** | operator | Sensor/actuator control |
| **maintenance** | **maint456** | maintenance | Maintenance APIs |
| **viewer** | **viewer** | viewer | Read-only monitoring |

---

## 🌐 Available Pages

| Page | URL | Description |
|------|-----|-------------|
| Dashboard | `http://<ip>/` or `/dashboard` | P&ID mimic panel, status |
| Sensors | `/sensors` | Real-time sensor trends |
| Actuators | `/actuators` | Motor/valve control interface |
| Alarms | `/alarms` | Alarm history & acknowledgment |
| Incidents | `/incidents` | Incident timeline & analysis |
| Vulnerabilities | `/vulnerabilities` | (EASY mode only) exploit hints |
| Admin | `/admin` | Admin controls & configuration |
| Defense | `/defense` | Defense system status |

---

## 🔌 Web APIs

### Authentication

```bash
# Login
curl -X POST 'http://192.168.4.1/api/login' \
  -H 'Content-Type: application/json' \
  -d '{"username":"admin","password":"admin"}'

# Returns JWT token in response
```

### Sensors

```bash
# List all sensors
curl 'http://192.168.4.1/api/sensors' -H 'Authorization: Bearer <TOKEN>'

# Get sensor readings (IDOR vulnerability - no per-line auth)
curl 'http://192.168.4.1/api/sensors/readings?sensor_id=TEMP-L1-01&limit=100' \
  -H 'Authorization: Bearer <TOKEN>'

# Control sensor (enable/disable)
curl -X POST 'http://192.168.4.1/api/sensors/control' \
  -H 'Authorization: Bearer <TOKEN>' \
  -H 'Content-Type: application/json' \
  -d '{"sensor_id":"TEMP-L1-01","enabled":false}'
```

### Actuators

```bash
# List all actuators
curl 'http://192.168.4.1/api/actuators' -H 'Authorization: Bearer <TOKEN>'

# Control actuator (INJECTION vulnerability)
curl -X POST 'http://192.168.4.1/api/actuators/control' \
  -H 'Authorization: Bearer <TOKEN>' \
  -H 'Content-Type: application/json' \
  -d '{"actuator_id":"MOTOR-L1-01","cmd":"set_speed","value":80}'
```

### Alarms

```bash
# Get alarm history
curl 'http://192.168.4.1/api/alarms' -H 'Authorization: Bearer <TOKEN>'

# Acknowledge alarm
curl -X POST 'http://192.168.4.1/api/alarms/acknowledge' \
  -H 'Authorization: Bearer <TOKEN>' \
  -H 'Content-Type: application/json' \
  -d '{"alarm_id":"ALARM-0001"}'
```

---

## 🐛 Exploit Paths (6 Vectors)

### Path 1: IDOR (Insecure Direct Object Reference)
**Vulnerability:** Sensor readings lack per-line access control  
**Endpoint:** `GET /api/sensors/readings?sensor_id=SENSOR-L2-03`  
**Learning:** Authorization bypass, cross-line data exfiltration  
**Exploit:** Request sensor IDs from unauthorized production lines

### Path 2: Command Injection
**Vulnerability:** Actuator commands not sanitized  
**Endpoint:** `POST /api/actuators/control` (cmd parameter)  
**Learning:** Input validation, shell injection techniques  
**Exploit:** Inject commands via `cmd` or `value` parameters

### Path 3: Race Condition
**Vulnerability:** Actuator state changes without mutex locks  
**Endpoint:** Rapid sequential actuator control requests  
**Learning:** Concurrency exploits, state corruption  
**Exploit:** Send overlapping control requests to corrupt state

### Path 4: Physics-Based Analysis
**Vulnerability:** Sensor anomaly detection can be inferred  
**Endpoint:** `GET /api/sensors/readings` (trend analysis)  
**Learning:** Cross-correlation, system behavior understanding  
**Exploit:** Correlate patterns across multiple sensors to predict system state

### Path 5: Forensics / Information Disclosure
**Vulnerability:** Debug endpoints expose internal state  
**Endpoint:** `/vuln/debug` (when ENABLE_VULNERABILITIES=true)  
**Learning:** Log analysis, data reconstruction  
**Exploit:** Extract session tokens, IP addresses, system uptime

### Path 6: Weak Authentication
**Vulnerability:** Default credentials visible in code/config  
**Endpoint:** `/api/login` with hardcoded user/password pairs  
**Learning:** Credential discovery, default password exploitation  
**Exploit:** Use `admin/admin` or `operator/operator123` credentials

---

## 🎯 Difficulty Levels

### EASY Mode
- **Hints visible** in `/vulnerabilities` page
- Defense **disabled**
- Default credentials prominently displayed
- **Use for:** Complete beginners, awareness training

### NORMAL Mode (Default)
- Vulnerabilities require **basic discovery**
- Hints available via `/api/hints` API (must request)
- Defense **enabled**
- 1-2 realistic incidents per session
- **Use for:** CTF-style labs, hands-on training

### HARD Mode
- Vulnerabilities **completely hidden**
- **Zero hints** available
- Defense **fully active** with all countermeasures
- 3+ cascading incidents
- Minimal forensic evidence
- **Use for:** Red team exercises, expert challenges

### Change Difficulty At Runtime

```bash
# Via API (admin only)
curl -X POST 'http://<ip>/api/admin/difficulty' \
  -H 'Authorization: Bearer <ADMIN_TOKEN>' \
  -H 'Content-Type: application/json' \
  -d '{"difficulty":"HARD"}'

# Via Serial (115200 baud)
difficulty HARD
```

---

## 🎮 Defense System (Instructor Mode)

The defense system responds to instructor commands via Serial:

```bash
# Block attacker IP (simulated iptables)
iptables -A INPUT -s 192.168.4.100 -j DROP --duration 120

# Enable rate limiting
tc qdisc add rate-limit --src 192.168.4.0/24 --duration 60

# Reset student session
session reset --ip 192.168.4.101

# View current blocks
iptables -L

# View defense status
defense status
```

---

## 🔧 Configuration

### Build-Time Configuration (`.env`)

Create a `.env` file in `ESP32-H4CK-SCADA/`:

```dotenv
# WiFi (station mode - connect to existing network)
WIFI_SSID=YourNetworkName
WIFI_PASSWORD=YourPassword
STATION_MODE=false

# Lab configuration
DIFFICULTY=NORMAL
JWT_SECRET=your_secret_key_here
DEBUG_MODE=true
ENABLE_VULNERABILITIES=true
ENABLE_INCIDENTS=true

# Defense
DEFENSE_ENABLED=true
```

Then run `./upload.sh` to apply.

### Runtime Configuration

```bash
# Via web UI: Login → Admin panel
# Via API: POST /api/admin/config
# Via Serial: config set key=value
```

---

## 🛠️ Troubleshooting

### `./upload.sh` Fails with "mklittlefs not found"

**Solution 1:** Install via Arduino CLI
```bash
arduino-cli core install esp32:esp32
```

**Solution 2:** Install mklittlefs manually
```bash
# Linux
sudo apt-get install mklittlefs

# macOS
brew install mklittlefs
```

### Can't Find ESP32 Port

```bash
# List available ports
ls /dev/tty* | grep -E 'USB|ACM'    # Linux/macOS

# If port shows but device doesn't respond:
# - Hold BOOT button during upload
# - Check USB cable (try different cable/port)
```

### Web Pages Show 404 / Missing CSS/JS

**This means LittleFS didn't upload!**

The `./upload.sh` script handles this automatically. If it fails:

```bash
# The script includes a data build + upload step
# Just re-run: ./upload.sh
```

### WiFi Won't Connect (Station Mode)

- Check SSID/password in `.env` and Serial Monitor output
- Try AP mode instead: `STATION_MODE=false` in `.env`
- Check WiFi is 2.4GHz (not 5GHz)

### Sensors Not Updating

Check Serial Monitor:
```
[PHYSICS] Sensor physics simulation active
```

If missing, verify `ENABLE_PHYSICS = true` in `01_Config.ino`

---

## 📊 System Specs

| Component | Resource | Typical Usage |
|-----------|----------|---------------|
| Flash | 4MB+ | 1.2MB firmware + 1.2MB LittleFS |
| RAM | 320KB+ | ~200KB in use by default |
| PSRAM | Optional | Recommended for concurrent WebSockets |
| LittleFS | 1.2MB | Web assets + database |

---

## 🔒 Security Warnings

⚠️ **CRITICAL:** This is an **INTENTIONALLY VULNERABLE** educational SCADA simulator!

**DO NOT:**
- Connect to real production ICS/SCADA networks
- Use on the internet or untrusted networks
- Enable without network isolation (firewall/VLAN)

**DO:**
- Use **isolated lab network** (preferred: air-gap or VLAN)
- Document all findings
- Get written permission before testing
- Have rollback/recovery procedures

---

## 📚 Learning Projects

### Week 1-2: Reconnaissance
- Port scanning: `nmap -sV 192.168.4.1`
- API enumeration: `curl -v http://192.168.4.1/api/`
- Identify exploit entry points

### Week 3: IDOR Exploitation (Path 1)
- Map sensor IDs: `GET /api/sensors`
- Access cross-line data
- Export all readings from unauthorized lines

### Week 4: Command Injection (Path 2)
- Fuzz `/api/actuators/control` endpoint
- Test payload encoding
- Simulate malicious device control

### Week 5: Race Conditions (Path 3)
- Rapid-fire control requests
- Attempt state corruption
- Trigger safety bypass

### Week 6-7: Physics & Forensics
- Analyze sensor patterns for anomalies
- Extract debug info from `/vuln/debug`
- Reconstruct historical events

### Week 8: Defense Evasion
- Work around IP blocks
- Bypass rate limiting
- Adapt under constraints

---

## ✅ Verification Checklist

Before students use:

- [ ] `./upload.sh` completed without errors
- [ ] Serial Monitor shows "System Ready! 🚀"
- [ ] Can access `http://192.168.4.1/` in browser
- [ ] Dashboard loads with P&ID diagram
- [ ] Can login with `admin/admin`
- [ ] `/api/sensors` returns valid JSON
- [ ] `/api/actuators` returns valid JSON
- [ ] WebSocket real-time updates working
- [ ] Difficulty can be changed without recompile
- [ ] Defense system responds to Serial commands
- [ ] Network is **isolated** from production
- [ ] Lab scope and rules documented
- [ ] Students briefed before starting

---

## 📞 Support

- **Serial Monitor** (115200 baud): Check boot output and debug messages
- **README.md**: Full technical documentation
- **Source code** (`*.ino` files): Comment-heavy and well-documented
- **ESP32 Forums**: https://esp32.com

---

## 📝 Version

- **Version:** 2.0
- **Release:** February 2026
- **Platform:** ESP32 (Arduino Framework)
- **License:** Educational Use Only

**Key Features:**
- 6 independent exploit paths (IDOR, Injection, Race, Physics, Forensics, Weak Auth)
- 4 production lines with 20+ realistic sensors
- Physics-based sensor simulation
- Difficulty-based hint system (EASY/NORMAL/HARD)
- Defense system with IDS/IP blocking
- Competitive leaderboard scoring
- WebSocket real-time updates
- Incident generation and tracking

---

**Ready to start!** Run `./upload.sh` and begin your SCADA penetration testing training. 🚀

For full documentation, see [README.md](README.md)
