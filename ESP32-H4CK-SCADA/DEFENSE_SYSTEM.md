# ESP32-H4CK Defense System Guide

## Overview 🎮

The Defense System module simulates realistic security measures (IP blocking, rate limiting, session management) with resource constraints inspired by Red/Blue Team exercises. All enforcement happens at the **application level** — no kernel or network stack modifications.

This system teaches:
- Resource-constrained decision making
- Cost/benefit analysis of security measures
- Realistic system administration commands (iptables, tc, session)
- Side-effects and trade-offs of defensive actions
- Multi-device coordination (future: serial/UDP/MQTT sync)

> ⚠️ **Important**: Commands simulate application-layer enforcement only. They do NOT modify OS kernel, firewall rules, or network stack. Perfect for safe educational labs.

---

## Architecture 🔧

### Resource System

**Defense Points (DP)**: Cost to activate defensive measures  
**Action Points (AP)**: Limits simultaneous actions per time window  
**Stability Score (SS)**: System stability impact (aggressive defenses reduce it)

Defaults:
- Max DP: 100 (regenerates +2 every 5 seconds)
- Max AP: 10 (regenerates +1 every 5 seconds)
- Max Stability: 100 (regenerates +1 every 5 seconds)

### Defense Types

| Defense | DP Cost | AP Cost | SS Impact | Cooldown | Duration |
|---------|---------|---------|-----------|----------|----------|
| IP Block | 15 | 1 | -5 | 60s | Configurable (default 30s) |
| Rate Limit | 10 | 1 | -3 | 30s | Configurable (default 60s) |
| Session Reset | 25 | 1 | -10 | 90s | Immediate |
| Mistrust Mode | 40 | 2 | -20 | 600s | 300s |

### Enforcement Points

1. **HTTP/HTTPS Server** (`03_WebServer.ino`)
   - Checks IP blocks and rate limits at start of each request
   - Returns 403 (blocked) or 429 (rate limited)

2. **WebSocket Shell** (`07_WebSocket.ino`)
   - Validates IP before command execution
   - Sends error message and logs violation

3. **Telnet Service** (`08_Telnet.ino`)
   - Checks IP on new connection attempts
   - Closes connection immediately if blocked

---

## Command Reference 📖

### IP Blocking (iptables-like)

**Block an IP address:**
```bash
iptables -A INPUT -s 192.168.4.100 -j DROP --duration 30
```
- Blocks IP `192.168.4.100` for 30 seconds
- Cost: DP=15, AP=1
- Returns JSON: `{"status":"ok","id":"...","result":{"rule_id":"r-001","expires":...}}`

**Remove IP block:**
```bash
iptables -D INPUT -s 192.168.4.100 -j DROP
```
- Immediately removes block
- No cost (defensive removal)

**List active blocks:**
```bash
iptables -L
```
Output example:
```
Chain INPUT (policy ACCEPT)
target     source               expires
DROP       192.168.4.100        23s
DROP       192.168.4.101        expired
```

**With custom request ID (idempotency):**
```bash
iptables -A INPUT -s 192.168.4.100 -j DROP --duration 30 --id req-001
```
- Duplicate `--id` returns cached response without re-executing

---

### Rate Limiting (tc-like)

**Apply rate limit:**
```bash
tc qdisc add rate-limit --src 192.168.4.0/24 --duration 60
```
- Limits requests from subnet for 60 seconds
- Cost: DP=10, AP=1
- Default: max 10 requests per 60-second window

**Remove rate limit:**
```bash
tc qdisc del rate-limit --src 192.168.4.0/24
```

**Show active rate limits:**
```bash
tc qdisc show
```
Output:
```
qdisc rate-limit:
  src: 192.168.4.0/24 (rule r-002)
  src: 0.0.0.0/0 (rule r-003)
```

**Notes:**
- `--src 0.0.0.0/0` applies globally
- Rate limiting tracks per IP internally

---

### Session Management

**Reset all sessions from an IP:**
```bash
session reset --ip 192.168.4.101
```
- Forcibly disconnects all active sessions
- Cost: DP=25, AP=1
- Returns: `{"status":"ok","result":{"sessions_removed":2}}`

**Optional reason parameter:**
```bash
session reset --ip 192.168.4.101 --reason "suspected breach"
```

---

### Status & Configuration

**Show current resources and active rules:**
```bash
defense status
```
JSON output:
```json
{
  "status": "ok",
  "result": {
    "resources": {
      "dp": 85,
      "dp_max": 100,
      "ap": 9,
      "ap_max": 10,
      "stability": 92,
      "stability_max": 100
    },
    "active_rules": 2,
    "rules": [
      {"id": "r-001", "type": "1", "target": "192.168.4.100", "expires": 1738502450},
      {"id": "r-002", "type": "2", "target": "192.168.4.0/24", "expires": 1738502470}
    ]
  }
}
```

