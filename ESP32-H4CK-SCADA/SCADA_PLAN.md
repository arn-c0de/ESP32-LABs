# Professional Industrial SCADA Lab – Complete Edition

**Version:** 2.0 | **Platform:** ESP32 | **Language:** English | **Status:** Full Implementation Ready

---

## TL;DR
**RED TEAM-FOCUSED** Industrial SCADA Lab for ESP32. Students learn by attacking: 4 production lines, 20+ sensors, **6 independent exploit paths** (IDOR, Injection, Race Condition, Physics-based, Forensics, Social Engineering). Realistic physics-based sensor data + dynamic incident generation for authenticity. 3 difficulty levels with progressive hint system. **Admin Defense System** runs parallel via Serial (teacher controls IDS/WAF/IP blocking as cyber scenario unfolds). Live SCADA Dashboard, leaderboard ranked by **`(exploits_found, time_to_solve)`**. Everything configurable via `01_Config.ino`.

---

## Project Structure

```
ESP32-SCADA/
├── ESP32-SCADA.ino           # Main entry
├── 01_Config.ino             # **MASTER: ALL configuration variables**
├── 02_WiFi.ino               # WiFi + AP mode
├── 03_WebServer.ino          # HTTP routes + defense hooks
├── 04_Auth.ino               # JWT/Sessions + role-based access
├── 05_Database.ino           # LittleFS + JSON ringbuffers
├── 06_Physics.ino            # Realistic sensor data generation
├── 07_Sensors.ino            # Sensor management + timeseries
├── 08_Actuators.ino          # Command execution + state machine
├── 09_Alarms.ino             # Threshold checks + escalation
├── 10_Safety.ino             # Interlocks + emergency protocols
├── 11_Incidents.ino          # Auto-incident generation + scenarios
├── 12_Defense.ino            # IDS + WAF + Rate limiting + Scoring
├── 13_Vulnerabilities.ino    # Gated exploit endpoints (6 paths)
├── 14_Gameplay.ino           # Multi-path flag logic + hint system
├── 15_API_Hints.ino          # Hint endpoint (difficulty-based)
├── 16_Leaderboard.ino        # Stats + scoring + JSON export
├── 17_Utils.ino              # Helpers (JSON, crypto, strings)
│
├── build.sh
├── upload.sh
├── partitions.csv
│
├── data/
│   ├── db/                    # LittleFS database (seed JSONs)
│   │   ├── equipment.json
│   │   ├── sensors.json
│   │   ├── actuators.json
│   │   ├── alarms.json
│   │   ├── maintenance.json
│   │   ├── incidents.json
│   │   └── logs.json          # flags, defense, stats, hints
│   │
│   ├── html/
│   │   ├── index.html         # Main dashboard
│   │   ├── dashboard.html     # SCADA Mimic Panel (P&ID + Trends)
│   │   ├── sensors.html       # Sensor graphs + anomalies
│   │   ├── actuators.html     # Control interface
│   │   ├── alarms.html        # Alarm history + escalation
│   │   ├── incidents.html     # Incident log + RCA prompts
│   │   ├── defense.html       # Blue Team: Defense status + controls
│   │   ├── flags.html         # Flag submission UI (multi-path hints)
│   │   ├── admin.html         # Admin panel (config + stats export)
│   │   ├── leaderboard.html   # Scores + replay
│   │   └── mobile.html        # Mobile-friendly operator view
│   │
│   ├── css/
│   │   ├── style.css          # Global + dark/light mode toggle
│   │   ├── scada.css          # SCADA-specific (gauges, trends)
│   │   └── mobile.css         # Responsive design
│   │
│   └── js/
│       ├── api.js             # Fetch wrapper + auto-retry
│       ├── dashboard.js       # Mimic panel rendering (D3/canvas)
│       ├── hints.js           # Progressive hint disclosure
│       ├── incidents.js       # Incident UI + RCA interface
│       ├── defense.js         # Defense controls + IDS alerts
│       ├── flags.js           # Flag submission validation
│       ├── leaderboard.js     # Score display
│       ├── auth-sync.js       # Session management
│       ├── mode.js            # Dark mode toggle
│       └── notifications.js   # Toast + sound alerts
│
├── docs/
│   ├── README.md              # Setup & quickstart
│   ├── GAMEPLAY.md            # Multiple paths, difficulty tiers
│   ├── CONFIG_GUIDE.md        # How to tune all variables
│   ├── HINTS.md               # Hint database (tiered)
│   ├── API.md                 # Full API reference
│   └── WALKTHROUGHS.md        # Difficulty-specific guides
│
└── tests/
    ├── test_sensors.py        # Validate physics models
    ├── test_exploits.py       # Verify vulnerabilities work
    ├── test_defense.py        # Defense effectiveness
    └── test_flag_flow.py      # Multi-path flag logic
```

---

## Configuration System (`01_Config.ino`) – Master Control Panel

All behavioral toggles centralized in **one file** for easy testing and iteration.

### A. Gameplay & Difficulty (Student-Facing)

```cpp
// ===== DIFFICULTY & GAMEPLAY =====
enum DifficultyLevel { EASY, NORMAL, HARD };
DifficultyLevel DIFFICULTY = NORMAL;  // Can be set via admin config

// Hint System (0=off, 1=minimal, 2=detailed, 3=step-by-step)
int HINTS_LEVEL = 2;
bool PROGRESSIVE_DISCLOSURE = true;  // Unlock hints as user explores
bool SHOW_HINTS_IN_RESPONSES = (DIFFICULTY == EASY);

// Exploit Path Enablement (all 6 independent)
bool ENABLE_EXPLOIT_IDOR = true;
bool ENABLE_EXPLOIT_INJECTION = true;
bool ENABLE_EXPLOIT_RACE = true;
bool ENABLE_EXPLOIT_PHYSICS = true;       // Hidden in sensor anomalies
bool ENABLE_EXPLOIT_FORENSICS = true;     // Reconstruct from logs
bool ENABLE_EXPLOIT_WEAK_AUTH = true;     // Weak creds in maintenance logs
int EXPLOITS_REQUIRED_TO_WIN = 3;         // How many out of 6 to "complete"

// Game Mode (exploration, not competitive)
enum GameMode { EXPLORATION, CTF };
GameMode GAME_MODE = EXPLORATION;
```

### B. Vulnerability Toggles (Red Team Learning)

```cpp
// ===== VULNERABILITY TOGGLES =====
// These enable specific exploitable flaws for student learning

bool VULN_IDOR_SENSORS = true;              // Path 1: Cross-line reading
bool VULN_IDOR_ALARMS = true;               // Path 1: Alarm history access
bool VULN_COMMAND_INJECT = true;            // Path 2: Shell-like injection
bool VULN_RACE_ACTUATORS = true;            // Path 3: Concurrent command race
bool VULN_WEAK_AUTH = (DIFFICULTY == EASY); // Path 6: Hardcoded weak creds
bool VULN_HARDCODED_SECRETS = (DIFFICULTY != HARD);  // Path 6: Creds in logs
bool VULN_INSECURE_DESERIALIZATION = true;  // Path 5: Corrupted JSON state
bool VULN_LOGIC_FLAWS = true;               // Path 4/5: Physics-based anomalies

// Defense Toggles (Teacher controls via Serial)
bool DEFENSE_ENABLED = (DIFFICULTY >= NORMAL);
bool IDS_ACTIVE = DEFENSE_ENABLED && (DIFFICULTY >= NORMAL);  // Logging + alerts
bool WAF_ACTIVE = DEFENSE_ENABLED && (DIFFICULTY >= HARD);     // Req. blocking
bool IP_BLOCKING_ENABLED = DEFENSE_ENABLED;
bool RATE_LIMIT_ACTIVE = DEFENSE_ENABLED;
```

### C. Sensor Physics & Realism

