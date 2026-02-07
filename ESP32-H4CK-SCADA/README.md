# ESP32 Industrial SCADA Lab

**Version:** 2.0 | **Platform:** ESP32 | **Status:** Implementation in Progress

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Language](https://img.shields.io/badge/language-C%2B%2B-brightgreen.svg)](https://isocpp.org/)

---

## Overview

<img src="images/h4ck-SCADA-dashboard.png" alt="ESP32-LAB - SCADA Industrial" style="float:left; margin:0 1rem 1rem 0; width:300px;">

ESP32 Industrial SCADA Lab is a red team-oriented training environment for industrial control systems (ICS/SCADA), implemented for the ESP32 platform. The project provides a realistic simulation of a multi-line production facility and six independent exploit paths to support hands-on cybersecurity exercises.

**GitHub Repository**: https://github.com/arn-c0de/ESP32-LABs

### Key Features

- **Six independent exploit vectors**: IDOR, command injection, race conditions, physics analysis, forensics, weak authentication
- **Realistic industrial simulation**: four production lines and over 20 sensors with physics-based behavior
- **Web-based SCADA dashboard**: P&ID diagrams, real-time trends, and alarm management
- **Three difficulty levels**: EASY (guided), NORMAL (discovery), HARD (expert)
- **Dynamic defense system**: instructor-controlled IDS/WAF and IP blocking
- **Competitive leaderboard**: ranking by exploits found and time to solve
- **Fully configurable**: centralized configuration file for customization

---

## Quick Start

### Hardware

- ESP32 development board (ESP32-WROOM-32 or equivalent)
- USB cable for programming
- Minimum 4 MB flash for LittleFS

### Software Requirements

- Arduino IDE 1.8.19+ or PlatformIO
- ESP32 board support package
- Required libraries:
  - WiFi
  - WebServer
  - LittleFS
  - ArduinoJson

### Installation

```bash
# Clone the repository
git clone https://github.com/arn-c0de/ESP32-LABs.git
cd ESP32-LABs

# Configure WiFi credentials (edit 01_Config.ino)
# Set your SSID and password

# Build and upload
./build.sh
./upload.sh

# Upload filesystem data
# Arduino IDE: Tools > ESP32 Sketch Data Upload
# PlatformIO: pio run --target uploadfs
```

### First Connection

1. Connect ESP32 to power
2. Find the IP address in Serial Monitor (115200 baud)
3. Open web browser: `http://<ESP32_IP>/`
4. Default credentials: `admin:admin123`

---

## Learning Objectives

### Primary: Red Team / Exploit Learning

Students will discover and exploit vulnerabilities to learn practical ICS/SCADA attack techniques, log analysis, and forensic investigation. Exercises also cover sensor physics and cross-correlation.

### Secondary: Defense & Adaptation

Instructor-controlled defenses provide practical experience in detecting and responding to attacks, evaluating counter-measures, and adapting techniques under constraints.

### Supporting Objectives

- Simulate multi-line industrial environments
- Practice incident response and root cause analysis
- Understand role-based access control and safety interlocks
- Provide competitive learning through leaderboards

---

## Exploit Paths

### Path 1: IDOR (Insecure Direct Object Reference)
**Endpoint**: `GET /api/sensor/reading?sensor_id=SENSOR-L2-03`  
**Vulnerability**: No cross-line access control  
**Learning**: Authorization bypass, privilege escalation

### Path 2: Command Injection
**Endpoint**: `POST /api/actuators/control`  
**Vulnerability**: Unsanitized input in command parameters  
**Learning**: Input validation, shell injection techniques

### Path 3: Race Condition
**Endpoint**: `POST /api/test/race?actuator=MOTOR-L2-01&count=10`  
**Vulnerability**: Missing synchronization locks  
**Learning**: Concurrency exploits, state corruption

### Path 4: Physics-Based Analysis
**Vulnerability**: Sensor anomaly detection failure  
**Learning**: Cross-correlation analysis, physical system behavior

### Path 5: Forensics
**Vulnerability**: Sensitive data in logs  
**Learning**: Log analysis, pattern recognition, data reconstruction

### Path 6: Weak Authentication
**Vulnerability**: Hardcoded credentials in maintenance logs  
**Learning**: Credential discovery, social engineering vectors

---

## Difficulty Levels

### EASY Mode (Guided Learning)
- All vulnerabilities active
- Step-by-step hints embedded in responses
- Defense disabled
- Predictable incidents
- **Use Case**: Beginners, security awareness training

### NORMAL Mode (Balanced Discovery)
- Mixed vulnerability exposure
- Hints available via API (must request)
- Optional defense (teacher-controlled)
- 1-2 realistic incidents per session
- **Use Case**: CTF-style learning, intermediate students

### HARD Mode (Expert Challenge)
- Vulnerabilities require discovery
- Zero hints available
- Defense active by default
- 3+ cascading incidents
- Forensic analysis required
- **Use Case**: Advanced training, penetration testing courses

---

## Architecture

### Project Structure

```
ESP32-SCADA/
├── ESP32-SCADA.ino           # Main entry point
├── 01_Config.ino             # Master configuration
├── 02_WiFi.ino               # Network connectivity
├── 03_WebServer.ino          # HTTP routes
├── 04_Auth.ino               # Authentication & authorization
├── 05_Database.ino           # LittleFS storage
├── 06_Physics.ino            # Sensor physics simulation
├── 07_Sensors.ino            # Sensor management
├── 08_Actuators.ino          # Actuator control
├── 09_Alarms.ino             # Alarm system
├── 10_Safety.ino             # Safety interlocks
├── 11_Incidents.ino          # Incident generation
├── 12_Defense.ino            # IDS/WAF/IP blocking
├── 13_Vulnerabilities.ino    # Exploit endpoints
├── 14_Gameplay.ino           # Flag & scoring logic
├── 15_API_Hints.ino          # Progressive hint system
├── 16_Leaderboard.ino        # Statistics & rankings
├── 17_Utils.ino              # Helper functions
│
├── data/
│   ├── html/                 # Web interface
│   ├── css/                  # Stylesheets
│   ├── js/                   # Client-side scripts
│   └── db/                   # JSON database files
│
├── docs/
│   ├── README.md
│   ├── GAMEPLAY.md
│   ├── CONFIG_GUIDE.md
│   ├── API.md
│   └── WALKTHROUGHS.md
│
└── tests/
    ├── test_sensors.py
    ├── test_exploits.py
    ├── test_defense.py
    └── test_flag_flow.py
```

### Configuration System

All behavior controlled via `01_Config.ino`:

```cpp
// Difficulty
DifficultyLevel DIFFICULTY = NORMAL;

// Exploit Paths
bool ENABLE_EXPLOIT_IDOR = true;
bool ENABLE_EXPLOIT_INJECTION = true;
bool ENABLE_EXPLOIT_RACE = true;
bool ENABLE_EXPLOIT_PHYSICS = true;
bool ENABLE_EXPLOIT_FORENSICS = true;
bool ENABLE_EXPLOIT_WEAK_AUTH = true;

// Defense System
bool DEFENSE_ENABLED = (DIFFICULTY >= NORMAL);
bool IDS_ACTIVE = true;
bool WAF_ACTIVE = (DIFFICULTY >= HARD);

// Hints
int HINTS_LEVEL = 2;  // 0=off, 1=minimal, 2=detailed, 3=step-by-step
```

---

## Web Interface

### Dashboard Features

- **SCADA Mimic Panel**: Interactive P&ID with real-time equipment status
- **Sensor Trends**: Historical data with customizable timeframes
- **Alarm Management**: Live alarm panel with acknowledgment
- **Incident Tracking**: Event timeline and root cause analysis interface
- **Control Interface**: Motor speed, valve position controls
- **Flag Submission**: CTF-style progress tracking
- **Leaderboard**: Real-time rankings and statistics

### Technology Stack

- HTML5 Canvas for P&ID diagrams
- Vanilla JavaScript (no external dependencies)
- WebSocket + polling fallback for real-time updates
- Responsive design (desktop/tablet/mobile)
- Dark mode optimized for control rooms

---

## API

### Public Endpoints
```
GET  /api/sensors/list              # List all sensors
GET  /api/sensor/reading            # Sensor timeseries (IDOR vector)
GET  /api/actuators/list            # List actuators
POST /api/actuators/control         # Control actuators (Injection vector)
GET  /api/alarms/history            # Alarm history (IDOR vector)
GET  /api/incidents/list            # Active incidents
GET  /api/hints                     # Progressive hints
```

### Admin Endpoints
```
POST /api/admin/flag                # Submit flags
GET  /api/admin/config              # Read configuration
POST /api/admin/config              # Update configuration
GET  /api/admin/leaderboard         # Export statistics
POST /api/admin/incidents/create    # Spawn incident
```

### Defense Endpoints
```
GET  /api/defense/status            # Defense state
POST /api/defense/block-ip          # Manual IP block
GET  /api/defense/alerts            # IDS/WAF events
```

---

## Defense System

### Teacher-Controlled Scenario

Defense runs parallel to student exploitation via Serial commands:

```bash
# Show defense status
defense status

# Block student IP (2 minutes)
iptables -A INPUT -s 192.168.1.100 -j DROP --duration 120

# Enable rate limiting
tc qdisc add rate-limit --src 0.0.0.0/0 --duration 300

# Force session logout
session reset --ip 192.168.1.100 --reason "breach detected"

# Activate honeypot alerts
defense config set honeypot_alerts=on
```

### Learning Outcomes

- Students experience realistic counter-measures
- Practice evasion and obfuscation techniques
- Learn timing and patience under constraints
- Understand defense costs and trade-offs

---

## Scoring System

### Ranking Metric: `(exploits_found, time_to_solve)`

**Primary Score**: Number of unique exploit paths discovered  
**Secondary Score**: Time elapsed (lower = better)

### Formula

```
base_score = 100
+ (exploit_paths_found × 500)        # PRIMARY
+ max(0, 3000 - time_min × 5)        # SECONDARY (time penalty)
+ (sub_flags × 50)
+ (incidents_resolved × 25)
+ (defense_evasions × 100)           # Bonus for adaptation
- (hints_requested × 10)
= total_score
```

### Leaderboard

```
Rank | Username | Difficulty | Exploits | Time (min) | Score
-----|----------|------------|----------|------------|-------
1    | alice    | HARD       | 6/6      | 45         | 5250
2    | bob      | NORMAL     | 5/6      | 38         | 3850
3    | charlie  | EASY       | 6/6      | 22         | 4100
```

---

## Testing

### Verification Commands

```bash
# List sensors
curl -s http://<ESP_IP>/api/sensors/list | jq .

# Test IDOR exploit
curl -s 'http://<ESP_IP>/api/sensor/reading?sensor_id=SENSOR-L2-03&limit=50' | jq .

# Test command injection
curl -X POST http://<ESP_IP>/api/actuators/control \
  -H 'Content-Type: application/json' \
  -d '{"id":"MOTOR-L2-01","cmd":"set","params":{"speed":"80;simulate:cat /data/db/maintenance.json"}}'

# Get hints
curl -s 'http://<ESP_IP>/api/hints?endpoint=GET_/api/sensor/reading&level=1'

# View leaderboard
curl -s 'http://<ESP_IP>/api/admin/leaderboard?difficulty=NORMAL' | jq .
```

### Python Test Suite

```bash
python3 tests/test_sensors.py --check-drift --check-noise
python3 tests/test_exploits.py --difficulty NORMAL --target 192.168.1.100
python3 tests/test_defense.py --run-idor-attack --check-blocking
python3 tests/test_flag_flow.py --verify-all-paths
```

---

## Security and Safety

- **No actual shell execution**: All injection is simulated
- **Config-driven security**: Weak credentials only in EASY mode
- **HMAC-based flag storage**: No plaintext secrets on device
- **Gated vulnerabilities**: Explicit `VULN_*` flags required
- **Safe for educational use**: Designed for isolated lab networks

---

## Documentation

- **[GAMEPLAY.md](docs/GAMEPLAY.md)**: Detailed gameplay mechanics and strategies
- **[CONFIG_GUIDE.md](docs/CONFIG_GUIDE.md)**: Complete configuration reference
- **[API.md](docs/API.md)**: Full API documentation with examples
- **[WALKTHROUGHS.md](docs/WALKTHROUGHS.md)**: Step-by-step exploit guides
- **[HINTS.md](docs/HINTS.md)**: Complete hint database

---

## Roadmap

### Phase 1: Scaffolding
- [x] Module stubs (17 `.ino` files)
- [x] Master configuration system
- [x] Database layer (LittleFS)
- [x] WiFi and web server basics

### Phase 2: Data & Physics (In Progress)
- [ ] Physics engine (drift, noise, correlation)
- [ ] Sensor management
- [ ] Actuator control
- [ ] Seed JSON files

### Phase 3: Gameplay & Exploits
- [ ] Multi-path flag logic
- [ ] 6 exploit endpoints
- [ ] Progressive hint system
- [ ] Authentication & authorization

### Phase 4: Advanced Features
- [ ] Incident generation engine
- [ ] Alarm system
- [ ] Safety interlocks
- [ ] Defense system (IDS/WAF)

### Phase 5: Frontend
- [ ] SCADA dashboard
- [ ] P&ID mimic panel
- [ ] Incident interface
- [ ] Defense control panel
- [ ] Mobile responsive design

### Phase 6: Testing & Polish
- [ ] Unit tests
- [ ] Integration tests
- [ ] Documentation
- [ ] Performance optimization

---

## Educational Use Cases

### Cybersecurity Courses
- Industrial control system security
- Penetration testing labs
- Incident response training
- Vulnerability assessment

### Engineering Programs
- SCADA system design
- Process control
- Sensor physics
- Fault diagnosis

### Professional Training
- Red team exercises
- Blue team defense drills
- ICS/SCADA awareness
- Hands-on CTF events

---

## Contributing

Contributions are welcome! Please read our contributing guidelines before submitting pull requests.

### Areas for Contribution
- Additional exploit paths
- New incident scenarios
- Frontend improvements
- Documentation enhancements
- Test coverage
- Performance optimization

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- ESP32 community for excellent documentation
- SCADA/ICS security researchers for vulnerability research
- Educational institutions testing and providing feedback

---

## Contact and Support

- **GitHub Issues**: For bug reports and feature requests
- **Discussions**: For questions and community support
- **Documentation**: Check `docs/` folder for detailed guides

---

## Disclaimer

This project is designed **exclusively for educational purposes** in controlled laboratory environments. Users are responsible for ensuring compliance with all applicable laws and regulations. Do not deploy on production systems or networks without proper authorization.

---

**Status**: Implementation in Progress | **Last Updated**: February 2026