**Show configuration:**
```bash
defense config show
```

**Update max resources:**
```bash
defense config set dp=150 ap=15 stability=100
```
- Changes persist across reboots (saved to Preferences)
- Current DP/AP/SS reset to new max

---

## Usage Scenarios 🎯

### Scenario 1: Brute Force Defense

**Situation**: Attacker at `192.168.4.105` attempts password brute force on `/api/login`

**Blue Team Response:**
1. Monitor logs via serial: `[HTTP] GET /api/login from 192.168.4.105` (repeated)
2. Check resources: `defense status`
3. Block attacker: `iptables -A INPUT -s 192.168.4.105 -j DROP --duration 120`
4. Verify: `iptables -L`

**Result**: 
- Attacker blocked for 2 minutes
- Cost: DP=15, AP=1
- Side effect: Stability -5 (acceptable for critical threat)

---

### Scenario 2: DDoS Mitigation

**Situation**: Multiple IPs flooding server

**Blue Team Response:**
1. Apply global rate limit: `tc qdisc add rate-limit --src 0.0.0.0/0 --duration 300`
2. Cost: DP=10, AP=1
3. Monitor: `tc qdisc show`

**Result**:
- All IPs limited to 10 req/min
- Legitimate users may experience slower service (trade-off)
- Stability impact: -3

**Alternative** (targeted):
```bash
tc qdisc add rate-limit --src 192.168.4.0/24 --duration 300
```

---

### Scenario 3: Suspected Session Hijacking

**Situation**: Unusual activity from IP `192.168.4.50`

**Blue Team Response:**
1. Reset sessions: `session reset --ip 192.168.4.50 --reason "suspicious activity"`
2. Block IP temporarily: `iptables -A INPUT -s 192.168.4.50 -j DROP --duration 60`
3. Total cost: DP=40 (25+15), AP=2

**Result**:
- All sessions from IP terminated
- IP blocked for investigation period
- High stability impact (-15 total)

---

### Scenario 4: Resource Exhaustion

**Situation**: Red Team forces Blue to exhaust all DP

**Blue observes:**
```bash
defense status
# Shows: dp=5, ap=1, stability=70
```

**Attempted defense:**
```bash
iptables -A INPUT -s 192.168.4.200 -j DROP --duration 30
# Returns: {"status":"error","error":{"code":403,"msg":"insufficient resources"}}
```

**Learning**:
- Must wait for DP regeneration (+2 every 5 sec)
- Cannot defend everything simultaneously
- Strategic choices required

---

## JSON API (Alternative) 📡

Commands can also be sent as JSON (single-line, newline-terminated):

**IP Block (JSON):**
```json
{"cmd":"iptables","op":"add","chain":"INPUT","src":"192.168.4.100","target":"DROP","duration":30,"cost":{"dp":15,"ap":1},"id":"req-123"}
```

**Rate Limit (JSON):**
```json
{"cmd":"tc","op":"add","type":"rate-limit","src":"192.168.4.0/24","duration":60,"cost":{"dp":10},"id":"req-124"}
```

**Session Reset (JSON):**
```json
{"cmd":"session","op":"reset","ip":"192.168.4.101","id":"req-125"}
```

**Response format:**
```json
{"status":"ok","id":"req-123","result":{"rule_id":"r-005","expires":1738502600}}
```

**Error format:**
```json
{"status":"error","id":"req-123","error":{"code":403,"msg":"insufficient resources"}}
```

---

## Multi-Device Coordination (Future) 🌐

Currently in planning; designed for:

**Option A: Serial Relay**
- Connect multiple ESP32s via UART
- Forward JSON commands between devices
- Coordinated defense across lab

**Option B: UDP Broadcast**
- Broadcast defense events on local network
- Fast propagation, no broker needed
- Loss-tolerant for non-critical sync

**Option C: MQTT (optional)**
- Reliable delivery with QoS
- Requires MQTT broker setup
- Best for production-like scenarios

---

## Troubleshooting 🔍

### "Cooldown active" error
```
{"status":"error","error":{"code":429,"msg":"cooldown active"}}
```
**Solution**: Wait for cooldown period to expire. Check `defense status` for last activation time.

### "Insufficient resources" error
```
{"status":"error","error":{"code":403,"msg":"insufficient resources"}}
```
**Solution**: 
- Wait for DP/AP regeneration (automatic)
- Or adjust limits: `defense config set dp=200`