```cpp
// ===== SENSOR PHYSICS =====
float SENSOR_DRIFT_RATE = 0.01;          // Drift per reading (ppm)
float SENSOR_NOISE_AMPLITUDE = 2.5;      // ±noise std dev
float SENSOR_CALIBRATION_ERROR = 1.5;    // Systematic bias
bool ENABLE_SENSOR_FAULTS = true;        // Stuck values, spikes
int FAULT_PROBABILITY_PERCENT = 5;       // 5% chance per reading
bool ENABLE_CROSS_CORRELATION = true;    // Motor speed → vibration
float LATENCY_SIMULATION_MS = 50;        // Network jitter simulation
```

### D. Incidents & Scenarios

```cpp
// ===== INCIDENT GENERATION =====
bool AUTO_INCIDENTS_ENABLED = true;
int INCIDENT_SPAWN_INTERVAL_SEC = 300;   // Every 5 min
int NUM_CONCURRENT_INCIDENTS = 1 + (DIFFICULTY == HARD ? 2 : 0);

// Incident Types
bool INCIDENT_TYPE_STUCK_VALVE = true;
bool INCIDENT_TYPE_SENSOR_FAULT = true;
bool INCIDENT_TYPE_MOTOR_OVERLOAD = true;
bool INCIDENT_TYPE_TEMPERATURE_SPIKE = true;
bool INCIDENT_TYPE_PRESSURE_LOSS = true;
bool INCIDENT_TYPE_LOSS_OF_SIGNAL = true;
bool INCIDENT_TYPE_SAFETY_BYPASS_DETECTED = true;

// Cascade Complexity
int INCIDENT_CASCADE_DEPTH = (DIFFICULTY == HARD) ? 3 : 1;
```

### E. Defense & Blue Team

```cpp
// ===== DEFENSE SYSTEM =====
bool DEFENSE_ENABLED = (GAME_MODE == DEFENSE_CHALLENGE);
int DEFENSE_POINTS_INITIAL = 100;
int DEFENSE_ARMOR_POINTS = 50;
int DEFENSE_SHIELD_STRENGTH = 30;

bool IDS_ACTIVE = DEFENSE_ENABLED && (DIFFICULTY >= NORMAL);
bool WAF_ACTIVE = DEFENSE_ENABLED && (DIFFICULTY >= HARD);
bool RATE_LIMIT_ACTIVE = DEFENSE_ENABLED;
int RATE_LIMIT_PER_MINUTE = 60;

bool HONEYPOT_ENDPOINTS_ENABLED = DEFENSE_ENABLED;
bool DECEPTION_ALERTS_ENABLED = DEFENSE_ENABLED;
bool AUTO_LOCKDOWN_ON_BREACH = (DIFFICULTY == HARD);

// IP Blocking
bool IP_BLOCKING_ENABLED = DEFENSE_ENABLED;
int MAX_BLOCKED_IPS = 10;
int BLOCK_DURATION_SEC = 300;
```

### F. Leaderboard & Scoring

```cpp
// ===== LEADERBOARD & STATS =====
bool LEADERBOARD_ENABLED = true;
bool TRACK_TIME_TO_FLAG = true;
bool TRACK_EXPLOITS_USED = true;
bool TRACK_DEFENSE_EFFECTIVENESS = DEFENSE_ENABLED;

enum ScoringSystem { SIMPLE, WEIGHTED, TIERED };
ScoringSystem SCORING_MODE = WEIGHTED;

int BASE_SCORE = 1000;
int TIME_PENALTY_PER_MINUTE = 10;
int EXPLOIT_BONUS_PER_PATH = 100;
int DEFENSE_SAVE_BONUS = 200;
```

### G. Hints Database

```cpp
// ===== HINTS CONFIGURATION =====
bool HINTS_AVAILABLE = (HINTS_LEVEL > 0);
int HINTS_UNLOCK_AFTER_MINUTES = 5;
int HINTS_MAX_PER_ENDPOINT = 3;

bool SHOW_ENDPOINT_HINTS = (HINTS_LEVEL >= 1);
bool SHOW_STRATEGY_HINTS = (HINTS_LEVEL >= 2);
bool SHOW_TOOL_HINTS = (HINTS_LEVEL >= 2);
bool SHOW_WALKTHROUGH = (HINTS_LEVEL >= 3);
bool SHOW_FLAG_FORMAT = (HINTS_LEVEL >= 3 && DIFFICULTY == EASY);
```

### H. Secrets & Auth

```cpp
// ===== SECRETS & CREDENTIALS =====
const char* SECRET_KEY = "SCADA-SECRET-WEAK-FOR-TESTING";
const char* ADMIN_PASSWORD = "admin123";
const char* JWT_SECRET = "jwt-secret-key-weak";

// Hardcoded weak credentials (VULN_WEAK_AUTH or VULN_HARDCODED_SECRETS)
const char* WEAK_OPERATOR_CREDS = "operator:changeme";
const char* WEAK_MAINTENANCE_CREDS = "maintenance:m4int3n@nc3";

bool CREDENTIALS_IN_LOGS = VULN_HARDCODED_SECRETS;
bool CREDENTIALS_IN_SOURCE = VULN_HARDCODED_SECRETS;
```

### I. Storage & Performance

```cpp
// ===== STORAGE & MEMORY =====
int RINGBUFFER_SENSOR_MAX_ENTRIES = 1000;
int RINGBUFFER_ALARM_MAX_ENTRIES = 500;
int RINGBUFFER_LOG_MAX_ENTRIES = 2000;
int RINGBUFFER_INCIDENT_MAX_ENTRIES = 100;

bool SAVE_TO_LITTLEFS = true;
bool COMPRESSION_ENABLED = false;
int AUTO_SAVE_INTERVAL_SEC = 60;
int CLEANUP_INTERVAL_SEC = 3600;
```

### J. Debug & Logging

```cpp
// ===== DEBUG & LOGGING =====
bool SERIAL_DEBUG = true;
int SERIAL_BAUD = 115200;
bool LOG_API_REQUESTS = (DIFFICULTY <= NORMAL);
bool LOG_DEFENSE_ACTIONS = true;
bool EXPOSE_DEBUG_ENDPOINTS = (DIFFICULTY == EASY);
bool SHOW_ERROR_DETAILS = (DIFFICULTY == EASY);
```

---

## 🎯 How Students Learn (Gameplay Flow)

### Red Team (Student Perspective)

1. **Explore** the SCADA system
   - List endpoints, sensors, actuators
   - Observe normal behavior (baseline)

2. **Hypothesize** vulnerabilities
   - "Can I read sensors from other lines?"
   - "Can I inject commands into the motor control?"
   - "Is there a race condition in state updates?"

3. **Exploit** (test your hypothesis)
   - Craft payloads
   - Analyze responses
   - Adapt based on feedback (hints, errors, defense)

4. **Collect sub-flags** from each exploit path
   - Embedded in responses, logs, or state corruption
   - 4+ out of 6 paths = complete coverage

5. **Compete** on leaderboard
   - Ranked by: `(exploit_paths_found, time_to_solve)`
   - Higher difficulty = more bonus points

### Blue Team (Teacher Perspective)

1. **Observe** student progress
   - via web dashboard or Serial terminal

2. **Deploy defenses** (Serial commands) when desired
   - Block IPs, apply rate limits, reset sessions
   - Teaches students **real-world counter-measures**

3. **Escalate or relax** defenses
   - Challenge advanced students
   - Support struggling students

4. **Discuss & debrief** with whole class
   - "What exploits were easiest?"
   - "How did defense change your tactics?"
   - "What would you do differently?"

### Difficulty as Scaffolding

| Difficulty | Student Needs | Teacher Role |
|-----------|---------------|--------------|
| **EASY** | Hints + step-by-steps | Provide walkthrough, minimal interference |
| **NORMAL** | Discovery with safety net | Enable defense mid-session; observe adaptation |
| **HARD** | True exploration | Full defense from start; let students struggle (productively) |

