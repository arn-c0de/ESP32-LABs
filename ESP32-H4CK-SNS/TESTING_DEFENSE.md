# ESP32-H4CK Defense System - Testing Guide

## Pre-Deployment Checklist ✅

Before uploading to ESP32:

1. **Verify Compilation**
   - Open `ESP32-H4CK.ino` in Arduino IDE
   - Select Board: "ESP32 Dev Module"
   - Click "Verify" (checkmark icon)
   - Confirm no compilation errors

2. **Upload Filesystem**
   - Use "ESP32 Sketch Data Upload" tool
   - Ensure `data/` folder contains HTML files

3. **Flash Firmware**
   - Connect ESP32 via USB
   - Click "Upload" (arrow icon)
   - Wait for "Done uploading" message

4. **Open Serial Monitor**
   - Set baud rate: 115200
   - Wait for "System Ready!" message
   - Look for "[DEFENSE] Defense system initialized"

---

## Test Scenarios 🧪

### Test 1: Basic Defense Status

**Serial Input:**
```
defense status
```

**Expected Output:**
```json
{
  "status": "ok",
  "result": {
    "resources": {
      "dp": 100,
      "dp_max": 100,
      "ap": 10,
      "ap_max": 10,
      "stability": 100,
      "stability_max": 100
    },
    "active_rules": 0,
    "rules": []
  }
}
```

**Pass Criteria:**
- ✅ JSON formatted correctly
- ✅ Initial resources at maximum
- ✅ No active rules

---

### Test 2: IP Blocking

**Step 1 - Add Block:**
```
iptables -A INPUT -s 192.168.4.100 -j DROP --duration 30
```

**Expected:**
```json
{"status":"ok","id":"...","result":{"rule_id":"r-1","expires":...}}
```
Serial log: `[DEFENSE] IP Block added: 192.168.4.100 (rule r-1, expires in 30s)`

**Step 2 - Verify:**
```
iptables -L
```
**Expected:**
```
Chain INPUT (policy ACCEPT)
target     source               expires
DROP       192.168.4.100        30s
```

**Step 3 - Test Enforcement:**
- Open web browser
- Navigate to `http://<esp-ip>/` from IP `192.168.4.100`
- **Expected:** "Access Denied" (403 error)
- Serial log: `[DEFENSE] Blocked request from 192.168.4.100`

**Step 4 - Remove Block:**
```
iptables -D INPUT -s 192.168.4.100 -j DROP
```
**Expected:**
```json
{"status":"ok","id":"...","msg":"rule removed"}
```

**Pass Criteria:**
- ✅ Rule created with unique ID
- ✅ Listed in `iptables -L`
- ✅ HTTP requests blocked
- ✅ Rule removed successfully

---

### Test 3: Rate Limiting

**Step 1 - Apply Limit:**
```
tc qdisc add rate-limit --src 192.168.4.0/24 --duration 60
```

**Expected:**
```json
{"status":"ok","id":"...","result":{"rule_id":"r-2"}}
```

**Step 2 - Verify:**
```
tc qdisc show
```
**Expected:**
```
qdisc rate-limit:
  src: 192.168.4.0/24 (rule r-2)
```

**Step 3 - Test Enforcement:**
- Send >10 HTTP requests rapidly from IP in range
- **Expected:** After 10th request, receive "Too Many Requests" (429)
- Serial log: `[DEFENSE] Rate limit exceeded for 192.168.4.xxx`

**Pass Criteria:**
- ✅ Rule applied
- ✅ Excess requests blocked with 429

---

### Test 4: Session Reset

**Step 1 - Create Session:**
- Login via web UI as `admin/admin`
- Note your IP address

**Step 2 - Reset Session:**
```
session reset --ip <your-ip>
```

**Expected:**
```json
{"status":"ok","id":"...","result":{"sessions_removed":1}}
```

**Step 3 - Verify:**
- Refresh browser
- **Expected:** Redirected to login (session terminated)

**Pass Criteria:**
- ✅ Session terminated
- ✅ Must re-authenticate

---

### Test 5: Resource Exhaustion

**Step 1 - Check Resources:**
```
defense status
```
Note: `dp=100, ap=10`

**Step 2 - Exhaust DP:**
```
iptables -A INPUT -s 1.1.1.1 -j DROP --duration 30
iptables -A INPUT -s 2.2.2.2 -j DROP --duration 30
iptables -A INPUT -s 3.3.3.3 -j DROP --duration 30
iptables -A INPUT -s 4.4.4.4 -j DROP --duration 30
iptables -A INPUT -s 5.5.5.5 -j DROP --duration 30
iptables -A INPUT -s 6.6.6.6 -j DROP --duration 30
```
Cost: 6 × 15 = 90 DP

**Step 3 - Check Status:**
```
defense status
```
**Expected:** `"dp": 10` (100 - 90)

**Step 4 - Attempt Another Block:**
```
iptables -A INPUT -s 7.7.7.7 -j DROP --duration 30
```
**Expected:**
```json
{"status":"error","error":{"code":403,"msg":"insufficient resources"}}
```