### Rule not blocking traffic
**Check**:
1. Verify rule is active: `iptables -L` or `defense status`
2. Check if rule expired (duration elapsed)
3. Ensure IP address matches exactly (case-sensitive)
4. Serial logs show `[DEFENSE] Blocked request from ...` when enforcing

### Rate limit not working
**Verify**:
- Rule covers correct IP/range
- Request counter increments (internal tracking)
- Window hasn't expired (60 sec default)

---

## Educational Use Cases 📚

### 1. Red Team Objectives
- Exhaust Blue's DP/AP resources
- Trigger false positives (block legitimate IPs)
- Exploit cooldown windows
- Force stability degradation

### 2. Blue Team Objectives
- Maintain service availability
- Detect attacks early
- Minimize false positives
- Balance cost vs. benefit

### 3. Discussion Topics
- When to use IP blocks vs. rate limits?
- Cost of aggressive defense (stability impact)
- Idempotency in distributed systems
- Application vs. network-layer enforcement
- Real-world analogies (AWS WAF, Cloudflare, iptables)

---

## Advanced Configuration ⚙️

### Custom Costs & Cooldowns

Edit via serial:
```bash
defense config set dp=200 ap=20 stability=150
```

Or modify `15_Defense.ino` defaults:
```cpp
defenseConfig.ipblock_cost.dp = preferences.getInt("ipb_dp", 20);  // Changed from 15
defenseConfig.ipblock_cooldown = preferences.getInt("ipb_cd", 30); // Changed from 60
```

### Persistence

All config saved to ESP32 `Preferences` namespace `"defense"`:
- Survives reboots
- Per-device storage
- Reset via: `preferences.begin("defense", false); preferences.clear();`

---

## Web GUI Design 🎨

### Design Philosophy

**Modern, Clean, Professional**
- Minimalist industrial aesthetic inspired by modern SCADA systems
- Dark mode by default (operator-friendly for 24/7 monitoring)
- High contrast for critical information
- Smooth animations (60fps transitions)
- Fully responsive (desktop → tablet → mobile)

### Technology Stack

**No External Dependencies** (ESP32-hosted only):
- Pure HTML5 + CSS3 (Grid + Flexbox)
- Vanilla JavaScript (ES6+)
- CSS Variables for theming
- LocalStorage for preferences
- EventSource for real-time updates

**Optional CDN Fallback** (when internet available):
- Chart.js for timeseries graphs
- Feather Icons for consistent iconography

### Color Palette

**Dark Theme (Default):**
```css
:root {
  --bg-primary: #0f1419;
  --bg-secondary: #1a1f27;
  --bg-tertiary: #252b36;
  
  --text-primary: #e6edf3;
  --text-secondary: #8b949e;
  --text-muted: #6e7681;
  
  --accent-blue: #2f81f7;
  --accent-green: #3fb950;
  --accent-yellow: #d29922;
  --accent-red: #f85149;
  --accent-purple: #a371f7;
  
  --border-default: #30363d;
  --shadow-default: rgba(0,0,0,0.3);
}
```

**Light Theme:**
```css
.light-mode {
  --bg-primary: #ffffff;
  --bg-secondary: #f6f8fa;
  --bg-tertiary: #eaeef2;
  
  --text-primary: #1f2328;
  --text-secondary: #656d76;
  --text-muted: #8c959f;
  
  --border-default: #d0d7de;
  --shadow-default: rgba(140,149,159,0.15);
}
```

### Component Library

#### 1. Status Cards
```html
<div class="status-card" data-status="active">
  <div class="status-header">
    <h3>Defense Points</h3>
    <span class="status-indicator"></span>
  </div>
  <div class="status-value">85<span class="unit">/100</span></div>
  <div class="status-trend">
    <span class="trend-icon">↑</span>
    <span class="trend-text">+2 / 5sec</span>
  </div>
</div>
```

**States:** `active`, `warning`, `critical`, `disabled`

#### 2. Command Terminal
```html
<div class="terminal">
  <div class="terminal-header">
    <span class="terminal-title">Defense Shell</span>
    <button class="terminal-clear">Clear</button>
  </div>
  <div class="terminal-output" id="terminal-output">
    <div class="terminal-line">
      <span class="prompt">esp32@defense:~$</span>
      <span class="command">defense status</span>
    </div>
    <div class="terminal-line terminal-response">
      {"status":"ok","result":{"dp":85,"ap":9}}
    </div>
  </div>
  <div class="terminal-input">
    <span class="prompt">esp32@defense:~$</span>
    <input type="text" id="command-input" autocomplete="off">
  </div>
</div>
```