---

### 🔴 **PRIMARY: Red Team / Exploit Learning**
- Students identify and exploit **6 independent vulnerability paths** (IDOR, Injection, Race, Physics, Forensics, Social Eng.)
- Learn real-world attack techniques on industrial systems
- Practice diagnosis via logs, sensor analysis, and state inspection
- Discover objectives through exploration, not handed tutorials

### 🔵 **SECONDARY: Admin/Defense (Teacher-Controlled)**
- Instructor runs **Cyber Incident Scenario** via Serial commands
- Deploy defenses (IDS, WAF, IP blocks) to simulate realistic counter-measures
- Students must adapt exploits under dynamic defense conditions
- Teaches **threat adaptation** + **defense evasion techniques**

### 📊 **Supporting Objectives**
- Simulate realistic multi-line industrial environment with physics-based sensor data
- Persist data in LittleFS (rangbuffers, JSON schemas)
- Role-based access control (`admin`, `operator`, `maintenance`, `viewer`)
- 3 difficulty levels: EASY (hints+walkthroughs), NORMAL (discovery), HARD (zero hints)
- Incident-driven learning: cascading failures reveal root causes
- Leaderboard: rank by exploits found + time-to-solve

---

## Data Model & Database Schema (LittleFS)

### `logs.json` – Core Game State

```json
{
  "players": [
    {
      "session_id": "sess-xxx",
      "username": "alice",
      "role": "operator",
      "difficulty": "NORMAL",
      "game_mode": "EXPLORATION",
      "started_at": "2026-02-05T10:00:00Z",
      "flag_paths_collected": ["IDOR", "INJECTION"],
      "flags_submitted": [
        {"path": "IDOR", "timestamp": "...", "correct": true},
        {"path": "INJECTION", "timestamp": "...", "correct": true}
      ],
      "exploits_used": ["GET /api/sensor/reading?sensor_id=...", "POST /api/actuators/control"],
      "incidents_encountered": ["STUCK_VALVE_L1", "TEMP_SPIKE_L2"],
      "incidents_resolved": ["STUCK_VALVE_L1"],
      "defense_breaches": 2,
      "scores": {
        "time_score": 850,
        "exploit_score": 400,
        "defense_score": 0,
        "total": 1250
      }
    }
  ],
  "game_state": {
    "current_incidents": [
      {
        "id": "INC-001",
        "type": "STUCK_VALVE",
        "line": 2,
        "severity": "HIGH",
        "created_at": "...",
        "resolved": false
      }
    ],
    "defense_events": [
      {"timestamp": "...", "event": "IDOR_ATTEMPT", "ip": "192.168.1.100", "action": "LOGGED"}
    ],
    "root_flag_hmac": "0x5f2a3c...",
    "root_flag_unlocked": false
  },
  "hints_database": {
    "GET_/api/sensor/reading": {
      "endpoint_name": "Sensor Timeseries",
      "difficulty": "EASY",
      "levels": [
        {"order": 1, "unlock_after_min": 0, "text": "Try reading sensors from different lines"},
        {"order": 2, "unlock_after_min": 5, "text": "Does it check your permissions?"},
        {"order": 3, "unlock_after_min": 15, "text": "IDOR = Insecure Direct Object Reference"}
      ]
    }
  }
}
```

### `sensors.json`, `actuators.json`, `alarms.json`, `maintenance.json`

Ring-buffered storage with auto-rotation per configured max entries.

### `incidents.json` – Scenario Templates & History

```json
{
  "templates": [
    {
      "type": "STUCK_VALVE",
      "description": "Pneumatic valve stuck CLOSED",
      "affected_equipment": ["VALVE-L2-02"],
      "cascade_effects": ["TEMP_SPIKE"],
      "resolution_hint": "Physical restart required",
      "sub_flag": "FLAG{stuck_valve_L2_...}"
    },
    {
      "type": "SENSOR_FAULT",
      "description": "Temperature sensor reads constant 99.5°C",
      "diagnosis_hint": "Compare with vibration sensor (should correlate)",
      "sub_flag": "FLAG{sensor_fault_L1_...}"
    },
    {
      "type": "RACE_CONDITION",
      "description": "10+ concurrent motor commands",
      "sub_flag": "Hidden in corrupted state field"
    }
  ],
  "history": []
}
```

---

## Exploit Paths: Core Learning Objectives (6 Independent Routes)

### **Path 1: IDOR (Insecure Direct Object Reference)**
- **Endpoint**: `GET /api/sensor/reading?sensor_id=SENSOR-L2-03&limit=50`
- **Flaw**: No cross-line access check when `VULN_IDOR_SENSORS=true`
- **Sub-flag**: Hidden in restricted sensor readings (embedded in JSON or fake alarm)
- **Hint Progression**:
  - L1 (0 min): "Sensors have IDs like SENSOR-L1-01... try L2-03"
  - L2 (5 min): "IDOR = reading data you shouldn't access"
  - L3 (15 min): "Check permissions on cross-line reads"

### **Path 2: Command Injection**
- **Endpoint**: `POST /api/actuators/control`
- **Flaw**: `params.speed` piped to simulated shell when `VULN_COMMAND_INJECT=true`
- **Sub-flag**: In simulated command output (e.g., `simulate:cat /data/db/maintenance.json`)
- **Example Payload**:
  ```json
  {
    "id": "MOTOR-L1-01",
    "cmd": "set",
    "params": {
      "speed": "50;simulate:cat /data/db/equipment.json"
    }
  }
  ```

### **Path 3: Race Condition**
- **Endpoint**: `POST /api/test/race?actuator=MOTOR-L2-01&count=10`
- **Flaw**: Rapid concurrent commands without locks when `VULN_RACE_ACTUATORS=true`
- **Sub-flag**: In corrupted state field (e.g., `"status":"RUNNING;FLAG{race_...}"`)

### **Path 4: Physics-Based (Sensor Anomaly Analysis)**
- **Flaw**: Motor running but vibration sensor unchanged (fault detection)
- **Diagnosis**: Cross-correlate sensors; identify missing dependencies
- **Sub-flag**: In maintenance log noting "sensor fault" or "calibration needed"
- **Difficulty**: Requires understanding sensor physics

### **Path 5: Forensics (Log Analysis)**
- **Flaw**: Reconstruct root flag from multiple sub-flags + timestamps
- **Requirements**: Read logs, analyze patterns, combine clues
- **Sub-flag Sources**: Spread across incident logs, maintenance records, security events
- **Difficulty**: HARD mode only; requires deep exploration

### **Path 6: Social Engineering / Weak Auth**
- **Flaw**: Maintenance credentials visible in logs: `maintenance:m4int3n@nc3`
- **Exploitation**: Use weak creds to access restricted endpoints
- **Sub-flag**: In operator's private data or restricted sensor readings
- **Guard**: `VULN_WEAK_AUTH` + `VULN_HARDCODED_SECRETS` flags

---

## API Endpoints

