// ============================================================
// 12_Defense.ino — IDS + WAF + Rate Limiting + IP Blocking
// Teacher-controlled via Serial commands
// ============================================================

struct BlockedIP {
  String ip;
  unsigned long blockedAt;
  int durationSec;
  String reason;
  bool active;
};

struct RateLimitEntry {
  String ip;
  int requestCount;
  unsigned long windowStart;
};

struct DefenseEvent {
  String type;
  String ip;
  String details;
  String timestamp;
};

#define MAX_BLOCKED_IPS_ARRAY 20
#define MAX_RATE_ENTRIES 50

BlockedIP blockedIPs[MAX_BLOCKED_IPS_ARRAY];
int blockedIPCount = 0;

RateLimitEntry rateLimits[MAX_RATE_ENTRIES];
int rateLimitCount = 0;

// Defense resource model
int defensePoints    = 100;
int actionPoints     = 10;
int stabilityScore   = 100;
unsigned long lastDefenseRegen = 0;

// WAF patterns
const char* wafPatterns[] = {
  "simulate:", "SELECT", "UNION", "<script>", "../../",
  "flag{", "FLAG{", "admin", ";cat", ";ls"
};
const int NUM_WAF_PATTERNS = 10;

void defenseInit() {
  Serial.println("[DEFENSE] Initializing defense system...");
  memset(blockedIPs, 0, sizeof(blockedIPs));
  memset(rateLimits, 0, sizeof(rateLimits));
  defensePoints  = DEFENSE_POINTS_INITIAL;
  actionPoints   = 10;
  stabilityScore = 100;
  lastDefenseRegen = millis();

  if (DEFENSE_ENABLED) {
    Serial.println("[DEFENSE] Defense ACTIVE.");
    Serial.printf("[DEFENSE] IDS: %s | WAF: %s | Rate Limit: %s\n",
      IDS_ACTIVE ? "ON" : "OFF", WAF_ACTIVE ? "ON" : "OFF",
      RATE_LIMIT_ACTIVE ? "ON" : "OFF");
  } else {
    Serial.println("[DEFENSE] Defense DISABLED (EASY mode or not enabled).");
  }
}

// ===== Defense update (regen resources) =====
void defenseUpdate() {
  if (!DEFENSE_ENABLED) return;

  unsigned long now = millis();
  if (now - lastDefenseRegen >= 5000) {
    lastDefenseRegen = now;
    defensePoints  = min(defensePoints + 2, DEFENSE_POINTS_INITIAL);
    actionPoints   = min(actionPoints + 1, 10);
    stabilityScore = min(stabilityScore + 1, 100);
  }

  // Expire blocked IPs
  for (int i = 0; i < blockedIPCount; i++) {
    if (blockedIPs[i].active) {
      unsigned long elapsed = (now - blockedIPs[i].blockedAt) / 1000;
      if ((int)elapsed >= blockedIPs[i].durationSec) {
        blockedIPs[i].active = false;
        debugLogf("DEFENSE", "IP unblocked (expired): %s", blockedIPs[i].ip.c_str());
      }
    }
  }
}

// ===== IP Blocking =====
bool isIPBlocked(const String& ip) {
  if (!IP_BLOCKING_ENABLED) return false;
  for (int i = 0; i < blockedIPCount; i++) {
    if (blockedIPs[i].active && blockedIPs[i].ip == ip) return true;
  }
  return false;
}

void blockIP(const String& ip, int durationSec) {
  if (!IP_BLOCKING_ENABLED) return;

  // Check if already blocked
  for (int i = 0; i < blockedIPCount; i++) {
    if (blockedIPs[i].ip == ip) {
      blockedIPs[i].active      = true;
      blockedIPs[i].blockedAt   = millis();
      blockedIPs[i].durationSec = durationSec;
      return;
    }
  }

  // New entry
  if (blockedIPCount < MAX_BLOCKED_IPS_ARRAY) {
    blockedIPs[blockedIPCount].ip          = ip;
    blockedIPs[blockedIPCount].blockedAt   = millis();
    blockedIPs[blockedIPCount].durationSec = durationSec;
    blockedIPs[blockedIPCount].reason      = "manual";
    blockedIPs[blockedIPCount].active      = true;
    blockedIPCount++;

    defensePoints -= 10;  // Costs DP
    logDefenseEvent("IP_BLOCK", ip, "Blocked for " + String(durationSec) + "s");
    debugLogf("DEFENSE", "IP blocked: %s for %ds", ip.c_str(), durationSec);
  }
}

void unblockIP(const String& ip) {
  for (int i = 0; i < blockedIPCount; i++) {
    if (blockedIPs[i].ip == ip) {
      blockedIPs[i].active = false;
      debugLogf("DEFENSE", "IP unblocked: %s", ip.c_str());
      return;
    }
  }
}