**Features:**
- Syntax highlighting for JSON responses
- Command history (↑/↓ arrows)
- Auto-complete for known commands
- Copy button for responses

#### 3. Rule Table
```html
<table class="data-table">
  <thead>
    <tr>
      <th>Rule ID</th>
      <th>Type</th>
      <th>Target</th>
      <th>Expires</th>
      <th>Actions</th>
    </tr>
  </thead>
  <tbody>
    <tr class="rule-row" data-status="active">
      <td><code>r-001</code></td>
      <td><span class="badge badge-red">IP Block</span></td>
      <td>192.168.4.100</td>
      <td class="countdown" data-expires="1738502450">23s</td>
      <td>
        <button class="btn-icon" title="Remove">
          <svg><!-- trash icon --></svg>
        </button>
      </td>
    </tr>
  </tbody>
</table>
```

**Features:**
- Real-time countdown updates
- Status indicators (active/expired/cooldown)
- Inline actions (remove, extend)
- Sortable columns

#### 4. Resource Meters
```html
<div class="resource-meter">
  <div class="meter-header">
    <span class="meter-label">Defense Points</span>
    <span class="meter-value">85 / 100</span>
  </div>
  <div class="meter-bar">
    <div class="meter-fill" style="width: 85%;" data-status="healthy"></div>
  </div>
  <div class="meter-regen">
    <span>Regeneration: +2 every 5s</span>
  </div>
</div>
```

**Status Thresholds:**
- `healthy`: > 70%
- `warning`: 30-70%
- `critical`: < 30%

#### 5. Alert Toasts
```html
<div class="toast toast-error" role="alert">
  <svg class="toast-icon"><!-- alert icon --></svg>
  <div class="toast-content">
    <div class="toast-title">Defense Activated</div>
    <div class="toast-message">IP 192.168.4.100 blocked for 120s</div>
  </div>
  <button class="toast-close">×</button>
</div>
```

**Variants:** `success`, `warning`, `error`, `info`

### Page Layouts

#### Defense Dashboard (`defense.html`)

**Header:**
```
+----------------------------------------------------------+
| 🛡️ Defense System          [Dark Mode]  [Settings]  [Help] |
+----------------------------------------------------------+
```

**Main Grid:**
```
+------------------------+------------------------+
| DP: 85/100  [████▓▓▓▓] | AP: 9/10   [████████▓] |
| Regen: +2/5s           | Regen: +1/5s           |
+------------------------+------------------------+
| SS: 92/100  [████████░] |  Active Rules: 2      |
| Regen: +1/5s           |  Last Action: 12s ago |
+------------------------+------------------------+

+----------------------------------------------------------+
| Active Defense Rules                        [Clear All] |
+----------------------------------------------------------+
| ID    | Type        | Target          | Expires | Action |
| r-001 | IP Block    | 192.168.4.100   | 23s     | [×]    |
| r-002 | Rate Limit  | 192.168.4.0/24  | 47s     | [×]    |
+----------------------------------------------------------+

+----------------------------------------------------------+
| Defense Shell                                  [Clear]  |
+----------------------------------------------------------+
| esp32@defense:~$ defense status                         |
| {"status":"ok","result":{"dp":85,"ap":9,"stability":92}}|
| esp32@defense:~$ _                                      |
+----------------------------------------------------------+

+----------------------------------------------------------+
| Quick Actions                                           |
+----------------------------------------------------------+
| [Block IP]  [Rate Limit]  [Reset Session]  [Config]    |
+----------------------------------------------------------+
```

#### Mobile Layout (< 768px)

**Stack vertically:**
```
+----------------------+
| DP: 85/100           |
| [████▓▓▓▓▓]          |
+----------------------+
| AP: 9/10             |
| [████████▓]          |
+----------------------+
| SS: 92/100           |
| [████████░]          |
+----------------------+
| Active Rules: 2      |
| [View Rules ▼]       |
+----------------------+
```

### JavaScript Patterns

#### Real-Time Updates

```javascript
// EventSource for server-sent events
const defenseEvents = new EventSource('/api/defense/stream');

defenseEvents.onmessage = (e) => {
  const data = JSON.parse(e.data);
  updateResourceMeters(data.resources);
  updateActiveRules(data.rules);
  updateStabilityScore(data.stability);
};

// Fallback: polling every 2s
if (!window.EventSource) {
  setInterval(async () => {
    const response = await fetch('/api/defense/status');
    const data = await response.json();
    updateDashboard(data);
  }, 2000);
}
```

#### Command Execution