| Endpoint | Method | Purpose | Auth | Gated By |
|----------|--------|---------|------|----------|
| `/api/sensors/list` | GET | List all sensors | Role | `PROTECT_SENSOR_ACCESS` |
| `/api/sensor/reading?id=X&limit=N` | GET | Sensor timeseries | Role | **IDOR vector** |
| `/api/actuators/list` | GET | List actuators | Role | `PROTECT_ACTUATOR_LOCKS` |
| `/api/actuators/control` | POST | Send command | Role + JWT | **INJECTION vector** |
| `/api/alarms/history?line=X&limit=N` | GET | Alarm history | Role | **IDOR vector** |
| `/api/incidents/list` | GET | Active incidents | Public | Educational |
| `/api/incidents/report?id=X` | POST | Submit RCA | Role | Incident gameplay |
| `/api/hints?endpoint=X&level=Y` | GET | Progressive hints | Public | `HINTS_LEVEL` |
| **Lab-Only** (gated by `LAB_MODE`) | | | | |
| `POST /vuln/sensor-tamper` | POST | Inject fake reading | Admin | `VULN_IDOR_SENSORS` |
| `POST /api/test/race` | POST | Trigger race | Admin | `VULN_RACE_ACTUATORS` |
| **Admin** | | | | |
| `POST /api/admin/flag` | POST | Submit root flag (HMAC) | Admin | `PROTECT_ADMIN_ENDPOINTS` |
| `GET/POST /api/admin/config` | GET/POST | Read/update config | Admin | HARD mode auth |
| `GET /api/admin/leaderboard` | GET | Export scores | Public | `LEADERBOARD_ENABLED` |
| `POST /api/admin/incidents/create` | POST | Spawn incident | Admin | Incident system |
| **Defense** (Blue Team) | | | | |
| `GET /api/defense/status` | GET | DP/AP/SS + blocked IPs | Public | `DEFENSE_ENABLED` |
| `POST /api/defense/block-ip` | POST | Manual IP block | Admin | `IP_BLOCKING_ENABLED` |
| `GET /api/defense/alerts` | GET | IDS/WAF events | Public | `IDS_ACTIVE` |

---

## Difficulty Scaling

### **EASY Mode** (Guided Exploit Learning)
- All vulnerabilities **active** (no randomness)
- Hints **embedded in responses** + step-by-step walkthroughs
- Defense **disabled** (for cleaner learning)
- No random incidents (predictable for focused learning)
- Speed: faster exploitation for confidence-building
- **Use Case**: Beginners, students new to security

### **NORMAL Mode** (Balanced Discovery)
- Some vulnerabilities **require exploration** to discover
- Hints available via `/api/hints` endpoint (must be requested)
- Defense **optional** (teacher can enable via Serial)
- 1-2 incidents per session (realistic disruption)
- Mixed vulnerability exposure (some hinted, some hidden)
- **Use Case**: CTF-style learning, intermediate students

### **HARD Mode** (Expert Exploration)**
- Vulnerabilities **require discovery** (no hints embedded)
- **Zero hints** available
- Defense **active by default** (teacher controls intensity via Serial)
- 3+ **cascading incidents** simultaneously
- Forensic analysis required (deep log inspection)
- Requires cross-correlation of multiple signals
- **Use Case**: Seasoned pentesters, advanced scenarios

---

## Parallelized Defense System (Admin Tool)

### 🛡️ **Not a Gameplay Mechanic** — Teacher-Controlled Cyber Scenario

Defense runs **parallel to student exploitation**, controlled by instructor via **Serial commands**:

```bash
# Instructor perspective (Serial terminal):
defense status
# → Shows current integrity, active rules, resource costs

iptables -A INPUT -s <student_ip> -j DROP --duration 120
# → Block that student's IP for 2 min (simulates defensive response)

tc qdisc add rate-limit --src 0.0.0.0/0 --duration 300
# → Global rate limiting (simulates WAF under attack)

session reset --ip <student_ip> --reason "suspicious activity"
# → Force logout (simulates incident response team action)
```

### Learning Objectives for Defense

- Student observes **real-world defensive counter-measures** in real-time
- Must **adapt exploits** under dynamic defense (evasion techniques)
- Learns **incident response timing** (how fast can defense react?)
- Practices **obfuscation** and **timing attacks**
- Understands **cost/benefit** of aggressive defense (stability vs. throughput)

### Configuration

Defense behavior tied to difficulty:
```cpp
bool DEFENSE_ENABLED = (DIFFICULTY >= NORMAL);  // EASY: disabled, NORMAL+: via Serial
int DEFENSE_AGGRESSIVENESS = (DIFFICULTY == HARD) ? 3 : 1;  // How fast to react?
```

---

## Incident Simulation Engine (Learning Tool)

### Purpose
Incidents are **teachable moments**, not gameplay objectives. They demonstrate:
- Real-world cascading failures in industrial systems
- How sensor anomalies propagate
- The value of **root cause analysis** (RCA)
- Impact of exploits on system stability

### Incident Types
- **STUCK_VALVE**: Pressure drops, flow stops, temp rises
- **SENSOR_FAULT**: Stuck value + no physical correlation
- **MOTOR_OVERLOAD**: Current spike, vibration high
- **TEMPERATURE_SPIKE**: Coolant/ambient anomaly
- **PRESSURE_LOSS**: Leak detection
- **LOSS_OF_SIGNAL**: Network/sensor disconnection
- **SAFETY_BYPASS_DETECTED**: Interlocks disabled (from exploit)
- **RACE_CONDITION_LOG**: State corruption detected

### Lifecycle
1. **Auto-Spawn** (every `INCIDENT_SPAWN_INTERVAL_SEC`, configurable)
2. **Escalation** (severity: LOW → MEDIUM → CRITICAL over time)
3. **Cascade** (sub-incidents spawn if `INCIDENT_CASCADE_DEPTH > 1`)
4. **Student Observation** (students analyze logs + sensor data)
5. **Teacher Intervention** (instructor can resolve via Serial if desired)

### Learning Application
- Students **diagnose root causes** from sensor data + logs
- Understand **system interdependencies** (motor → vibration, temp, motor speed)
- Practice **forensic analysis** of corrupted state
- Observe impact of their own exploits (e.g., if they exploit actuators, incidents may spike)

### Configuration
```cpp
bool AUTO_INCIDENTS_ENABLED = true;
int INCIDENT_SPAWN_INTERVAL_SEC = 300;  // Every 5 min (configurable)
int NUM_CONCURRENT_INCIDENTS = 1 + (DIFFICULTY == HARD ? 2 : 0);
int INCIDENT_CASCADE_DEPTH = (DIFFICULTY == HARD) ? 3 : 1;
```

---

## Defense System (Teacher-Controlled Cyber Scenario)

**Not a student gameplay mechanic.** Instead, the instructor deploys defenses via **Serial terminal** to simulate realistic Counter-Measures, forcing students to adapt and practice **evasion techniques**.

### Instructor Commands (Serial)

```bash
# Show current defense state
defense status
# → Resources, active rules, integrity

# Block student IP (2-minute timeout)
iptables -A INPUT -s 192.168.1.105 -j DROP --duration 120
# → Student sees 403 Forbidden on next request; must pivot/obfuscate

# Rate-limit all traffic (realistic WAF)
tc qdisc add rate-limit --src 0.0.0.0/0 --duration 300 rate=10-per-min
# → All students slow down; teaches timing + patience

# Force logout (session termination)
session reset --ip 192.168.1.105 --reason "breach detected"
# → Student must re-authenticate; learns about session management

# Activate honeypot alerts
defense config set honeypot_alerts=on
# → Decoys reveal when students poke wrong endpoints
```

### Student Adaptation (Learning)

When defense is active, students learn:
1. **Evasion**: Use proxies, timing delays, IP rotation
2. **Stealth**: Minimize logging (craft payloads to avoid WAF triggers)
3. **Timing**: Adapt to rate limits without getting blocked
4. **Persistence**: Continue exploiting after setbacks
5. **Forensic Awareness**: Defense actions appear in logs (analyze counter-measures)

### Simplified Resource Model

| Resource | Default | Regeneration | Use |
|----------|---------|--------------|-----|
| DP (Defense Points) | 100 | +2/5sec | Cost to deploy rules |
| AP (Action Points) | 10 | +1/5sec | Simultaneous actions |
| SS (Stability) | 100 | +1/5sec | System health (aggr. defense reduces it) |

**Why?** Teaches that defending is **costly** — aggressive defense may cripple legitimate services or cause DoS to all users.

### Teacher Discretion