// ===== Rate Limiting =====
bool checkRateLimit(const String& ip) {
  if (!RATE_LIMIT_ACTIVE) return true;

  unsigned long now = millis();

  // Find existing entry
  for (int i = 0; i < rateLimitCount; i++) {
    if (rateLimits[i].ip == ip) {
      // Reset window if older than 60 seconds
      if (now - rateLimits[i].windowStart > 60000) {
        rateLimits[i].requestCount = 1;
        rateLimits[i].windowStart  = now;
        return true;
      }
      rateLimits[i].requestCount++;
      if (rateLimits[i].requestCount > RATE_LIMIT_PER_MINUTE) {
        logDefenseEvent("RATE_LIMIT", ip,
          "Exceeded " + String(RATE_LIMIT_PER_MINUTE) + " req/min");

        // Auto-block on excessive requests
        if (rateLimits[i].requestCount > RATE_LIMIT_PER_MINUTE * 2) {
          blockIP(ip, 60);
        }
        return false;
      }
      return true;
    }
  }

  // New entry
  if (rateLimitCount < MAX_RATE_ENTRIES) {
    rateLimits[rateLimitCount].ip           = ip;
    rateLimits[rateLimitCount].requestCount = 1;
    rateLimits[rateLimitCount].windowStart  = now;
    rateLimitCount++;
  }
  return true;
}

// ===== WAF Check =====
bool wafCheck(const String& input) {
  if (!WAF_ACTIVE) return true;

  String lower = input;
  lower.toLowerCase();

  for (int i = 0; i < NUM_WAF_PATTERNS; i++) {
    String pattern = String(wafPatterns[i]);
    pattern.toLowerCase();
    if (lower.indexOf(pattern) >= 0) {
      logDefenseEvent("WAF_BLOCK", "", "Pattern match: " + String(wafPatterns[i]));
      return false;
    }
  }
  return true;
}

// ===== Log defense event =====
void logDefenseEvent(const String& type, const String& ip, const String& details) {
  if (!DEFENSE_ENABLED) return;

  JsonDocument entry;
  entry["type"]      = type;
  entry["ip"]        = ip;
  entry["details"]   = details;
  entry["timestamp"] = getISOTimestamp();
  String json;
  serializeJson(entry, json);
  defenseLogBuffer.push(json);

  if (LOG_DEFENSE_ACTIONS) {
    debugLogf("DEFENSE", "[%s] %s: %s", type.c_str(), ip.c_str(), details.c_str());
  }
}

// ===== Get defense status =====
String getDefenseStatus() {
  JsonDocument doc;
  doc["defense_enabled"] = DEFENSE_ENABLED;

  JsonObject resources = doc["resources"].to<JsonObject>();
  resources["dp"]        = defensePoints;
  resources["dp_max"]    = DEFENSE_POINTS_INITIAL;
  resources["ap"]        = actionPoints;
  resources["ap_max"]    = 10;
  resources["stability"] = stabilityScore;

  JsonObject modules = doc["modules"].to<JsonObject>();
  modules["ids"]          = IDS_ACTIVE;
  modules["waf"]          = WAF_ACTIVE;
  modules["rate_limit"]   = RATE_LIMIT_ACTIVE;
  modules["ip_blocking"]  = IP_BLOCKING_ENABLED;
  modules["honeypot"]     = HONEYPOT_ENDPOINTS_ENABLED;

  JsonArray blocked = doc["blocked_ips"].to<JsonArray>();
  for (int i = 0; i < blockedIPCount; i++) {
    if (blockedIPs[i].active) {
      JsonObject b = blocked.add<JsonObject>();
      b["ip"]       = blockedIPs[i].ip;
      b["duration"] = blockedIPs[i].durationSec;
      b["remaining"] = max(0, blockedIPs[i].durationSec -
        (int)((millis() - blockedIPs[i].blockedAt) / 1000));
      b["reason"] = blockedIPs[i].reason;
    }
  }

  doc["aggressiveness"] = DEFENSE_AGGRESSIVENESS;

  String out;
  serializeJson(doc, out);
  return out;
}

// ===== Get defense alerts =====
String getDefenseAlerts() {
  JsonDocument doc;
  doc["ids_active"] = IDS_ACTIVE;
  doc["alerts"]     = serialized(defenseLogBuffer.toJsonArray(50));
  String out;
  serializeJson(doc, out);
  return out;
}

// ===== Set rate limit (for Serial commands) =====
void setGlobalRateLimit(int reqPerMin) {
  RATE_LIMIT_PER_MINUTE = reqPerMin;
  RATE_LIMIT_ACTIVE = true;
  defensePoints -= 5;
  debugLogf("DEFENSE", "Rate limit set: %d req/min", reqPerMin);
}

// ===== Reset all sessions for IP =====
void resetSessionsForIP(const String& ip, const String& reason) {
  destroySessionsByIP(ip);
  defensePoints -= 15;
  logDefenseEvent("SESSION_RESET", ip, "Reason: " + reason);
  debugLogf("DEFENSE", "Sessions reset for %s: %s", ip.c_str(), reason.c_str());
}