```javascript
async function executeCommand(cmd) {
  const terminal = document.getElementById('terminal-output');
  
  // Echo command
  appendTerminalLine(`<span class="prompt">$</span> ${cmd}`);
  
  try {
    const response = await fetch('/api/shell', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ cmd })
    });
    
    const result = await response.json();
    
    // Pretty-print JSON
    const formatted = JSON.stringify(result, null, 2);
    appendTerminalLine(
      `<pre class="json-response">${syntaxHighlight(formatted)}</pre>`
    );
    
    // Update UI based on result
    if (result.status === 'ok') {
      showToast('success', 'Command executed');
      refreshDefenseStatus();
    } else {
      showToast('error', result.error.msg);
    }
  } catch (err) {
    appendTerminalLine(`<span class="error">Error: ${err.message}</span>`);
  }
}
```

#### Countdown Timers

```javascript
function startCountdowns() {
  document.querySelectorAll('.countdown[data-expires]').forEach(el => {
    const expiresAt = parseInt(el.dataset.expires);
    
    const update = () => {
      const remaining = expiresAt - Math.floor(Date.now() / 1000);
      
      if (remaining <= 0) {
        el.textContent = 'expired';
        el.classList.add('expired');
      } else {
        el.textContent = formatDuration(remaining);
        
        // Warning at < 10s
        if (remaining < 10) {
          el.classList.add('warning');
        }
      }
    };
    
    update();
    setInterval(update, 1000);
  });
}
```

### Accessibility Features

**Keyboard Navigation:**
- Tab through all interactive elements
- `Ctrl+L`: Focus command input
- `Ctrl+K`: Clear terminal
- `Esc`: Close modals/toasts
- `↑/↓`: Command history

**Screen Readers:**
- ARIA labels on all controls
- Live regions for status updates
- Semantic HTML (`<main>`, `<nav>`, `<article>`)

**High Contrast Mode:**
- Respects `prefers-contrast: high`
- Thicker borders, larger text

**Reduced Motion:**
- Respects `prefers-reduced-motion`
- Disables animations, uses instant transitions

### Performance Optimizations

**Lazy Loading:**
- Load chart library only when graphs visible
- Defer non-critical CSS

**Efficient Updates:**
- Virtual DOM for terminal output (max 500 lines)
- Debounced input handlers
- RequestAnimationFrame for smooth animations

**Memory Management:**
- Auto-trim logs > 1000 entries
- Clear old countdown intervals
- Unsubscribe EventSource on page leave

### Example CSS (Excerpt)

```css
/* Status Card */
.status-card {
  background: var(--bg-secondary);
  border: 1px solid var(--border-default);
  border-radius: 8px;
  padding: 20px;
  box-shadow: var(--shadow-default);
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}

.status-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px var(--shadow-default);
}

.status-card[data-status="warning"] {
  border-left: 4px solid var(--accent-yellow);
}

.status-card[data-status="critical"] {
  border-left: 4px solid var(--accent-red);
  animation: pulse 2s ease-in-out infinite;
}

/* Terminal */
.terminal {
  background: #0d1117;
  border-radius: 6px;
  overflow: hidden;
  font-family: 'Consolas', 'Monaco', monospace;
  font-size: 14px;
}

.terminal-output {
  height: 400px;
  overflow-y: auto;
  padding: 15px;
  line-height: 1.6;
}

.terminal-line {
  margin: 4px 0;
}

.prompt {
  color: var(--accent-green);
  user-select: none;
}

.command {
  color: var(--text-primary);
}

.json-response {
  color: var(--accent-blue);
  margin-left: 20px;
}

/* Resource Meter */
.meter-bar {
  height: 8px;
  background: var(--bg-tertiary);
  border-radius: 4px;
  overflow: hidden;
}

.meter-fill {
  height: 100%;
  background: linear-gradient(90deg, var(--accent-green), var(--accent-blue));
  transition: width 0.5s ease, background 0.3s ease;
}

.meter-fill[data-status="warning"] {
  background: linear-gradient(90deg, var(--accent-yellow), var(--accent-red));
}

.meter-fill[data-status="critical"] {
  background: var(--accent-red);
  animation: pulse-bg 1s ease-in-out infinite;
}

/* Animations */
@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.8; }
}

@keyframes pulse-bg {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.7; }
}

/* Responsive */
@media (max-width: 768px) {
  .status-card {
    padding: 15px;
  }
  
  .terminal-output {
    height: 300px;
    font-size: 12px;
  }
  
  .data-table {
    font-size: 12px;
  }
}
```

---

## Version History

- **1.0.1** (2026-02-02): Initial Defense System implementation
  - iptables, tc, session commands
  - DP/AP/SS resource management
  - Serial command interface
  - Application-level enforcement
  - Idempotency support