- **Scenario A** (EASY): Defense disabled → pure exploitation
- **Scenario B** (NORMAL): Defense enabled after 10 min → adaptation challenge
- **Scenario C** (HARD): Defense always active + escalates → advanced evasion required
- **Scenario D** (CTF Mode): Instructor provides **defense timeline** in advance (students plan accordingly)

---

## Leaderboard & Stats

### 🏆 **Ranking Metric: `(exploits_found, time_to_solve)`**

**Primary Score** = number of **unique exploit paths discovered**  
**Secondary Score** = **time elapsed** to find them (lower = better)

### Per-Player Metrics
```json
{
  "session_id": "sess-xxx",
  "username": "alice",
  "difficulty": "NORMAL",
  "stats": {
    "time_elapsed_sec": 2730,
    "exploit_paths_found": 5,
    "sub_flags_collected": 12,
    "incidents_resolved": 3,
    "hints_requested": 4,
    "defense_evasions": 2,
    "total_score": 5250
  }
}
```

### Scoring Formula (Exploit-First)
```
base_score = 100
+ (exploit_paths_found × 500)           ← PRIMARY
+ max(0, 3000 - time_min × 5)           ← SECONDARY (time penalty)
+ (sub_flags × 50)
+ (incidents_resolved × 25)
+ (defense_evasions × 100)              ← Bonus for adapting under defense
- (hints_requested × 10)
= total_score
```

### Leaderboard View
```
Rank | Username | Difficulty | Exploits | Time (min) | Score
-----|----------|-------------|----------|-----------|--------
1    | alice    | HARD        | 6/6      | 45        | 5250
2    | bob      | NORMAL      | 5/6      | 38        | 3850
3    | charlie  | EASY        | 6/6      | 22        | 4100
```

### Optional: Defense Survival Bonus
If teacher enables defense during session:
```
+ (minutes_while_defense_active × 50) if student still exploiting
```
This rewards **adaptation** and **persistence under counter-measures**.

---

## Implementation Roadmap

### **Phase 1: Scaffolding** (Week 1)
- [x] Create `.ino` file stubs (17 modules)
- [x] Implement `01_Config.ino` (master config)
- [x] Implement `05_Database.ino` (LittleFS + ringbuffer)
- [x] Implement `02_WiFi.ino` + `03_WebServer.ino` (basic routes)

### **Phase 2: Data & Physics** (Week 2)
- [ ] Implement `06_Physics.ino` (drift, noise, faults, correlation)
- [ ] Implement `07_Sensors.ino` + seed `sensors.json`
- [ ] Implement `08_Actuators.ino` + seed `actuators.json`
- [ ] Create seed JSON files for all entities

### **Phase 3: Gameplay & Exploits** (Week 3)
- [ ] Implement `14_Gameplay.ino` (multi-path flag logic)
- [ ] Implement `13_Vulnerabilities.ino` (6 exploit paths)
- [ ] Implement `15_API_Hints.ino` (progressive disclosure)
- [ ] Implement `04_Auth.ino` (JWT + roles)

### **Phase 4: Advanced Features** (Week 4)
- [ ] Implement `11_Incidents.ino` (cascade scenarios)
- [ ] Implement `09_Alarms.ino` + `10_Safety.ino`
- [ ] Implement `12_Defense.ino` (IDS + WAF + IP block)
- [ ] Implement `16_Leaderboard.ino` (scoring)

### **Phase 5: Frontend** (Week 5)
- [ ] Dashboard + P&ID mimic panel (HTML + D3/canvas)
- [ ] Incident UI + RCA interface
- [ ] Defense control panel
- [ ] Leaderboard + admin panel
- [ ] Mobile view + dark mode

### **Phase 6: Testing & Documentation** (Week 6)
- [ ] Unit tests (Python: exploits, physics, flags)
- [ ] Integration tests (full game flows)
- [ ] Documentation (CONFIG_GUIDE, WALKTHROUGHS, API)
- [ ] Performance tuning + memory optimization

---

## Modern Web GUI Design 🎨

### Design System

**Professional Industrial SCADA Aesthetic**
- Clean, high-contrast interface for operator use
- P&ID (Piping & Instrumentation Diagram) style visualizations
- Real-time updates without page refresh
- Mobile-responsive for field operations
- Dark mode optimized for 24/7 control rooms

### Technology Stack

**Zero External Dependencies (ESP32-hosted):**
- HTML5 Canvas for P&ID diagrams
- CSS Grid + Flexbox for layouts
- Vanilla JavaScript (ES6+)
- WebSocket for real-time data
- LocalStorage for user preferences

**Optional Enhancements (when online):**
- Chart.js for trend graphs
- D3.js for advanced visualizations
- Feather Icons

### Color System

```css
:root {
  /* Dark Mode (Primary) */
  --scada-bg-dark: #0a0e14;
  --scada-panel-dark: #141921;
  --scada-surface-dark: #1e242e;
  
  /* Industrial Colors */
  --scada-red: #ff4757;      /* Alarms, emergency stop */
  --scada-green: #26de81;    /* Normal operation */
  --scada-yellow: #fed330;   /* Warnings */
  --scada-blue: #45aaf2;     /* Running motors */
  --scada-purple: #a55eea;   /* Trending data */
  --scada-orange: #fd9644;   /* Maintenance */
  
  /* Status Indicators */
  --status-running: var(--scada-green);
  --status-stopped: #636e72;
  --status-warning: var(--scada-yellow);
  --status-alarm: var(--scada-red);
  --status-maintenance: var(--scada-orange);
  
  /* Sensor Values */
  --sensor-normal: var(--scada-green);
  --sensor-high: var(--scada-yellow);
  --sensor-critical: var(--scada-red);
}
```

### Component Library

#### 1. SCADA Mimic Panel

**Canvas-Based P&ID:**
```html
<div class="scada-mimic">
  <canvas id="pid-canvas" width="1200" height="800"></canvas>
  <div class="mimic-controls">
    <button class="zoom-in">+</button>
    <button class="zoom-out">−</button>
    <button class="zoom-reset">⊙</button>
    <select class="line-selector">
      <option value="all">All Lines</option>
      <option value="1">Line 1</option>
      <option value="2">Line 2</option>
      <option value="3">Line 3</option>
      <option value="4">Line 4</option>
    </select>
  </div>
</div>
```

**Interactive Elements:**
- Motors: animated rotation when running
- Valves: color-coded (open=green, closed=gray, stuck=red)
- Sensors: live value overlays
- Pipes: flow direction arrows
- Click elements for details modal

#### 2. Process Overview Cards

```html
<div class="process-card" data-line="1" data-status="running">
  <div class="card-header">
    <h3>Production Line 1</h3>
    <span class="status-badge badge-green">Running</span>
  </div>
  <div class="card-metrics">
    <div class="metric">
      <span class="metric-label">Output</span>
      <span class="metric-value">87<span class="unit">%</span></span>
    </div>
    <div class="metric">
      <span class="metric-label">Efficiency</span>
      <span class="metric-value">92<span class="unit">%</span></span>
    </div>
    <div class="metric">
      <span class="metric-label">Uptime</span>
      <span class="metric-value">6h 42m</span>
    </div>
  </div>
  <div class="card-sensors">
    <div class="sensor-mini" data-status="normal">
      <span class="sensor-icon">🌡️</span>
      <span class="sensor-value">72.3°C</span>
    </div>
    <div class="sensor-mini" data-status="normal">
      <span class="sensor-icon">⚡</span>
      <span class="sensor-value">12.5kW</span>
    </div>
    <div class="sensor-mini" data-status="warning">
      <span class="sensor-icon">〰️</span>
      <span class="sensor-value">0.82mm/s</span>
    </div>
  </div>
  <div class="card-actions">
    <button class="btn-secondary">View Details</button>
    <button class="btn-primary">Control</button>
  </div>
</div>
```

#### 3. Sensor Trend Graph