**Step 5 - Wait for Regeneration:**
- Wait 10 seconds (DP +4 = 14)
- Retry step 4
- **Expected:** Success

**Pass Criteria:**
- ✅ Resource tracking accurate
- ✅ Insufficient resources rejected
- ✅ Regeneration works

---

### Test 6: Cooldown Mechanics

**Step 1 - Block IP:**
```
iptables -A INPUT -s 8.8.8.8 -j DROP --duration 30
```
**Expected:** Success

**Step 2 - Immediate Retry:**
```
iptables -A INPUT -s 9.9.9.9 -j DROP --duration 30
```
**Expected:**
```json
{"status":"error","error":{"code":429,"msg":"cooldown active"}}
```

**Step 3 - Wait 60 Seconds:**
- Use stopwatch or wait
- Retry step 2
- **Expected:** Success

**Pass Criteria:**
- ✅ Cooldown enforced
- ✅ Timer resets after period

---

### Test 7: Idempotency

**Step 1 - With Custom ID:**
```
iptables -A INPUT -s 10.10.10.10 -j DROP --duration 30 --id test-123
```
**Expected:** Success, returns `"id":"test-123"`

**Step 2 - Duplicate Request:**
```
iptables -A INPUT -s 10.10.10.10 -j DROP --duration 30 --id test-123
```
**Expected:**
```json
{"status":"ok","msg":"duplicate request (cached)","id":"test-123"}
```

**Step 3 - Verify Only One Rule:**
```
iptables -L
```
**Expected:** Only ONE entry for `10.10.10.10`

**Pass Criteria:**
- ✅ Duplicate ID detected
- ✅ No double execution
- ✅ Cached response returned

---

### Test 8: Configuration Persistence

**Step 1 - Modify Config:**
```
defense config set dp=200 ap=20 stability=150
```
**Expected:** `{"status":"ok","msg":"config updated"}`

**Step 2 - Verify:**
```
defense config show
```
**Expected:** `"max_dp":200,"max_ap":20,"max_stability":150`

**Step 3 - Reboot ESP32:**
- Serial: `/restart`
- Wait for "System Ready!"

**Step 4 - Check Persistence:**
```
defense config show
```
**Expected:** Same values as Step 2

**Pass Criteria:**
- ✅ Config saved to Preferences
- ✅ Survives reboot

---

## Integration Testing 🌐

### WebSocket Shell

1. Open `http://<esp-ip>/shell.html`
2. Type commands in shell
3. Block your IP: `iptables -A INPUT -s <your-ip> -j DROP --duration 60`
4. Try another command
5. **Expected:** "ERROR: Access Denied"

### Telnet

1. `telnet <esp-ip> 23`
2. Login as `admin/admin`
3. From another terminal, block Telnet client IP via serial
4. **Expected:** Connection drops

---

## Performance Benchmarks ⚡

**Memory Usage (after init):**
- Defense module: ~1-2 KB RAM
- Max active rules (32): ~2 KB
- Rate limit tracking (16): ~0.5 KB

**Command Response Time:**
- `defense status`: <10ms
- `iptables` add: <20ms
- Enforcement check: <1ms per request

**Regeneration Rate:**
- DP: +2 every 5 seconds
- AP: +1 every 5 seconds
- Stability: +1 every 5 seconds

---

## Known Limitations ⚠️

1. **Not kernel-level**: Application enforcement only
2. **Max 32 rules**: Ring buffer, oldest auto-expire
3. **Max 16 rate-limit IPs**: Least-recently-used eviction
4. **No subnet matching**: Exact IP match only (future: CIDR)
5. **Single-device**: Multi-ESP sync not yet implemented

---

## Troubleshooting 🔧

**Symptom:** Commands not recognized  
**Check:**
- Serial baud rate = 115200
- Commands exactly as documented
- No extra spaces/quotes

**Symptom:** Rules not enforcing  
**Check:**
- Rule still active (`defense status`)
- IP address exact match
- Serial logs for `[DEFENSE]` messages

**Symptom:** Low heap warnings  
**Solution:**
- Reduce max rules in `15_Defense.ino`
- Clear old rules manually

**Symptom:** Compile errors  
**Check:**
- All `.ino` files in same folder
- Arduino IDE recognizes multi-file sketch
- Libraries installed (Preferences, LittleFS)

---

## Next Steps 🚀

After successful testing:

1. **Scenario Design**: Create Red/Blue Team exercises
2. **Multi-Device**: Plan serial/UDP relay protocol
3. **Web UI**: Optional `/defense` API endpoint
4. **Metrics**: Add scoring system for gameplay
5. **Documentation**: Lab manuals for students

---

## Reporting Issues 🐛

If tests fail:

1. Copy full serial log output
2. Note exact command entered
3. Record ESP32 model and Arduino version
4. Email: arn-c0de@protonmail.com

---

**Version:** 1.0.1  
**Last Updated:** 2026-02-02  
**Testing Time:** ~20 minutes for full suite
