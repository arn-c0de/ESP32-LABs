# Smart Grid Energy Lab v2.0 — Complete Design & Implementation Plan ⚡

**Version:** 2.0 | **Platform:** ESP32 | **Target Date:** April 2026 | **Language:** English

---

## Executive Summary 📋

A comprehensive Smart Grid security laboratory running on ESP32 that combines realistic energy infrastructure simulation with educational gamification. The system features multiple learning paths (Red/Blue/Admin/Architect), configurable difficulty levels, campaign-based missions, and a full defense system - all configurable via JSON without hardcoding.

**Key Enhancements v2.0:**
✅ Multiple learning paths with distinct objectives  
✅ 4 difficulty presets + custom parameter adjustment  
✅ Campaign & mission system with progressive unlocking  
✅ Gamification: XP, levels, achievements, leaderboards  
✅ Multiple paths to success for each mission  
✅ Complete JSON-driven configuration  
✅ Enhanced web UI with mission dashboard  
✅ Comprehensive audit and forensics tools  

---

## Table of Contents 📚

1. [System Architecture](#system-architecture)
2. [Learning Paths](#learning-paths)
3. [Difficulty System](#difficulty-system)
4. [Campaign & Mission System](#campaign-mission-system)
5. [Multiple Paths to Success](#multiple-paths-to-success)
6. [Gamification System](#gamification-system)
7. [Configuration System](#configuration-system)
8. [Data Model & Storage](#data-model-storage)
9. [API Endpoints](#api-endpoints)
10. [Vulnerabilities & Defense](#vulnerabilities-defense)
11. [Frontend & UX](#frontend-ux)
12. [Implementation Roadmap](#implementation-roadmap)
13. [Testing & Validation](#testing-validation)

---

## System Architecture 🏗️

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 Smart Grid v2.0                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ 1. SCENARIO ENGINE                                  │  │
│  │    - Campaign Management                            │  │
│  │    - Mission Tracking & Unlocking                   │  │
│  │    - Objective Evaluation                           │  │
│  │    - Dynamic Reward Calculation                     │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ 2. DIFFICULTY ENGINE                                │  │
│  │    - JSON-based Configuration                       │  │
│  │    - Presets: Easy/Intermediate/Hard/Expert         │  │
│  │    - Custom Slider Adjustments                      │  │
│  │    - Live Parameter Updates                         │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ 3. PROGRESSION SYSTEM                               │  │
│  │    - Experience Points & Leveling                   │  │
│  │    - Achievement Tracking                           │  │
│  │    - Content Unlocking & Gating                     │  │
│  │    - Leaderboard Management                         │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ 4. SIMULATION CORE                                  │  │
│  │    - Smart Meter Simulation                         │  │
│  │    - Consumption Time-Series                        │  │
│  │    - Tariff-Based Billing Engine                    │  │
│  │    - Defense System (DP/AP/SS)                      │  │
│  │    - Intentional Vulnerabilities                    │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ 5. DATA LAYER                                       │  │
│  │    - LittleFS Persistence                           │  │
│  │    - Snapshot & Rollback                            │  │
│  │    - JSON Export/Import                             │  │
│  │    - Audit Log Streaming                            │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ 6. INTERFACE LAYER                                  │  │
│  │    - Web UI (HTML/CSS/JS from LittleFS)            │  │
│  │    - REST API (JSON)                                │  │
│  │    - WebSocket (Real-time updates)                  │  │
│  │    - Telnet CLI (Commands & Hints)                  │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Learning Paths 👥

### 1. RED TEAM PATH (Security Researcher Focus)

**Objective:** Exploit vulnerabilities and break the system

**Missions:**
- `MISSION_IDOR_01` — Cross-customer data access
- `MISSION_METER_TAMPER_01` — Manipulate meter readings
- `MISSION_RACE_CONDITION` — Generate double-billing
- `MISSION_AUTH_BYPASS` — Session hijacking
- `MISSION_ADMIN_ESCALATION` — Privilege escalation
- `MISSION_CHAIN_ATTACK` — Multi-step exploit chains

**Scoring:** 10-150 points per mission, bonus for stealth (no alerts)

**Skills Developed:**
- IDOR Penetration (Level 1-10)
- Advanced Exploitation (Level 1-10)
- Race Condition Mastery (Level 1-10)
- Stealth Operations (Level 1-10)

**Recommended Difficulty:** Hard/Expert

---

### 2. BLUE TEAM PATH (Defender Focus)

**Objective:** Detect and block attacks with limited resources

**Missions:**
- `DEF_DETECT_IDOR` — Identify unusual request patterns
- `DEF_RATE_LIMIT` — Contain DDoS with `tc` commands
- `DEF_IP_BLOCK` — Block attackers with `iptables`
- `DEF_SESSION_MGMT` — Reset hijacked sessions
- `DEF_ALERT_CONFIG` — Configure automatic alert rules
- `DEF_INCIDENT_RESPONSE` — Full 30-minute scenario

**Scoring:** Based on response speed, service availability, and resource efficiency

**Resource Budget:** DP/AP/SS management critical

**Skills Developed:**
- Attention to Detail (Level 1-10)
- Detection Engineering (Level 1-10)
- Resource Management (Level 1-10)
- Incident Response (Level 1-10)

**Recommended Difficulty:** Intermediate/Hard

---

### 3. ADMIN PATH (Operations Focus)

**Objective:** Maintain system health, manage tariffs, generate reports

**Missions:**
- `ADMIN_SETUP` — Initial configuration and user creation
- `ADMIN_TARIFF` — Adjust tariff plans for scenarios
- `ADMIN_REPORTING` — Create consumption/revenue reports
- `ADMIN_MAINTENANCE` — Calibrate meters, clean data
- `ADMIN_AUDIT` — Search audit trails for key events
- `ADMIN_COMPLIANCE` — Enforce regulatory rules

**Scoring:** Accuracy, timeliness, cost optimization

**Skills Developed:**
- System Operations (Level 1-10)
- Data Management (Level 1-10)
- Compliance & Audit (Level 1-10)
- Cost Optimization (Level 1-10)

**Recommended Difficulty:** Easy/Intermediate

---

### 4. ARCHITECT PATH (System Design Focus)

**Objective:** Configure, extend, and optimize the system

**Missions:**
- `ARCH_PERF_TUNE` — Optimize memory/CPU without losing features
- `ARCH_SECURITY_HARDENING` — Document vulnerabilities and countermeasures
- `ARCH_CAPACITY_PLANNING` — Plan scalability for N meters/customers
- `ARCH_DISASTER_RECOVERY` — Document backup/restore strategy
- `ARCH_INTEGRATION` — Multi-device setup (UDP/MQTT)

**Scoring:** Design quality, documentation, best practices adherence

**Skills Developed:**
- System Thinking (Level 1-10)
- Security Architecture (Level 1-10)
- Performance Engineering (Level 1-10)
- Enterprise Design (Level 1-10)

**Recommended Difficulty:** Expert

---

## Difficulty System ⚙️

### Preset Difficulty Profiles

#### EASY
```json
{
  "preset": "easy",
  "vulnerability_exposure": 0.3,
  "defense_dp_regen": 5,
  "alert_sensitivity": 0.3,
  "time_acceleration": 1,
  "meter_noise_factor": 0.1,
  "attacker_skill_simulation": 0.2,
  "hint_system": {
    "enabled": true,
    "detail_level": "high",
    "progressive_hints": true,
    "hint_cost": 0
  },
  "ui_guidance": {
    "show_vulnerable_endpoints": true,
    "highlight_attack_vectors": true,
    "tutorial_mode": true
  },
  "defense_costs": {
    "ip_block_dp": 10,
    "rate_limit_dp": 8,
    "session_reset_dp": 15
  }
}
```

#### INTERMEDIATE
```json
{
  "preset": "intermediate",
  "vulnerability_exposure": 0.6,
  "defense_dp_regen": 3,
  "alert_sensitivity": 0.6,
  "time_acceleration": 2,
  "meter_noise_factor": 0.2,
  "attacker_skill_simulation": 0.5,
  "hint_system": {
    "enabled": true,
    "detail_level": "medium",
    "progressive_hints": true,
    "hint_cost": 5
  },
  "defense_costs": {
    "ip_block_dp": 15,
    "rate_limit_dp": 10,
    "session_reset_dp": 25
  }
}
```

#### HARD
```json
{
  "preset": "hard",
  "vulnerability_exposure": 0.8,
  "defense_dp_regen": 2,
  "alert_sensitivity": 0.8,
  "time_acceleration": 4,
  "meter_noise_factor": 0.3,
  "attacker_skill_simulation": 0.8,
  "hint_system": {
    "enabled": true,
    "detail_level": "low",
    "progressive_hints": false,
    "hint_cost": 10
  },
  "defense_costs": {
    "ip_block_dp": 20,
    "rate_limit_dp": 15,
    "session_reset_dp": 35
  }
}
```

#### EXPERT
```json
{
  "preset": "expert",
  "vulnerability_exposure": 1.0,
  "defense_dp_regen": 1,
  "alert_sensitivity": 1.0,
  "time_acceleration": 8,
  "meter_noise_factor": 0.5,
  "attacker_skill_simulation": 1.0,
  "hint_system": {
    "enabled": false,
    "detail_level": "none",
    "progressive_hints": false,
    "hint_cost": 0
  },
  "defense_costs": {
    "ip_block_dp": 30,
    "rate_limit_dp": 20,
    "session_reset_dp": 50
  }
}
```

### Custom Parameter Sliders 🎚️

Users can fine-tune individual parameters:

- **Vulnerability Exposure** (0.0-1.0): Controls how obvious vulnerabilities are
- **Defense DP Regeneration** (1-10): DP recovery per 5 seconds
- **Alert Sensitivity** (0.0-1.0): Threshold for triggering alerts
- **Time Acceleration** (1x-16x): Simulation speed multiplier
- **Meter Noise** (0.0-1.0): Randomness in consumption data
- **Attacker Skill** (0.0-1.0): Simulated attacker sophistication

---

## Campaign & Mission System 🎮

### Campaign Structure

```json
{
  "campaign_id": "CAMPAIGN_GRID_AWAKENS",
  "title": "The Grid Awakens",
  "description": "Learn the fundamentals of Smart Grid infrastructure",
  "difficulty": "easy",
  "duration_minutes": 120,
  "total_points": 100,
  "missions": [
    {
      "mission_id": "MISSION_WELCOME",
      "title": "Welcome to the Grid",
      "objective": "Explore the dashboard and understand basic concepts",
      "locked_until": null,
      "points": 10,
      "expected_time_minutes": 15,
      "hints": [
        "Check the dashboard for an overview",
        "View meter status to see current readings",
        "Explore the API documentation"
      ],
      "success_condition": "visited_dashboard && viewed_meters"
    },
    {
      "mission_id": "MISSION_READ_CONSUMPTION",
      "title": "Read Consumption Data",
      "objective": "Access consumption history for your assigned meter",
      "locked_until": "MISSION_WELCOME",
      "points": 15,
      "expected_time_minutes": 20,
      "attack_path": "legitimate_access",
      "hints": [
        "Use GET /api/consumption/history",
        "Your meter_id is displayed on the dashboard",
        "Check the API response format"
      ]
    },
    {
      "mission_id": "MISSION_FIRST_BILL",
      "title": "Generate First Invoice",
      "objective": "Generate and view your first electricity bill",
      "locked_until": "MISSION_READ_CONSUMPTION",
      "points": 20,
      "expected_time_minutes": 25
    }
  ]
}
```

### Campaign Progression Example

```
Campaign: "Red Alert" (Security-Focused)
├── Mission 1: Reconnaissance [UNLOCKED]
│   ├── Objective: Enumerate meter IDs
│   ├── Hints: 3 available
│   ├── Points: 20
│   └── Unlocks: Mission 2
│
├── Mission 2: IDOR Exploitation [LOCKED until Mission 1]
│   ├── Objective: Access 3 bills from other customers
│   ├── Multiple Paths:
│   │   ├── Path A: Meter enumeration + API (20 min, 30 pts)
│   │   ├── Path B: ID prediction (25 min, 35 pts)
│   │   └── Path C: Brute force (40 min, 25 pts)
│   └── Unlocks: Mission 3
│
├── Mission 3: Race Condition [LOCKED until Mission 2]
│   ├── Objective: Generate double-billing
│   ├── Points: 70
│   └── Unlocks: Mission 4
│
└── Mission 4: Meter Tampering [LOCKED until Mission 3]
    ├── Objective: Manipulate meter readings
    ├── Difficulty: Expert only
    ├── Points: 100
    └── Unlocks: Campaign Complete
```

---

## Multiple Paths to Success 🌳

### Concept
Each mission can be solved in multiple ways, catering to different skill levels and approaches.

### Example Mission: "Falsify Billing"

#### PATH 1: IDOR Data Access (Red, Beginner)
- **Difficulty:** ⭐⭐ (Easy)
- **Time:** 15-20 minutes
- **Points:** 30 base → 50 with speed bonus
- **Skill XP:** +20 IDOR Penetration

**Strategy:**
1. Enumerate meter IDs
2. Exploit IDOR in billing API
3. Read invoice data
4. Document findings

**Success Condition:** Access 3+ bills from other customers

---

#### PATH 2: Race Condition (Red, Advanced)
- **Difficulty:** ⭐⭐⭐ (Intermediate/Hard)
- **Time:** 45-70 minutes
- **Points:** 70 base → 100 perfect
- **Skill XP:** +50 Race Condition Exploitation

**Strategy:**
1. Understand billing engine (500ms race window)
2. Write parallel request scripts
3. Generate double-billing (€150 × 2 = €300)
4. Exploit timing window

**Example Exploit:**
```python
import requests
import threading

def trigger_billing():
    requests.post('http://esp32/api/billing/generate',
                  json={'meter_id': 5},
                  headers={'Authorization': f'Bearer {TOKEN}'})

t1 = threading.Thread(target=trigger_billing)
t2 = threading.Thread(target=trigger_billing)
t1.start()
t2.start()
t1.join()
t2.join()
# Result: 2 bills in database, each €150
```

**Success Condition:** 2 bills for same meter in same period

---

#### PATH 3: Meter Tampering (Red, Expert)
- **Difficulty:** ⭐⭐⭐⭐ (Hard/Expert)
- **Time:** 60-90 minutes
- **Points:** 100 base → 150 with speed bonus
- **Skill XP:** +100 Advanced Exploitation

**Strategy:**
1. Find hidden `/vuln/meter-tamper` endpoint
2. Manipulate meter readings
3. Falsify consumption data
4. Generate billing from false data

**Success Condition:** Consumption >50% lower, bill -€75+, audit trail shows tamper

---

#### PATH 4: Admin Tariff Manipulation (Admin/Blue)
- **Difficulty:** ⭐⭐ (Easy/Intermediate)
- **Time:** 20-30 minutes
- **Points:** 40 base → 60 with governance
- **Skill XP:** +30 Admin Operations

**Strategy:**
1. Use admin credentials
2. Edit tariff definitions
3. Set peak prices to €0
4. Generate €0 invoice

**Success Condition:** Tariff changed to €0, bill shows €0, audit logged

---

#### PATH 5: Defense Detection & Blocking (Blue)
- **Difficulty:** ⭐⭐⭐ (Intermediate)
- **Time:** 30-45 minutes
- **Points:** 60 base → 80 without false positives
- **Skill XP:** +40 Detection & Response

**Strategy:**
1. Monitor for red team attacks
2. Identify anomalies in logs
3. Determine attacker IP
4. Deploy defense measures (iptables/rate-limit)
5. Verify blockage, maintain service availability

**Success Condition:** Correct IP identified, appropriate defense applied, requests blocked, no false positives

---

## Gamification System 🏆

### Experience & Leveling

```json
{
  "user_profile": {
    "username": "sec_student",
    "level": 8,
    "xp_total": 3420,
    "xp_next_level": 4000,
    "skills": {
      "idor_penetration": {
        "level": 5,
        "xp": 850,
        "xp_next": 1000
      },
      "race_condition_mastery": {
        "level": 3,
        "xp": 420,
        "xp_next": 600
      },
      "defense_operations": {
        "level": 6,
        "xp": 1100,
        "xp_next": 1200
      }
    }
  }
}
```

### Achievement Catalog

```json
{
  "achievements": [
    {
      "id": "ACH_FIRST_BLOOD",
      "name": "First Blood",
      "description": "Complete your first exploit",
      "category": "red_team",
      "points": 10,
      "icon": "🩸"
    },
    {
      "id": "ACH_IDOR_LORD",
      "name": "IDOR Lord",
      "description": "Successfully exploit 10 IDOR vulnerabilities",
      "category": "red_team",
      "points": 50,
      "icon": "👑"
    },
    {
      "id": "ACH_PHANTOM_CHARGES",
      "name": "Phantom Charges",
      "description": "Generate double-billing via race condition",
      "category": "red_team",
      "points": 75,
      "icon": "👻"
    },
    {
      "id": "ACH_BLUE_GUARDIAN",
      "name": "Blue Guardian",
      "description": "Successfully defend against 10 attacks",
      "category": "blue_team",
      "points": 60,
      "icon": "🛡️"
    },
    {
      "id": "ACH_PATH_MASTER",
      "name": "Path Master",
      "description": "Complete same mission using 3 different paths",
      "category": "exploration",
      "points": 100,
      "icon": "🌟"
    },
    {
      "id": "ACH_SPEED_DEMON",
      "name": "Speed Demon",
      "description": "Complete campaign in under 60 minutes",
      "category": "performance",
      "points": 80,
      "icon": "⚡"
    }
  ]
}
```

### Leaderboard

```json
{
  "leaderboard": {
    "global": [
      {
        "rank": 1,
        "username": "red_ninja",
        "total_points": 9850,
        "level": 18,
        "paths_completed": ["red", "blue", "admin"]
      },
      {
        "rank": 2,
        "username": "sec_student",
        "total_points": 8420,
        "level": 15,
        "paths_completed": ["red", "blue"]
      }
    ],
    "by_campaign": {
      "CAMPAIGN_GRID_AWAKENS": [
        {
          "rank": 1,
          "username": "speed_runner",
          "points": 150,
          "time_minutes": 45
        }
      ]
    },
    "by_path": {
      "MISSION_FRAUD_01": {
        "idor_path": {
          "winner": "race_expert",
          "points": 100,
          "time": "12:34"
        },
        "race_condition": {
          "winner": "timing_master",
          "points": 120,
          "time": "18:23"
        }
      }
    }
  }
}
```

---

## Configuration System 📝

### Master Configuration File: `config.json`

```json
{
  "system": {
    "version": "2.0.0",
    "lab_mode": true,
    "enable_vulnerabilities": true,
    "enable_defense": true,
    "enable_gamification": true,
    "enable_campaigns": true,
    "language": "en"
  },
  
  "difficulty": {
    "current_preset": "intermediate",
    "presets": {
      "easy": { /* see Difficulty System section */ },
      "intermediate": { /* see above */ },
      "hard": { /* see above */ },
      "expert": { /* see above */ }
    },
    "custom": {
      "vulnerability_exposure": 0.6,
      "defense_dp_regen": 3,
      "alert_sensitivity": 0.6,
      "time_acceleration": 2,
      "meter_noise_factor": 0.2
    }
  },
  
  "vulnerabilities": {
    "VULN_IDOR_CONSUMPTION": {
      "enabled": true,
      "exposure_level": 0.8,
      "audit_log": true
    },
    "VULN_IDOR_BILLING": {
      "enabled": true,
      "exposure_level": 0.7,
      "audit_log": true
    },
    "VULN_METER_TAMPER": {
      "enabled": true,
      "exposure_level": 0.3,
      "endpoint": "/vuln/meter-tamper",
      "audit_log": true
    },
    "VULN_RACE_BILLING": {
      "enabled": true,
      "race_window_ms": 500,
      "audit_log": true
    }
  },
  
  "defense": {
    "resources": {
      "dp_max": 100,
      "dp_regen_per_interval": 3,
      "dp_regen_interval_sec": 5,
      "ap_max": 10,
      "ss_max": 100
    },
    "rules": {
      "ip_block": {
        "cost_dp": 15,
        "cost_ap": 1,
        "max_duration_sec": 3600
      },
      "rate_limit": {
        "cost_dp": 10,
        "cost_ap": 1,
        "max_duration_sec": 1800
      },
      "session_reset": {
        "cost_dp": 25,
        "cost_ap": 2
      }
    },
    "auto_defense": {
      "enabled": false,
      "rules": [
        {
          "trigger": "IDOR_10_ATTEMPTS",
          "action": "IP_BLOCK",
          "duration_sec": 300
        }
      ]
    }
  },
  
  "simulation": {
    "meters": {
      "count": 10,
      "types": ["electricity", "water", "gas"],
      "base_consumption": {
        "electricity": 5.0,
        "water": 2.0,
        "gas": 1.5
      },
      "noise_factor": 0.15,
      "update_interval_sec": 10
    },
    "time": {
      "acceleration_factor": 2,
      "start_date": "2026-01-01T00:00:00Z"
    },
    "billing": {
      "generation_interval_days": 30,
      "tariff_type": "dynamic"
    }
  },
  
  "gamification": {
    "xp_per_mission_base": 10,
    "xp_multiplier_easy": 1.0,
    "xp_multiplier_intermediate": 1.5,
    "xp_multiplier_hard": 2.0,
    "xp_multiplier_expert": 3.0,
    "level_up_xp_base": 1000,
    "level_up_xp_multiplier": 1.2
  }
}
```

### Configuration Management Commands

**Telnet CLI:**
```bash
> config show
> config set difficulty.preset hard
> config set custom.vulnerability_exposure 0.9
> config save
> config reload
> config export > config_backup.json
```

**Web UI:**
- Config panel with sliders for real-time adjustment
- Preset buttons: [Easy] [Intermediate] [Hard] [Expert]
- Module toggles: Vulnerabilities, Defense, Gamification, Campaigns
- Export/Import buttons

---

## Data Model & Storage 📂

### LittleFS Structure

```
/data/
  ├── config.json                 # Master configuration
  ├── campaigns.json              # Campaign definitions
  ├── achievements.json           # Achievement catalog
  ├── index.html                  # Main dashboard
  ├── dashboard.html              # User dashboard
  ├── consumption.html            # Consumption charts
  ├── billing.html                # Invoice history
  ├── meter-details.html          # Meter metadata
  ├── admin.html                  # Admin panel
  ├── missions.html               # Mission dashboard
  ├── leaderboard.html            # Leaderboard view
  ├── config-panel.html           # Configuration UI
  ├── shell.html                  # WebSocket shell
  ├── css/
  │   └── style.css
  └── js/
      ├── auth-sync.js
      ├── mode.js
      ├── navbar.js
      ├── mission-controller.js
      └── config-controller.js

/db/
  ├── customers.json              # User accounts & roles
  ├── meters.json                 # Meter metadata
  ├── consumption.json            # Time-series data (ring buffer)
  ├── bills.json                  # Invoices & payment status
  ├── tariffs.json                # Tariff definitions
  ├── audit-logs.json             # Security & action logs
  ├── user-profiles.json          # XP, levels, skills
  ├── user-achievements.json      # Unlocked achievements
  └── campaign-progress.json      # Mission completion tracking
```

### Data Models

#### Customer Profile
```json
{
  "customer_id": 1,
  "username": "customer1",
  "password_hash": "...",
  "role": "customer",
  "assigned_meters": [1, 2],
  "balance": 150.75,
  "created_at": "2026-01-15T10:30:00Z"
}
```

#### Meter
```json
{
  "meter_id": 1,
  "type": "electricity",
  "customer_id": 1,
  "location": "Building A",
  "calibration_status": "valid",
  "last_reading": 1234.56,
  "last_reading_time": "2026-02-06T14:30:00Z"
}
```

#### Consumption Record
```json
{
  "meter_id": 1,
  "timestamp": "2026-02-06T14:00:00Z",
  "value": 5.23,
  "unit": "kWh"
}
```

#### Bill
```json
{
  "bill_id": 100,
  "customer_id": 1,
  "meter_id": 1,
  "period_start": "2026-01-01T00:00:00Z",
  "period_end": "2026-01-31T23:59:59Z",
  "total_consumption": 450.5,
  "amount": 150.75,
  "status": "unpaid",
  "generated_at": "2026-02-01T00:00:00Z"
}
```

#### Audit Log
```json
{
  "log_id": 1234,
  "timestamp": "2026-02-06T14:35:22Z",
  "event_type": "IDOR_ATTEMPT",
  "severity": "high",
  "source_ip": "192.168.4.50",
  "user": "customer1",
  "details": "Attempted access to meter_id=5 (not owned)",
  "blocked": true
}
```

---

## API Endpoints 🔌

### Mission & Campaign Endpoints

```
GET  /api/mission/current              - Get current active mission
GET  /api/mission/list                 - List all missions for user
POST /api/mission/:id/hint             - Request hint (may cost XP)
POST /api/mission/:id/complete         - Mark mission complete with proof

GET  /api/campaign/list                - List available campaigns
POST /api/campaign/:id/start           - Start a campaign
GET  /api/campaign/:id/progress        - Get campaign progress
```

### Gamification Endpoints

```
GET  /api/user/profile                 - Get user profile (XP, level, skills)
POST /api/user/xp/award                - Award XP (admin only)
GET  /api/user/achievements            - Get user achievements
GET  /api/leaderboard/global           - Global leaderboard
GET  /api/leaderboard/campaign/:id     - Campaign-specific leaderboard
```

### Configuration Endpoints

```
GET  /api/admin/config                 - Get current configuration
POST /api/admin/config/difficulty      - Set difficulty preset
POST /api/admin/config/custom          - Set custom parameter
POST /api/admin/config/save            - Save config to LittleFS
```

### Meter & Consumption Endpoints

```
GET  /api/meter/status?meter_id=<id>              - Current reading
GET  /api/meter/list                               - List all meters (filtered by role)
GET  /api/consumption/history?meter_id=<id>&from=&to=  - Time-series (IDOR vuln)
```

### Billing Endpoints

```
GET  /api/billing/invoice?bill_id=<id>       - Invoice details (IDOR vuln)
POST /api/billing/generate?meter_id=<id>     - Generate invoice (race condition)
GET  /api/billing/list?customer_id=<id>      - List invoices
```

### Vulnerability Endpoints (Lab Mode)

```
POST /vuln/meter-tamper?meter_id=<id>&value=<val>  - Inject false readings
GET  /vuln/endpoints                                - List vulnerable endpoints (easy mode)
```

### Defense Endpoints

```
GET  /api/defense/status                    - Get DP/AP/SS and active rules
POST /api/defense/rule/ip-block             - Block IP
POST /api/defense/rule/rate-limit           - Rate limit IP/subnet
POST /api/defense/rule/session-reset        - Reset session
GET  /api/defense/logs                      - Defense action logs
```

---

## Vulnerabilities & Defense ⚠️🛡️

### Vulnerability Flags

| Flag | Description | Exposure Control |
|------|-------------|------------------|
| `VULN_IDOR_CONSUMPTION` | Cross-customer consumption access | Easy: 0.3, Hard: 0.8 |
| `VULN_IDOR_BILLING` | Cross-customer invoice access/update | Easy: 0.3, Hard: 0.7 |
| `VULN_METER_TAMPER` | Endpoint to inject false readings | Easy: obvious, Expert: hidden |
| `VULN_RACE_BILLING` | No locking in billing generation | Race window: 500ms |
| `PROTECT_ADMIN_ENDPOINTS` | Admin access control | Lab mode: false |

### Defense System

**Resources:**
- **DP (Defense Points):** 100 max, regen 1-5 per 5 sec (difficulty-based)
- **AP (Action Points):** 10 max, regen 1 per 60 sec
- **SS (System Stability):** 100 max, degrades under attack, regen 2 per 10 sec

**Actions:**

| Action | Cost | Effect | Max Duration |
|--------|------|--------|--------------|
| IP Block | DP 15-30, AP 1 | DROP packets from IP | 3600 sec |
| Rate Limit | DP 10-20, AP 1 | Limit requests/sec from IP | 1800 sec |
| Session Reset | DP 25-50, AP 2 | Invalidate user sessions | Immediate |
| Mistrust Mode | DP 40, AP 1 | Extra validation for IP/user | 600 sec |

**Auto-Defense Rules (Optional):**
```json
{
  "auto_defense": {
    "enabled": true,
    "rules": [
      {
        "trigger": "IDOR_10_ATTEMPTS_1MIN",
        "action": "IP_BLOCK",
        "duration_sec": 300,
        "cost_override": null
      },
      {
        "trigger": "RATE_100_REQ_PER_SEC",
        "action": "RATE_LIMIT",
        "duration_sec": 600
      }
    ]
  }
}
```

### Audit Logging

All security events logged to `db/audit-logs.json`:

```json
{
  "timestamp": "2026-02-06T15:20:45Z",
  "event_type": "IDOR_ATTEMPT",
  "severity": "high",
  "source_ip": "192.168.4.50",
  "user": "customer1",
  "endpoint": "/api/consumption/history",
  "params": {"meter_id": 5},
  "blocked": true,
  "defense_action": "IP_BLOCK_300SEC"
}
```

---

## Frontend & UX 🌐

### Web Pages

| Page | Purpose | Key Features |
|------|---------|--------------|
| `dashboard.html` | Main overview | Current meters, consumption, quick stats |
| `missions.html` | Mission dashboard | Active mission, progress, hints, completion |
| `consumption.html` | Consumption charts | Time-series graphs (Chart.js), export CSV |
| `billing.html` | Invoice history | List invoices, payment status, download PDF |
| `meter-details.html` | Meter metadata | IDOR testbed, detailed readings |
| `admin.html` | Admin panel | Tariff management, user management, system config |
| `leaderboard.html` | Leaderboard | Global/campaign rankings, path comparison |
| `config-panel.html` | Configuration UI | Difficulty sliders, module toggles, export/import |
| `shell.html` | WebSocket CLI | Interactive terminal for advanced commands |

### Mission Dashboard Example

```html
<div id="mission-panel">
  <h2>Current Mission</h2>
  <div class="mission-card">
    <h3 id="mission-title">IDOR Exploitation</h3>
    <p id="mission-objective">Access 3 bills from other customers</p>
    <div class="progress-bar">
      <div class="progress-fill" style="width: 33%;">1/3</div>
    </div>
    <button onclick="getHint()" class="hint-btn">💡 Get Hint (Cost: 5 XP)</button>
    <button onclick="completeMission()" class="complete-btn">✓ Complete</button>
  </div>
  
  <div class="hints-section" id="hints">
    <h4>Hints</h4>
    <ul>
      <li>Try enumerating bill_id values</li>
      <li>The API doesn't validate customer ownership</li>
      <li class="locked">🔒 Unlock next hint (5 XP)</li>
    </ul>
  </div>
</div>
```

### Configuration Panel UI

```html
<div class="config-panel">
  <h2>System Configuration</h2>
  
  <div class="difficulty-presets">
    <button onclick="setDifficulty('easy')" class="preset-btn">Easy</button>
    <button onclick="setDifficulty('intermediate')" class="preset-btn active">Intermediate</button>
    <button onclick="setDifficulty('hard')" class="preset-btn">Hard</button>
    <button onclick="setDifficulty('expert')" class="preset-btn">Expert</button>
  </div>
  
  <div class="custom-sliders">
    <label>Vulnerability Exposure <span id="vuln-val">0.6</span></label>
    <input type="range" min="0" max="100" value="60" oninput="updateConfig('vuln_exposure', this.value/100)">
    
    <label>Defense DP Regen <span id="dp-regen-val">3</span></label>
    <input type="range" min="1" max="10" value="3" oninput="updateConfig('dp_regen', this.value)">
    
    <label>Time Acceleration <span id="time-accel-val">2x</span></label>
    <input type="range" min="1" max="16" value="2" oninput="updateConfig('time_accel', this.value)">
  </div>
  
  <div class="module-toggles">
    <label><input type="checkbox" checked> Vulnerabilities</label>
    <label><input type="checkbox" checked> Defense System</label>
    <label><input type="checkbox" checked> Gamification</label>
    <label><input type="checkbox" checked> Campaigns</label>
  </div>
  
  <button onclick="saveConfig()">💾 Save</button>
  <button onclick="exportConfig()">📤 Export</button>
  <button onclick="resetConfig()">🔄 Reset to Default</button>
</div>
```

### JavaScript Controllers

**Mission Controller:**
```javascript
class MissionController {
  async getCurrentMission() {
    const res = await fetch('/api/mission/current');
    return res.json();
  }
  
  async completeMission(proof) {
    const res = await fetch('/api/mission/complete', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({proof})
    });
    return res.json();
  }
  
  async getHint(missionId, level) {
    const res = await fetch(`/api/mission/${missionId}/hint`, {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({level})
    });
    return res.json();
  }
}
```

**Config Controller:**
```javascript
class ConfigController {
  async loadConfig() {
    const res = await fetch('/api/admin/config');
    return res.json();
  }
  
  async setDifficulty(preset) {
    const res = await fetch('/api/admin/config/difficulty', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({preset})
    });
    return res.json();
  }
  
  async setCustomParam(key, value) {
    const res = await fetch('/api/admin/config/custom', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({key, value})
    });
    return res.json();
  }
}
```

---

## Implementation Roadmap 🛣️

### Phase 1: Core Systems (Weeks 1-2)

**Files to Create:**
- `00_Config.ino` (300 lines) — Config manager with JSON parsing
- `06_Campaigns.ino` (400 lines) — Campaign & mission engine
- `11_Gamification.ino` (350 lines) — XP, levels, achievements

**Tasks:**
- [ ] JSON config system with LittleFS
- [ ] Difficulty preset loading
- [ ] Campaign structure & mission tracking
- [ ] Mission locking/unlocking logic
- [ ] XP calculation & level-up system
- [ ] Achievement tracking

**Test Criteria:**
✅ Config loads on boot  
✅ All JSON parameters accessible  
✅ Preset switching works  
✅ Missions lock/unlock properly  
✅ XP awards correctly  
✅ Achievements unlock on conditions  

---

### Phase 2: Gamification & UX (Week 3)

**Files to Create/Update:**
- `data/missions.html` — Mission dashboard
- `data/leaderboard.html` — Rankings
- `data/config-panel.html` — Config UI
- `js/mission-controller.js` — Frontend mission logic
- `js/config-controller.js` — Frontend config logic

**Tasks:**
- [ ] Mission panel with progress bars
- [ ] Hint system with cost display
- [ ] Achievement showcase
- [ ] Leaderboard with filters
- [ ] Config sliders and toggles

**Test Criteria:**
✅ Mission UI displays current mission  
✅ Hints unlock progressively  
✅ Leaderboards sort correctly  
✅ Config changes apply immediately  

---

### Phase 3: Simulation Parameters (Week 4)

**Files to Update:**
- `07_Meters.ino` — Parameterize simulation
- `08_Consumption.ino` — Time acceleration
- `09_Billing.ino` — Race window config

**Tasks:**
- [ ] Meter noise from config
- [ ] Time acceleration dynamic
- [ ] Billing parameters configurable
- [ ] Tariff type switching

**Test Criteria:**
✅ `time_acceleration=8x` → time runs 8x faster  
✅ `meter_noise=0.5` → data varies ±50%  
✅ `tariff_type=dynamic` → prices change  

---

### Phase 4: API Endpoints (Weeks 5-6)

**Files to Update:**
- `03_WebServer.ino` — Add new handlers

**New Endpoints:**
```cpp
// Mission endpoints (15 new)
server.on("/api/mission/current", HTTP_GET, handleMissionCurrent);
server.on("/api/mission/list", HTTP_GET, handleMissionList);
server.on("/api/mission/:id/hint", HTTP_POST, handleMissionHint);
server.on("/api/mission/:id/complete", HTTP_POST, handleMissionComplete);

// Gamification endpoints
server.on("/api/user/profile", HTTP_GET, handleUserProfile);
server.on("/api/user/achievements", HTTP_GET, handleAchievements);
server.on("/api/leaderboard/global", HTTP_GET, handleLeaderboardGlobal);
server.on("/api/leaderboard/campaign/:id", HTTP_GET, handleLeaderboardCampaign);

// Campaign endpoints
server.on("/api/campaign/list", HTTP_GET, handleCampaignList);
server.on("/api/campaign/:id/start", HTTP_POST, handleCampaignStart);
server.on("/api/campaign/:id/progress", HTTP_GET, handleCampaignProgress);

// Config endpoints
server.on("/api/admin/config", HTTP_GET, handleConfigGet);
server.on("/api/admin/config/difficulty", HTTP_POST, handleDifficultySet);
server.on("/api/admin/config/custom", HTTP_POST, handleCustomConfigSet);
server.on("/api/admin/config/save", HTTP_POST, handleConfigSave);
```

**Test Criteria:**
✅ All endpoints return valid JSON  
✅ Auth checks function  
✅ Defense checks trigger  
✅ Data persists across reboots  

---

### Phase 5: Defense Integration (Weeks 6-7)

**Files to Update:**
- `15_Defense.ino` — Config-driven costs

**Tasks:**
- [ ] Defense costs from config
- [ ] Auto-defense rules
- [ ] Difficulty affects DP regen

**Test Criteria:**
✅ Defense costs adjustable via config  
✅ Easy mode: 5 DP regen  
✅ Hard mode: 2 DP regen  
✅ Auto-rules execute  

---

### Phase 6: Testing & Validation (Weeks 7-8)

**Test Suite:**
- [ ] Easy mode: Vulnerabilities obvious, hints detailed
- [ ] Hard mode: Minimal hints, expensive defense
- [ ] Expert mode: No hints, hidden endpoints
- [ ] Campaign progression: Missions unlock sequentially
- [ ] Multiple paths: Each mission solvable 3+ ways

**Manual Test Cases:**
```cpp
TEST: Easy Mode Vulnerabilities Visible
- Set difficulty.preset = "easy"
- Check: vulnerability_exposure = 0.3
- Check: hint_enabled = true
- Check: /vuln/endpoints returns list

TEST: Hard Mode DP Regeneration Slow
- Set difficulty.preset = "hard"
- Check: DP regen = 2 per 5sec
- Wait 5 seconds, verify DP +2 only

TEST: Mission Unlocking
- Complete MISSION_01
- Check: MISSION_02 status = "available"
- Check: MISSION_03 status = "locked"
```

---

### Phase 7: Documentation (Weeks 8-9)

**Documents to Create:**
- `USER_GUIDE.md` — Getting started, tutorials
- `DEVELOPER_GUIDE.md` — Code structure, extension guide
- `CONFIG_REFERENCE.md` — All parameters explained
- `API_DOCUMENTATION.md` — Complete endpoint reference

---

### Phase 8: Polish & Optimization (Week 10)

**Tasks:**
- [ ] Memory profiling & optimization
- [ ] Performance tuning (targets: config parse <500ms, API response <200ms)
- [ ] Bug fixes
- [ ] Edge case handling
- [ ] Final security audit

---

## Testing & Validation ✅

### Unit Tests

```cpp
// Meter simulation
TEST(MeterSimulation, GeneratesRealisticData) {
  MeterSimulator meter(1, "electricity");
  float reading = meter.getReading();
  EXPECT_GT(reading, 0.0);
  EXPECT_LT(reading, 100.0);
}

// Config system
TEST(ConfigSystem, LoadsPresets) {
  configMgr.applyDifficultyPreset("easy");
  EXPECT_EQ(configMgr.getVulnExposure(), 0.3f);
  EXPECT_TRUE(configMgr.get("hint_enabled"));
}

// XP system
TEST(Gamification, AwardsXPCorrectly) {
  gamificationEngine.awardXP("user1", 50);
  EXPECT_EQ(gamificationEngine.getUserXP("user1"), 50);
}
```

### Security Tests

**IDOR Exploitation:**
```bash
# Test: Access other customer's consumption
curl "http://esp32/api/consumption/history?meter_id=5" \
  -H "Authorization: Bearer <customer1_token>"
# Expected: customer1 sees data from customer2's meter 5 (if VULN_IDOR enabled)
```

**Race Condition:**
```python
# Test: Generate double-billing
import requests, threading

def generate_bill():
    requests.post('http://esp32/api/billing/generate',
                  json={'meter_id': 5},
                  headers={'Authorization': f'Bearer {TOKEN}'})

threads = [threading.Thread(target=generate_bill) for _ in range(2)]
for t in threads: t.start()
for t in threads: t.join()

# Expected: 2 bills in database for same period (if VULN_RACE enabled)
```

**Meter Tampering:**
```bash
# Test: Inject false reading
curl -X POST "http://esp32/vuln/meter-tamper?meter_id=5&value=0.0"
# Expected: Meter 5 reading = 0.0, audit log entry created
```

### Defense Tests

```bash
# Test: IP block
> defense ip-block --ip 192.168.4.50 --duration 300
Cost: DP 15 | Current: DP=85/100, AP=9/10
Rule r-001 active

# Verify
> defense status
Active Rules: 1
r-001: DROP 192.168.4.50 (285s remaining)

# Test from blocked IP
curl http://esp32/api/meter/status
# Expected: Connection refused or timeout
```

### Integration Tests

```bash
# Test: Complete campaign flow
1. Start campaign "CAMPAIGN_GRID_AWAKENS"
2. Complete MISSION_WELCOME
3. Verify MISSION_READ_CONSUMPTION unlocked
4. Complete MISSION_READ_CONSUMPTION
5. Verify XP awarded (+15)
6. Check achievement "First Steps" unlocked
7. Verify leaderboard updated
```

---

## Success Metrics 🎯

**System should achieve:**

1. ✅ 100% JSON-driven configuration (no hardcoded values)
2. ✅ 4 distinct difficulty levels with measurable differences
3. ✅ 10+ campaigns with 5-10 missions each
4. ✅ Gamification with XP/levels/25+ achievements
5. ✅ 4 learning paths (Red/Blue/Admin/Architect)
6. ✅ Multiple solution paths for each mission (3+ per mission)
7. ✅ Suitable for beginners through experts
8. ✅ High engagement (avg session >45 min)
9. ✅ Performance targets met (API <200ms, config load <500ms)
10. ✅ Complete documentation (user + developer guides)

---

## Project File Structure 📁

```
ESP32-H4CK-GRD/
├── ESP32-SmartGrid.ino           # Main sketch
├── 00_Config.ino                 # Configuration manager ★NEW★
├── 01_Config.ino                 # Legacy config (merge into 00)
├── 02_WiFi.ino                   # WiFi setup
├── 03_WebServer.ino              # HTTP server + new endpoints ★ENHANCED★
├── 04_Auth.ino                   # Authentication & sessions
├── 05_Database.ino               # LittleFS JSON helpers
├── 06_Campaigns.ino              # Campaign engine ★NEW★
├── 07_Meters.ino                 # Meter simulation ★ENHANCED★
├── 08_Consumption.ino            # Consumption aggregation ★ENHANCED★
├── 09_Billing.ino                # Billing engine ★ENHANCED★
├── 10_Admin.ino                  # Admin endpoints
├── 11_Gamification.ino           # XP/achievements ★NEW★
├── 12_Vulnerabilities.ino        # Intentional vulns
├── 15_Defense.ino                # Defense system ★ENHANCED★
├── config.json                   # Master config ★NEW★
├── SMART_GRID_PLAN.md            # This document
├── README.md                     # Project overview
├── build.sh                      # Build script
├── upload.sh                     # Upload script
├── partitions.csv                # Partition table
└── data/                         # Web assets
    ├── config.json               # Config (copied to LittleFS)
    ├── campaigns.json            # Campaign definitions ★NEW★
    ├── achievements.json         # Achievement catalog ★NEW★
    ├── index.html
    ├── dashboard.html            # User dashboard ★ENHANCED★
    ├── consumption.html
    ├── billing.html
    ├── meter-details.html
    ├── admin.html
    ├── missions.html             # Mission dashboard ★NEW★
    ├── leaderboard.html          # Leaderboard view ★NEW★
    ├── config-panel.html         # Configuration UI ★NEW★
    ├── shell.html
    ├── css/
    │   └── style.css
    └── js/
        ├── auth-sync.js
        ├── mode.js
        ├── navbar.js
        ├── mission-controller.js ★NEW★
        └── config-controller.js  ★NEW★
```

---

## Conclusion 🎓

This comprehensive Smart Grid v2.0 design transforms the original concept into a full-featured educational security laboratory. Through configurable difficulty, multiple learning paths, gamification, and campaign-based progression, users from beginners to experts can engage with realistic smart grid security scenarios in an interactive, rewarding environment.

**Key Innovations:**
- **No hardcoding:** Everything driven by JSON configuration
- **Multiple paths:** Same goal achievable via different skill sets
- **Progressive difficulty:** Gradual learning curve with adaptive challenges
- **Gamified engagement:** XP, achievements, and leaderboards maintain motivation
- **Comprehensive coverage:** Red team, blue team, admin, and architect perspectives

**Ready for Implementation:** Phase 1 (Core Systems) can begin immediately.

---

**Version:** 2.0.0  
**Status:** ✅ Design Complete — Ready for Development  
**Next Action:** Begin Phase 1 implementation with `00_Config.ino` and `config.json`  
**Estimated Completion:** April 2026 (10 weeks)

---

**Author:** ESP32 Smart Grid Lab Team  
**Last Updated:** February 6, 2026

---

## Modern Web GUI Design ⚡

### Design Philosophy

**Clean Energy Dashboard Aesthetic**
- Modern utility company interface
- Real-time consumption visualization
- Customer-friendly billing transparency
- Utility admin power-user tools
- Mobile-first for customer apps
- Desktop-optimized for admin panels

### Technology Stack

**Zero-Dependency Core:**
- HTML5 + CSS3 (Grid + Flexbox)
- Vanilla JavaScript (ES6+)
- Canvas for meter animations
- WebSocket for live updates
- LocalStorage for preferences

**Optional Enhancements:**
- Chart.js for consumption graphs
- Feather Icons for UI elements

### Color Palette

```css
:root {
  /* Smart Grid Brand */
  --grid-primary: #1e88e5;      /* Energy blue */
  --grid-secondary: #43a047;    /* Green energy */
  --grid-accent: #ffb300;       /* Solar yellow */
  --grid-danger: #e53935;       /* Alert red */
  
  /* Consumption States */
  --consumption-low: #66bb6a;   /* Efficient */
  --consumption-medium: #ffa726; /* Moderate */
  --consumption-high: #ef5350;  /* Peak usage */
  
  /* Backgrounds */
  --bg-light: #fafafa;
  --bg-dark: #121212;
  --bg-card: #ffffff;
  --bg-card-dark: #1e1e1e;
  
  /* Text */
  --text-primary: #212121;
  --text-secondary: #757575;
  --text-primary-dark: #ffffff;
  --text-secondary-dark: #b0b0b0;
  
  /* Borders */
  --border-light: #e0e0e0;
  --border-dark: #333333;
}
```

### Component Library

#### 1. Smart Meter Card

```html
<div class="meter-card" data-meter-id="MTR-001" data-status="active">
  <div class="meter-header">
    <div class="meter-icon">
      <svg class="meter-spinning"><!-- animated meter icon --></svg>
    </div>
    <div class="meter-info">
      <h3>Smart Meter MTR-001</h3>
      <span class="meter-type">Electricity</span>
    </div>
    <span class="status-badge badge-green">Active</span>
  </div>
  
  <div class="meter-reading">
    <div class="reading-main">
      <span class="reading-value">1,247</span>
      <span class="reading-unit">kWh</span>
    </div>
    <div class="reading-rate">
      <span class="rate-icon">⚡</span>
      <span class="rate-value">2.3 kW</span>
      <span class="rate-label">current draw</span>
    </div>
  </div>
  
  <div class="meter-details">
    <div class="detail-item">
      <span class="detail-label">Last Updated</span>
      <span class="detail-value">2 min ago</span>
    </div>
    <div class="detail-item">
      <span class="detail-label">Daily Total</span>
      <span class="detail-value">32.4 kWh</span>
    </div>
    <div class="detail-item">
      <span class="detail-label">Cost Today</span>
      <span class="detail-value cost-high">€4.86</span>
    </div>
  </div>
  
  <div class="meter-actions">
    <button class="btn-link">View History</button>
    <button class="btn-link">Download Data</button>
  </div>
</div>
```

**Animations:**
- Spinning meter icon when actively consuming
- Pulse effect on current draw value
- Smooth transitions on data updates

#### 2. Consumption Timeline

```html
<div class="consumption-timeline">
  <div class="timeline-header">
    <h3>Consumption History</h3>
    <div class="timeline-controls">
      <button class="period-btn active" data-period="24h">24H</button>
      <button class="period-btn" data-period="7d">7D</button>
      <button class="period-btn" data-period="30d">30D</button>
      <button class="period-btn" data-period="1y">1Y</button>
    </div>
  </div>
  
  <div class="timeline-chart">
    <canvas id="consumption-chart" width="800" height="300"></canvas>
  </div>
  
  <div class="timeline-stats">
    <div class="stat-card">
      <span class="stat-label">Average Daily</span>
      <span class="stat-value">28.6 kWh</span>
      <span class="stat-trend trend-down">-12%</span>
    </div>
    <div class="stat-card">
      <span class="stat-label">Peak Draw</span>
      <span class="stat-value">4.8 kW</span>
      <span class="stat-time">18:30</span>
    </div>
    <div class="stat-card">
      <span class="stat-label">Off-Peak %</span>
      <span class="stat-value">62%</span>
      <span class="stat-trend trend-up">+5%</span>
    </div>
  </div>
</div>
```

**Chart Features:**
- Area graph with gradient fill
- Peak/off-peak zones (color-coded)
- Hover tooltips with precise values
- Responsive zoom and pan

#### 3. Billing Dashboard

```html
<div class="billing-dashboard">
  <div class="bill-summary">
    <div class="bill-amount">
      <span class="amount-label">Current Bill</span>
      <span class="amount-value">€87.32</span>
      <span class="amount-period">Jan 1 - Jan 31, 2026</span>
    </div>
    <div class="bill-status">
      <span class="status-label">Status:</span>
      <span class="status-badge badge-warning">Pending</span>
    </div>
  </div>
  
  <div class="bill-breakdown">
    <h4>Cost Breakdown</h4>
    <div class="breakdown-item">
      <span class="breakdown-label">Energy Charge</span>
      <span class="breakdown-value">€72.40</span>
      <span class="breakdown-detail">724 kWh × €0.10/kWh</span>
    </div>
    <div class="breakdown-item">
      <span class="breakdown-label">Peak Surcharge</span>
      <span class="breakdown-value">€8.52</span>
      <span class="breakdown-detail">142 kWh × €0.06/kWh</span>
    </div>
    <div class="breakdown-item">
      <span class="breakdown-label">Network Fee</span>
      <span class="breakdown-value">€4.20</span>
      <span class="breakdown-detail">Fixed monthly</span>
    </div>
    <div class="breakdown-item">
      <span class="breakdown-label">Tax (19%)</span>
      <span class="breakdown-value">€2.20</span>
    </div>
    <div class="breakdown-total">
      <span class="total-label">Total</span>
      <span class="total-value">€87.32</span>
    </div>
  </div>
  
  <div class="bill-actions">
    <button class="btn-primary">Pay Now</button>
    <button class="btn-secondary">Download PDF</button>
    <button class="btn-link">View Details</button>
  </div>
</div>
```

#### 4. Admin Control Panel

```html
<div class="admin-panel">
  <div class="panel-tabs">
    <button class="tab active" data-tab="tariffs">Tariffs</button>
    <button class="tab" data-tab="meters">Meters</button>
    <button class="tab" data-tab="customers">Customers</button>
    <button class="tab" data-tab="reports">Reports</button>
  </div>
  
  <div class="tab-content" data-tab-id="tariffs">
    <div class="tariff-editor">
      <h3>Tariff Configuration</h3>
      
      <div class="form-group">
        <label>Base Rate (€/kWh)</label>
        <input type="number" step="0.01" value="0.10" class="form-control">
      </div>
      
      <div class="form-group">
        <label>Peak Surcharge (€/kWh)</label>
        <input type="number" step="0.01" value="0.06" class="form-control">
      </div>
      
      <div class="form-group">
        <label>Peak Hours</label>
        <div class="time-range-selector">
          <input type="time" value="18:00" class="form-control">
          <span>to</span>
          <input type="time" value="22:00" class="form-control">
        </div>
      </div>
      
      <div class="form-group">
        <label>Off-Peak Discount (%)</label>
        <input type="number" step="1" value="15" class="form-control">
      </div>
      
      <div class="form-actions">
        <button class="btn-primary">Save Changes</button>
        <button class="btn-secondary">Reset</button>
      </div>
    </div>
    
    <div class="tariff-preview">
      <h4>Price Preview</h4>
      <div class="preview-chart">
        <!-- 24-hour price visualization -->
      </div>
    </div>
  </div>
</div>
```

#### 5. Meter Calibration Interface

```html
<div class="calibration-panel">
  <div class="panel-header">
    <h3>Meter Calibration: MTR-042</h3>
    <span class="calibration-status status-warning">Calibration Due</span>
  </div>
  
  <div class="calibration-form">
    <div class="form-section">
      <h4>Reference Measurement</h4>
      <div class="form-row">
        <label>Reference Value (kWh)</label>
        <input type="number" step="0.001" placeholder="Enter certified reading">
      </div>
      <div class="form-row">
        <label>Meter Reading (kWh)</label>
        <input type="number" step="0.001" value="1247.832" readonly>
      </div>
      <div class="form-row">
        <label>Deviation</label>
        <span class="deviation-value deviation-high">+2.3%</span>
      </div>
    </div>
    
    <div class="form-section">
      <h4>Calibration Factors</h4>
      <div class="form-row">
        <label>Voltage Factor</label>
        <input type="number" step="0.0001" value="1.0000">
      </div>
      <div class="form-row">
        <label>Current Factor</label>
        <input type="number" step="0.0001" value="0.9980">
      </div>
      <div class="form-row">
        <label>Phase Correction (°)</label>
        <input type="number" step="0.01" value="0.00">
      </div>
    </div>
    
    <div class="form-actions">
      <button class="btn-primary">Apply Calibration</button>
      <button class="btn-secondary">Test</button>
      <button class="btn-danger">Reset to Factory</button>
    </div>
  </div>
</div>
```

#### 6. Alert & Notification System

```html
<div class="notification-panel">
  <div class="notification-header">
    <h3>Alerts & Notifications</h3>
    <button class="mark-all-read">Mark All Read</button>
  </div>
  
  <div class="notification-list">
    <div class="notification-item notification-warning unread">
      <div class="notification-icon">⚠️</div>
      <div class="notification-content">
        <div class="notification-title">High Consumption Detected</div>
        <div class="notification-message">
          Meter MTR-001 exceeded 5 kW draw for 2 hours
        </div>
        <div class="notification-meta">
          <span class="notification-time">15 min ago</span>
          <span class="notification-meter">MTR-001</span>
        </div>
      </div>
      <button class="notification-close">×</button>
    </div>
    
    <div class="notification-item notification-info">
      <div class="notification-icon">ℹ️</div>
      <div class="notification-content">
        <div class="notification-title">Bill Ready</div>
        <div class="notification-message">
          Your January bill (€87.32) is ready for review
        </div>
        <div class="notification-meta">
          <span class="notification-time">2 hours ago</span>
          <a href="/billing.html" class="notification-action">View Bill</a>
        </div>
      </div>
    </div>
    
    <div class="notification-item notification-success">
      <div class="notification-icon">✓</div>
      <div class="notification-content">
        <div class="notification-title">Payment Received</div>
        <div class="notification-message">
          December bill payment confirmed (€82.15)
        </div>
        <div class="notification-meta">
          <span class="notification-time">2 days ago</span>
        </div>
      </div>
    </div>
  </div>
</div>
```

### Page Layouts

#### Customer Dashboard (`dashboard.html`)

**Desktop Layout:**
```
+----------------------------------------------------------+
| SmartGrid Energy      [🔔 3]  [John Doe ▼]  [⚙️]  [🌙]    |
+----------------------------------------------------------+
|                                                           |
|  Welcome back, John!                                     |
|  Current Usage: 2.3 kW    Today: 32.4 kWh    €4.86      |
|                                                           |
|  +---------------------------------------------------+   |
|  | Consumption Timeline (24H)                        |   |
|  |                                                   |   |
|  |  [Area chart showing hourly consumption]          |   |
|  |  Peak zones highlighted in yellow                 |   |
|  |                                                   |   |
|  +---------------------------------------------------+   |
|                                                           |
|  +-----------------------+  +--------------------------+  |
|  | Smart Meter MTR-001   |  | Current Bill             |  |
|  | [spinning icon]       |  | €87.32                   |  |
|  | 1,247 kWh             |  | Jan 1 - 31, 2026         |  |
|  | 2.3 kW now            |  | Status: Pending          |  |
|  | [View Details]        |  | [Pay Now] [View Breakdown]|
|  +-----------------------+  +--------------------------+  |
|                                                           |
|  +-----------------------+  +--------------------------+  |
|  | Usage Insights        |  | Recommendations          |  |
|  | • Peak usage: 18:30   |  | • Use off-peak for       |  |
|  | • Off-peak: 62%       |  |   laundry (-15%)         |  |
|  | • vs last month: -12% |  | • Consider solar panels  |  |
|  +-----------------------+  +--------------------------+  |
+----------------------------------------------------------+
```

#### Consumption History (`consumption.html`)

```
+----------------------------------------------------------+
| Consumption History                          [Back]      |
+----------------------------------------------------------+
|                                                           |
|  Meter: MTR-001 (Electricity)    Period: [7D ▼]         |
|                                                           |
|  +---------------------------------------------------+   |
|  | Consumption Chart (Last 7 Days)                   |   |
|  |                                                   |   |
|  |  [Detailed line/area chart with zoom controls]    |   |
|  |                                                   |   |
|  +---------------------------------------------------+   |
|                                                           |
|  Statistics:              Comparison:                    |
|  ┌──────────────────┐    ┌───────────────────────┐      |
|  │ Total: 198.5 kWh │    │ vs Last Week: -8.2%   │      |
|  │ Daily Avg: 28.4  │    │ vs Last Month: -12%   │      |
|  │ Peak: 4.8 kW     │    │ vs Last Year: +3%     │      |
|  │ Off-Peak: 65%    │    │ Rank: Top 20%         │      |
|  └──────────────────┘    └───────────────────────┘      |
|                                                           |
|  Hourly Breakdown:                                       |
|  ┌─────────────────────────────────────────────────┐    |
|  │ Hour | kWh  | Cost  | Peak? | Rate    | Compare │    |
|  ├─────────────────────────────────────────────────┤    |
|  │ 00   | 1.2  | €0.12 | No    | €0.10/k | -5%     │    |
|  │ 01   | 1.1  | €0.11 | No    | €0.10/k | -8%     │    |
|  │ ...                                              │    |
|  │ 18   | 4.2  | €0.67 | Yes   | €0.16/k | +12%    │    |
|  │ 19   | 4.8  | €0.77 | Yes   | €0.16/k | +18%    │    |
|  └─────────────────────────────────────────────────┘    |
|                                                           |
|  [Export CSV]  [Download PDF]  [Share Report]           |
+----------------------------------------------------------+
```

#### Admin Dashboard (`admin.html`)

```
+----------------------------------------------------------+
| Admin Control Panel                [Utility Energy Inc.] |
+----------------------------------------------------------+
|                                                           |
|  [Tariffs] [Meters] [Customers] [Reports] [Defense] [Logs]
|                                                           |
|  System Overview:                                        |
|  Active Meters: 1,247    Online: 1,198 (96%)            |
|  Total Load: 2.8 MW      Peak Today: 3.2 MW (14:30)     |
|  Revenue Today: €4,286   Revenue MTD: €128,450          |
|                                                           |
|  +---------------------------------------------------+   |
|  | Meter Status Map                                  |   |
|  |                                                   |   |
|  |  [Heatmap showing consumption by region]          |   |
|  |  Green = normal, Yellow = high, Red = alert       |   |
|  |                                                   |   |
|  +---------------------------------------------------+   |
|                                                           |
|  Recent Activity:                Alerts:                 |
|  ┌────────────────────┐         ┌────────────────────┐  |
|  │ MTR-042: Calib due │         │ 3 meters offline   │  |
|  │ Bill-1247: Generated│         │ 2 high consumption │  |
|  │ Customer#52: Paid  │         │ 1 tamper suspected │  |
|  └────────────────────┘         └────────────────────┘  |
|                                                           |
|  Quick Actions:                                          |
|  [Generate Bills] [Export Data] [Run Diagnostics]       |
+----------------------------------------------------------+
```

### JavaScript Architecture

#### Real-Time Meter Updates

```javascript
class MeterDashboard {
  constructor() {
    this.meters = new Map();
    this.ws = null;
    this.updateInterval = 5000; // 5s fallback
    this.init();
  }
  
  init() {
    this.loadMeters();
    this.connectWebSocket();
    this.setupEventListeners();
    this.startAnimations();
  }
  
  async loadMeters() {
    const response = await fetch('/api/meters/list');
    const data = await response.json();
    
    data.meters.forEach(meter => {
      this.meters.set(meter.id, meter);
      this.renderMeterCard(meter);
    });
  }
  
  connectWebSocket() {
    this.ws = new WebSocket(`ws://${location.host}/ws/meters`);
    
    this.ws.onmessage = (event) => {
      const update = JSON.parse(event.data);
      this.updateMeter(update);
    };
    
    this.ws.onclose = () => {
      setTimeout(() => this.connectWebSocket(), 5000);
    };
  }
  
  updateMeter(data) {
    const meter = this.meters.get(data.meter_id);
    if (!meter) return;
    
    // Update internal state
    Object.assign(meter, data);
    
    // Update UI
    const card = document.querySelector(`[data-meter-id="${data.meter_id}"]`);
    if (card) {
      this.updateMeterCard(card, data);
      this.animateMeterIcon(card, data.current_draw > 0);
    }
    
    // Update charts if visible
    if (this.activeChart === data.meter_id) {
      this.appendChartData(data);
    }
  }
  
  animateMeterIcon(card, isActive) {
    const icon = card.querySelector('.meter-spinning');
    if (isActive) {
      icon.classList.add('spinning');
    } else {
      icon.classList.remove('spinning');
    }
  }
}
```

#### Consumption Chart

```javascript
class ConsumptionChart {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext('2d');
    this.data = [];
    this.period = '24h';
    this.peakZones = [];
  }
  
  async loadData(meterId, period) {
    const response = await fetch(
      `/api/consumption/history?meter_id=${meterId}&period=${period}`
    );
    const data = await response.json();
    
    this.data = data.timeseries;
    this.peakZones = data.peak_zones;
    this.render();
  }
  
  render() {
    const width = this.canvas.width;
    const height = this.canvas.height;
    const padding = 40;
    
    // Clear canvas
    this.ctx.clearRect(0, 0, width, height);
    
    // Draw peak zones (background)
    this.ctx.fillStyle = 'rgba(255, 179, 0, 0.1)';
    this.peakZones.forEach(zone => {
      const x1 = this.timeToX(zone.start);
      const x2 = this.timeToX(zone.end);
      this.ctx.fillRect(x1, padding, x2 - x1, height - 2 * padding);
    });
    
    // Draw grid
    this.drawGrid();
    
    // Draw area fill
    this.ctx.beginPath();
    this.ctx.moveTo(padding, height - padding);
    
    this.data.forEach((point, i) => {
      const x = this.timeToX(point.timestamp);
      const y = this.valueToY(point.value);
      
      if (i === 0) {
        this.ctx.lineTo(x, y);
      } else {
        this.ctx.lineTo(x, y);
      }
    });
    
    this.ctx.lineTo(width - padding, height - padding);
    this.ctx.closePath();
    
    // Gradient fill
    const gradient = this.ctx.createLinearGradient(0, 0, 0, height);
    gradient.addColorStop(0, 'rgba(30, 136, 229, 0.3)');
    gradient.addColorStop(1, 'rgba(30, 136, 229, 0)');
    this.ctx.fillStyle = gradient;
    this.ctx.fill();
    
    // Draw line
    this.ctx.beginPath();
    this.data.forEach((point, i) => {
      const x = this.timeToX(point.timestamp);
      const y = this.valueToY(point.value);
      
      if (i === 0) {
        this.ctx.moveTo(x, y);
      } else {
        this.ctx.lineTo(x, y);
      }
    });
    
    this.ctx.strokeStyle = '#1e88e5';
    this.ctx.lineWidth = 2;
    this.ctx.stroke();
    
    // Draw points
    this.data.forEach(point => {
      const x = this.timeToX(point.timestamp);
      const y = this.valueToY(point.value);
      
      this.ctx.beginPath();
      this.ctx.arc(x, y, 3, 0, 2 * Math.PI);
      this.ctx.fillStyle = '#1e88e5';
      this.ctx.fill();
    });
  }
  
  timeToX(timestamp) {
    const range = this.data[this.data.length - 1].timestamp - this.data[0].timestamp;
    const offset = timestamp - this.data[0].timestamp;
    return 40 + (offset / range) * (this.canvas.width - 80);
  }
  
  valueToY(value) {
    const max = Math.max(...this.data.map(d => d.value));
    return this.canvas.height - 40 - (value / max) * (this.canvas.height - 80);
  }
}
```

### Responsive Design

```css
/* Desktop: 1920x1080 */
@media (min-width: 1920px) {
  .dashboard-grid { grid-template-columns: repeat(3, 1fr); }
  .meter-card { max-width: 400px; }
  .consumption-timeline { height: 500px; }
}

/* Laptop: 1366x768 */
@media (max-width: 1919px) {
  .dashboard-grid { grid-template-columns: repeat(2, 1fr); }
  .meter-card { max-width: none; }
}

/* Tablet: 768x1024 (Portrait) */
@media (max-width: 1024px) {
  .dashboard-grid { grid-template-columns: 1fr; }
  .admin-panel .panel-tabs { overflow-x: auto; }
  .billing-dashboard { flex-direction: column; }
}

/* Mobile: 375x667 */
@media (max-width: 768px) {
  .meter-card {
    padding: 15px;
    font-size: 14px;
  }
  
  .consumption-timeline {
    height: 300px;
  }
  
  .timeline-stats {
    flex-direction: column;
  }
  
  .admin-panel {
    padding: 10px;
  }
  
  .notification-panel {
    position: fixed;
    bottom: 0;
    left: 0;
    right: 0;
    max-height: 50vh;
    overflow-y: auto;
  }
}
```

### Accessibility

**WCAG 2.1 AA Compliance:**
- High contrast ratios (≥ 4.5:1)
- Focus indicators on all controls
- Keyboard navigation
- ARIA labels and live regions
- Screen reader friendly

**Keyboard Shortcuts:**
- `Ctrl+D`: Dashboard
- `Ctrl+C`: Consumption
- `Ctrl+B`: Billing
- `Ctrl+M`: Meters (admin)
- `Ctrl+A`: Admin panel
- `Esc`: Close modal

---