```html
<div class="trend-chart">
  <div class="chart-header">
    <h4>Temperature - Line 1</h4>
    <div class="chart-controls">
      <button class="timerange active" data-range="1h">1H</button>
      <button class="timerange" data-range="4h">4H</button>
      <button class="timerange" data-range="24h">24H</button>
      <button class="timerange" data-range="7d">7D</button>
    </div>
  </div>
  <canvas id="temp-trend" width="800" height="300"></canvas>
  <div class="chart-legend">
    <div class="legend-item">
      <span class="legend-color" style="background: var(--scada-blue);"></span>
      <span>Current: 72.3°C</span>
    </div>
    <div class="legend-item">
      <span class="legend-color" style="background: var(--scada-yellow);"></span>
      <span>High Threshold: 80°C</span>
    </div>
    <div class="legend-item">
      <span class="legend-color" style="background: var(--scada-red);"></span>
      <span>Critical: 90°C</span>
    </div>
  </div>
</div>
```

#### 4. Alarm Panel

```html
<div class="alarm-panel">
  <div class="alarm-header">
    <h3>Active Alarms <span class="alarm-count">3</span></h3>
    <button class="acknowledge-all">Acknowledge All</button>
  </div>
  <div class="alarm-list">
    <div class="alarm-item alarm-critical" data-id="ALM-001">
      <div class="alarm-indicator"></div>
      <div class="alarm-content">
        <div class="alarm-title">Temperature High</div>
        <div class="alarm-details">
          Line 2 / SENSOR-L2-01 / 91.2°C (> 90°C)
        </div>
        <div class="alarm-meta">
          <span class="alarm-time">2 min ago</span>
          <span class="alarm-line">Line 2</span>
        </div>
      </div>
      <button class="alarm-ack">Ack</button>
    </div>
    <div class="alarm-item alarm-warning" data-id="ALM-002">
      <div class="alarm-indicator"></div>
      <div class="alarm-content">
        <div class="alarm-title">Vibration Elevated</div>
        <div class="alarm-details">
          Line 1 / MOTOR-L1-01 / 0.92mm/s (> 0.8mm/s)
        </div>
        <div class="alarm-meta">
          <span class="alarm-time">15 min ago</span>
          <span class="alarm-line">Line 1</span>
        </div>
      </div>
      <button class="alarm-ack">Ack</button>
    </div>
  </div>
</div>
```

#### 5. Control Interface

```html
<div class="control-panel">
  <div class="panel-header">
    <h3>Actuator Control</h3>
    <span class="role-badge">Operator</span>
  </div>
  
  <!-- Motor Control -->
  <div class="control-group">
    <label>Motor Speed (MOTOR-L1-01)</label>
    <div class="slider-control">
      <input type="range" min="0" max="100" value="75" 
             class="speed-slider" id="motor-speed">
      <span class="slider-value">75%</span>
    </div>
    <div class="control-buttons">
      <button class="btn-success" data-action="start">Start</button>
      <button class="btn-danger" data-action="stop">Stop</button>
      <button class="btn-warning" data-action="emergency">E-Stop</button>
    </div>
  </div>
  
  <!-- Valve Control -->
  <div class="control-group">
    <label>Valve Position (VALVE-L1-02)</label>
    <div class="toggle-control">
      <button class="toggle-btn active" data-state="open">Open</button>
      <button class="toggle-btn" data-state="closed">Closed</button>
    </div>
    <div class="control-status">
      Status: <span class="status-text status-green">OPEN</span>
    </div>
  </div>
</div>
```

#### 6. Incident Report Modal

```html
<div class="modal" id="incident-modal">
  <div class="modal-overlay"></div>
  <div class="modal-content">
    <div class="modal-header">
      <h3>Incident Report: INC-001</h3>
      <button class="modal-close">×</button>
    </div>
    <div class="modal-body">
      <div class="incident-details">
        <div class="detail-row">
          <span class="detail-label">Type:</span>
          <span class="detail-value">Stuck Valve</span>
        </div>
        <div class="detail-row">
          <span class="detail-label">Line:</span>
          <span class="detail-value">Line 2</span>
        </div>
        <div class="detail-row">
          <span class="detail-label">Equipment:</span>
          <span class="detail-value">VALVE-L2-02</span>
        </div>
        <div class="detail-row">
          <span class="detail-label">Severity:</span>
          <span class="badge badge-red">HIGH</span>
        </div>
        <div class="detail-row">
          <span class="detail-label">Started:</span>
          <span class="detail-value">2026-02-05 14:23:17</span>
        </div>
      </div>
      
      <div class="incident-timeline">
        <h4>Event Timeline</h4>
        <div class="timeline-item">
          <span class="timeline-time">14:23:17</span>
          <span class="timeline-event">Valve failed to respond to CLOSE command</span>
        </div>
        <div class="timeline-item">
          <span class="timeline-time">14:23:45</span>
          <span class="timeline-event">Pressure increased to 8.2 bar (> 8.0 threshold)</span>
        </div>
        <div class="timeline-item">
          <span class="timeline-time">14:24:12</span>
          <span class="timeline-event">Temperature spike detected: 87.5°C</span>
        </div>
      </div>
      
      <div class="rca-form">
        <h4>Root Cause Analysis</h4>
        <textarea placeholder="Enter your diagnosis..." rows="4"></textarea>
        <button class="btn-primary">Submit RCA</button>
      </div>
    </div>
  </div>
</div>
```

### Page Layouts

#### Dashboard (`dashboard.html`)

**Desktop Layout (1920x1080):**
```
+----------------------------------------------------------+
| ESP32-SCADA           [Search] [Alerts:3] [User] [Mode] |
+----------------------------------------------------------+
|                                                           |
|  +-------------------+  +-------------------+            |
|  | Line 1: Running   |  | Line 2: Alarm     |            |
|  | Output: 87%       |  | Output: 45%       |            |
|  | 🌡️72.3°C ⚡12.5kW |  | 🌡️91.2°C ⚡8.2kW  |            |
|  +-------------------+  +-------------------+            |
|                                                           |
|  +-------------------+  +-------------------+            |
|  | Line 3: Stopped   |  | Line 4: Running   |            |
|  | Output: 0%        |  | Output: 93%       |            |
|  +-------------------+  +-------------------+            |
|                                                           |
|  +---------------------------------------------------+   |
|  | P&ID Mimic Panel - All Lines                      |   |
|  |                                                   |   |
|  |  [Interactive canvas with motors, valves, pipes] |   |
|  |                                                   |   |
|  +---------------------------------------------------+   |
|                                                           |
|  +------------------------+  +--------------------------+ |
|  | Active Alarms (3)      |  | Recent Incidents (5)    | |
|  | • Temp High (L2)       |  | • Stuck Valve (L2)      | |
|  | • Vibration (L1)       |  | • Sensor Fault (L3)     | |
|  | • Pressure Low (L4)    |  +-------------------------+ |
|  +------------------------+                               |
+----------------------------------------------------------+
```

#### Sensor Details (`sensors.html`)

```
+----------------------------------------------------------+
| Sensor: SENSOR-L1-03 (Temperature)        [Back to List]|
+----------------------------------------------------------+
|                                                           |
|  +---------------------------------------------------+   |
|  | Current Reading: 72.3°C         Status: Normal    |   |
|  | Last Updated: 2s ago            Line: 1           |   |
|  +---------------------------------------------------+   |
|                                                           |
|  +---------------------------------------------------+   |
|  | Temperature Trend (Last 4 Hours)                  |   |
|  |                                                   |   |
|  |  [Chart.js line graph with thresholds]           |   |
|  |                                                   |   |
|  +---------------------------------------------------+   |
|                                                           |
|  Statistics:                Thresholds:                  |
|  • Min: 68.2°C             • Normal: < 80°C              |
|  • Max: 75.8°C             • High: 80-90°C               |
|  • Avg: 71.5°C             • Critical: > 90°C            |
|  • StdDev: 1.8°C                                         |
|                                                           |
|  Correlated Sensors:        Actions:                     |
|  → Motor Speed (0.87)       [Download CSV]               |
|  → Vibration (0.62)         [Export Graph]               |
|                             [Set Alert]                  |
+----------------------------------------------------------+
```

#### Flags Submission (`flags.html`)

**CTF-Style Interface:**
```
+----------------------------------------------------------+
| Flag Submission                     Exploits: 3/6        |
+----------------------------------------------------------+
|                                                           |
|  +---------------------------------------------------+   |
|  | Your Progress                                     |   |
|  |                                                   |   |
|  |  ✓ Path 1: IDOR (Sensor Access)                  |   |
|  |  ✓ Path 2: Command Injection                     |   |
|  |  ✓ Path 3: Race Condition                        |   |
|  |  ○ Path 4: Physics-Based Analysis                |   |
|  |  ○ Path 5: Forensics                             |   |
|  |  ○ Path 6: Weak Authentication                   |   |
|  +---------------------------------------------------+   |
|                                                           |
|  +---------------------------------------------------+   |
|  | Submit Flag                                       |   |
|  |                                                   |   |
|  | [FLAG{___________________________________________}]  |
|  |                                                   |   |
|  | [Submit]                            [Need Hint?] |   |
|  +---------------------------------------------------+   |
|                                                           |
|  +---------------------------------------------------+   |
|  | Hints (Unlocked: 2/6)                            |   |
|  |                                                   |   |
|  | Level 1: Sensors have IDs like SENSOR-L1-01...   |   |
|  | Level 2: Try accessing sensors from other lines  |   |
|  | Level 3: [Unlock in 3 minutes]                   |   |
|  +---------------------------------------------------+   |
+----------------------------------------------------------+
```

### JavaScript Architecture

#### Real-Time Data Flow

```javascript
class SCADADashboard {
  constructor() {
    this.ws = null;
    this.updateInterval = 2000; // fallback polling
    this.init();
  }
  
  init() {
    // Try WebSocket first
    this.connectWebSocket();
    
    // Fallback to polling
    setTimeout(() => {
      if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
        this.startPolling();
      }
    }, 5000);
    
    // Initialize UI components
    this.initMimicPanel();
    this.initTrendCharts();
    this.initAlarmPanel();
    this.setupEventListeners();
  }
  
  connectWebSocket() {
    this.ws = new WebSocket(`ws://${location.host}/ws`);
    
    this.ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      this.updateDashboard(data);
    };
    
    this.ws.onerror = () => {
      console.warn('WebSocket failed, using polling');
      this.startPolling();
    };
  }
  
  async startPolling() {
    setInterval(async () => {
      const data = await fetch('/api/dashboard/status').then(r => r.json());
      this.updateDashboard(data);
    }, this.updateInterval);
  }
  
  updateDashboard(data) {
    this.updateProcessCards(data.lines);
    this.updateMimicPanel(data.equipment);
    this.updateAlarms(data.alarms);
    this.updateTrends(data.sensors);
  }
}

// Initialize on load
document.addEventListener('DOMContentLoaded', () => {
  window.scadaDashboard = new SCADADashboard();
});
```

#### Canvas P&ID Rendering

```javascript
class PIDCanvas {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext('2d');
    this.zoom = 1.0;
    this.pan = { x: 0, y: 0 };
    this.elements = [];
  }
  
  drawMotor(x, y, status, rpm) {
    const radius = 30;
    
    // Motor body
    this.ctx.beginPath();
    this.ctx.arc(x, y, radius, 0, 2 * Math.PI);
    this.ctx.fillStyle = status === 'running' ? '#26de81' : '#636e72';
    this.ctx.fill();
    this.ctx.strokeStyle = '#fff';
    this.ctx.lineWidth = 2;
    this.ctx.stroke();
    
    // Rotation indicator (animated)
    if (status === 'running') {
      const angle = (Date.now() / 50) % (2 * Math.PI);
      this.ctx.save();
      this.ctx.translate(x, y);
      this.ctx.rotate(angle);
      this.ctx.beginPath();
      this.ctx.moveTo(0, -15);
      this.ctx.lineTo(0, 15);
      this.ctx.strokeStyle = '#fff';
      this.ctx.lineWidth = 3;
      this.ctx.stroke();
      this.ctx.restore();
    }
    
    // RPM label
    this.ctx.fillStyle = '#fff';
    this.ctx.font = '12px monospace';
    this.ctx.textAlign = 'center';
    this.ctx.fillText(`${rpm} RPM`, x, y + radius + 20);
  }
  
  drawValve(x, y, state) {
    const size = 40;
    
    // Valve body
    this.ctx.beginPath();
    this.ctx.moveTo(x - size/2, y);
    this.ctx.lineTo(x, y - size/2);
    this.ctx.lineTo(x + size/2, y);
    this.ctx.lineTo(x, y + size/2);
    this.ctx.closePath();
    
    const colors = {
      open: '#26de81',
      closed: '#636e72',
      stuck: '#ff4757'
    };
    
    this.ctx.fillStyle = colors[state] || '#636e72';
    this.ctx.fill();
    this.ctx.strokeStyle = '#fff';
    this.ctx.lineWidth = 2;
    this.ctx.stroke();
    
    // State indicator
    this.ctx.fillStyle = '#fff';
    this.ctx.font = '10px monospace';
    this.ctx.textAlign = 'center';
    this.ctx.fillText(state.toUpperCase(), x, y + 5);
  }
  
  drawPipe(x1, y1, x2, y2, flow) {
    // Pipe line
    this.ctx.beginPath();
    this.ctx.moveTo(x1, y1);
    this.ctx.lineTo(x2, y2);
    this.ctx.strokeStyle = flow > 0 ? '#45aaf2' : '#636e72';
    this.ctx.lineWidth = 6;
    this.ctx.stroke();
    
    // Flow arrows
    if (flow > 0) {
      const numArrows = Math.floor(Math.hypot(x2 - x1, y2 - y1) / 50);
      for (let i = 0; i < numArrows; i++) {
        const t = (i + 1) / (numArrows + 1);
        const x = x1 + (x2 - x1) * t;
        const y = y1 + (y2 - y1) * t;
        this.drawArrow(x, y, Math.atan2(y2 - y1, x2 - x1));
      }
    }
  }
  
  animate() {
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    
    // Apply zoom and pan
    this.ctx.save();
    this.ctx.translate(this.pan.x, this.pan.y);
    this.ctx.scale(this.zoom, this.zoom);
    
    // Render all elements
    this.elements.forEach(el => {
      switch (el.type) {
        case 'motor':
          this.drawMotor(el.x, el.y, el.status, el.rpm);
          break;
        case 'valve':
          this.drawValve(el.x, el.y, el.state);
          break;
        case 'pipe':
          this.drawPipe(el.x1, el.y1, el.x2, el.y2, el.flow);
          break;
      }
    });
    
    this.ctx.restore();
    requestAnimationFrame(() => this.animate());
  }
}
```

### Responsive Breakpoints

```css
/* Desktop: 1920x1080+ */
@media (min-width: 1920px) {
  .dashboard-grid { grid-template-columns: repeat(4, 1fr); }
  .mimic-panel { height: 800px; }
}

/* Laptop: 1366x768 */
@media (max-width: 1919px) {
  .dashboard-grid { grid-template-columns: repeat(3, 1fr); }
  .mimic-panel { height: 600px; }
}

/* Tablet: 768x1024 */
@media (max-width: 1024px) {
  .dashboard-grid { grid-template-columns: repeat(2, 1fr); }
  .mimic-panel { height: 500px; }
  .trend-chart canvas { height: 200px; }
}

/* Mobile: 375x667 */
@media (max-width: 768px) {
  .dashboard-grid { grid-template-columns: 1fr; }
  .mimic-panel { height: 400px; }
  .control-panel { flex-direction: column; }
  .alarm-panel { position: fixed; bottom: 0; width: 100%; }
}
```

### Accessibility

**WCAG 2.1 AA Compliance:**
- Color contrast ratio ≥ 4.5:1
- Focus indicators on all controls
- Keyboard navigation (Tab, Arrow keys, Enter/Space)
- Screen reader labels (ARIA)
- Skip navigation links

**Keyboard Shortcuts:**
- `Ctrl+D`: Dashboard
- `Ctrl+S`: Sensors
- `Ctrl+A`: Alarms
- `Ctrl+I`: Incidents
- `Ctrl+F`: Flags
- `Esc`: Close modal/dropdown

---

## Example: Full Exploitation Session (NORMAL Mode)

### Scenario: Student Discovers 3 Exploit Paths (45 min)

**T=0:00** → Start in EXPLORATION mode, NORMAL difficulty
- Dashboard: 4 production lines, 20 sensors, 0 incidents
- Hints available via `/api/hints` (must request)

**T=2 min** → Incident #1: STUCK_VALVE_L2_02 (learning observation)
- Alarms trigger; student observes cascading effect on sensors
- Student notes: "When valve stuck, temp rises but flow drops"
- *No objective here; pure learning*

**T=8 min** → Student discovers **IDOR Exploit**
- Tests: `GET /api/sensor/reading?sensor_id=SENSOR-L3-05`
- Realizes: can read sensors from any line (no access check)
- **Exploit Path 1 unlocked** ✓
- Requests hint: "What does IDOR mean?"
- Response: "Insecure Direct Object Reference — you control the object ID and server doesn't verify authorization"

**T=18 min** → Student discovers **Command Injection**
- Crafts payload: `POST /api/actuators/control`
- Body: `{"id":"MOTOR-L1-01","cmd":"set","params":{"speed":"50;simulate:cat /data/db/maintenance.json"}}`
- Response includes file contents with embedded sub-flag
- **Exploit Path 2 unlocked** ✓

**T=35 min** → Second incident: SENSOR_FAULT_L1_03
- Sensor reads constant 99.5°C (no variation)
- Student cross-correlates with vibration sensor (should correlate with motor speed)
- Student diagnoses: "Sensor is stuck, not reacting to physical changes"
- Notes this as **physics-based analysis** (Exploit Path 3 reasoning)

**T=42 min** → Student discovers **Weak Auth (Social Engineering)**
- Finds `maintenance:m4int3n@nc3` in error logs
- Logs in as maintenance user
- Accesses restricted maintenance logs with embedded sub-flag
- **Exploit Path 4 unlocked** ✓

**T=45 min** → Session ends
- **Total Score: 5250 pts**
  - 4 exploit paths × 500 = 2000
  - Time bonus: 3000 - 45×5 = 2775
  - Incidents observed: 2 × 25 = 50
  - Hints used: 2 × -10 = -20
  - **Adjusted: 4805 pts** (Rank #3 in leaderboard)

---

### Advanced Scenario: Teacher Enables Defense (50 min)

**Same student, HARD difficulty + Defense enabled**

**T=25 min** → Teacher deploys IP block (simulates incident response team)
```bash
# Teacher console:
iptables -A INPUT -s 192.168.1.100 -j DROP --duration 60
```
Student sees: `403 Forbidden` on all requests

**T=26 min** → Student adapts
- Realizes: IP is blocked (learned from firewall logs in error)
- Switches to school VPN / different network interface
- Resumes exploitation from new IP

**T=28 min** → Teacher escalates (teaches evasion depth)
```bash
tc qdisc add rate-limit --src 0.0.0.0/0 --duration 300 rate=5-per-min
```
Rate limit: max 5 requests per minute for all users

**T=29 min** → Student adapts (patience + timing)
- Uses sequential exploitation (not parallel)
- Focuses on highest-value exploits first
- Discovers Injection path (single complex payload)

**T=50 min** → Session ends under active defense
- 3 exploit paths discovered (under constraints)
- **Bonus**: Defense Adaptation × 25 min = 625 pts
- **Total: 4200 pts** (high difficulty, lower score but greater achievement)

**Learning Transfer**:
- Student understands WAF triggers and timing
- Practiced evasion techniques on realistic system
- Learned cost/benefit of aggressive defense

---

## Verification Commands

```bash
# List sensors
curl -s http://<ESP_IP>/api/sensors/list | jq .

# Test IDOR: cross-line access
curl -s 'http://<ESP_IP>/api/sensor/reading?sensor_id=SENSOR-L2-03&limit=50' | jq .

# Test command injection
curl -X POST http://<ESP_IP>/api/actuators/control \
  -H 'Content-Type: application/json' \
  -d '{"id":"MOTOR-L2-01","cmd":"set","params":{"speed":"80;simulate:cat /data/db/maintenance.json"}}'

# Submit root flag
curl -X POST http://<ESP_IP>/api/admin/flag \
  -H 'Content-Type: application/json' \
  -d '{"flag":"FLAG{example}"}'

# Get hints
curl -s 'http://<ESP_IP>/api/hints?endpoint=GET_/api/sensor/reading&level=1'

# View leaderboard
curl -s 'http://<ESP_IP>/api/admin/leaderboard?difficulty=NORMAL&sort=score&limit=10' | jq .

# Spawn incident (admin)
curl -X POST http://<ESP_IP>/api/admin/incidents/create \
  -H 'Content-Type: application/json' \
  -d '{"type":"STUCK_VALVE","line":2}'

# Defense: block IP
curl -X POST http://<ESP_IP>/api/defense/block-ip \
  -H 'Content-Type: application/json' \
  -d '{"ip":"192.168.1.100"}'
```

---

## Testing Suite

### Python Tests

```bash
python3 tests/test_sensors.py --check-drift --check-noise --check-correlation
python3 tests/test_exploits.py --difficulty NORMAL --target 192.168.1.100
python3 tests/test_defense.py --run-idor-attack --run-injection-attack --check-blocking
python3 tests/test_flag_flow.py --verify-all-paths --verify-hmac
```

---

## Security & Safety Notes

- ✅ **No actual shell execution** on host; all injection simulated
- ✅ **Config-driven security**: weak secrets only in EASY/testing mode
- ✅ **HMAC-based flag storage**: plaintext never written to device
- ✅ **All flaws gated**: explicit `VULN_*` + `LAB_MODE` checks
- ✅ **Incident-driven learning**: realistic failure propagation

---

## Key Decisions & Rationale

| Decision | Rationale |
|----------|-----------|
| **6 exploit paths as primary** | Diverse thinking required; students must explore multiple attack vectors |
| **Red Team focus, Defense secondary** | Exploit *discovery* is the learning goal; defense is a *complication* (like reality) |
| **Defense via Serial commands (not web UI)** | Teachers control scenario; students adapt; prevents "defense gaming" |
| **No root flag submission required** | Exploits are the objective, not flag collection; reduces distraction |
| **Incidents as learning tools, not objectives** | Cascading failures teach causality + forensics | 
| **Physics-based sensors** | Realism enables analysis-based exploits (cross-correlation detection) |
| **3 difficulty levels** | Scaffolds learning: guided → discovery → expert |
| **Leaderboard: exploits + time** | Incentivizes breadth (find all paths) + speed (efficient exploitation) |
| **Config-driven vulnerabilities** | Easy to enable/disable per class; customizable difficulty |
| **HMAC flag storage** | Security best-practice; avoids plaintext secrets on device |

---

## Next Steps

1. ✅ **Plan approved** → Move to Phase 1 scaffolding
2. Create `.ino` file stubs
3. Implement `01_Config.ino` (master control)
4. Implement `05_Database.ino` + seed JSONs
5. Begin Phase 2 (physics, sensors, actuators)

**Status**: Ready for implementation. All configuration variables defined. Ready to build! 🚀
