/*
 * Defense & Gameplay Module
 * 
 * Simulates defensive measures (IP blocking, rate limiting, session management)
 * with resource constraints (Defense Points, Action Points, Stability).
 * Provides iptables-like, tc-like, and session command syntax for education.
 * 
 * Commands simulate application-level enforcement, NOT kernel/network changes.
 */

// ===== DATA STRUCTURES =====

enum DefenseType {
  DEFENSE_NONE = 0,
  DEFENSE_IP_BLOCK,
  DEFENSE_RATE_LIMIT,
  DEFENSE_SESSION_RESET,
  DEFENSE_MISTRUST_MODE
};

struct DefenseCost {
  int dp;  // Defense Points
  int ap;  // Action Points
  int ss;  // Stability Score impact (negative)
};

struct DefenseRule {
  String id;              // Unique rule ID (e.g., "r-001")
  String requestId;       // Client request ID for idempotency
  DefenseType type;
  String target;          // IP address or range
  unsigned long createdAt;
  unsigned long expiresAt;
  int cost_dp;
  int cost_ap;
  bool active;
  String metadata;        // Extra data (JSON-like)
};

struct DefenseConfig {
  int max_dp;             // Maximum Defense Points
  int max_ap;             // Maximum Action Points
  int max_stability;      // Maximum Stability Score
  int current_dp;
  int current_ap;
  int current_stability;
  
  // Per-defense costs
  DefenseCost ipblock_cost;
  DefenseCost ratelimit_cost;
  DefenseCost sessionreset_cost;
  DefenseCost mistrust_cost;
  
  // Cooldowns (seconds)
  int ipblock_cooldown;
  int ratelimit_cooldown;
  int sessionreset_cooldown;
  int mistrust_cooldown;
  
  // Last activation timestamps
  unsigned long ipblock_last;
  unsigned long ratelimit_last;
  unsigned long sessionreset_last;
  unsigned long mistrust_last;
};

// Rate limiting tracking
struct RateLimitEntry {
  String ip;
  int requestCount;
  unsigned long windowStart;
  int maxRequests;
};

// ===== GLOBAL STATE =====

DefenseConfig defenseConfig;
DefenseRule activeRules[32];  // Max 32 concurrent rules
int activeRuleCount = 0;
String recentRequestIds[32];  // For idempotency
int recentIdCount = 0;
RateLimitEntry rateLimits[16]; // Track rate limits per IP
int rateLimitCount = 0;

// ===== INITIALIZATION =====

void initDefense() {
  // Load config from Preferences or use defaults
  preferences.begin("defense", false);
  
  defenseConfig.max_dp = preferences.getInt("max_dp", 100);
  defenseConfig.max_ap = preferences.getInt("max_ap", 10);
  defenseConfig.max_stability = preferences.getInt("max_stab", 100);
  defenseConfig.current_dp = defenseConfig.max_dp;
  defenseConfig.current_ap = defenseConfig.max_ap;
  defenseConfig.current_stability = defenseConfig.max_stability;
  
  // IP Block defaults
  defenseConfig.ipblock_cost.dp = preferences.getInt("ipb_dp", 15);
  defenseConfig.ipblock_cost.ap = preferences.getInt("ipb_ap", 1);
  defenseConfig.ipblock_cost.ss = preferences.getInt("ipb_ss", -5);
  defenseConfig.ipblock_cooldown = preferences.getInt("ipb_cd", 60);
  defenseConfig.ipblock_last = 0;
  
  // Rate Limit defaults
  defenseConfig.ratelimit_cost.dp = preferences.getInt("rl_dp", 10);
  defenseConfig.ratelimit_cost.ap = preferences.getInt("rl_ap", 1);
  defenseConfig.ratelimit_cost.ss = preferences.getInt("rl_ss", -3);
  defenseConfig.ratelimit_cooldown = preferences.getInt("rl_cd", 30);
  defenseConfig.ratelimit_last = 0;
  
  // Session Reset defaults
  defenseConfig.sessionreset_cost.dp = preferences.getInt("sr_dp", 25);
  defenseConfig.sessionreset_cost.ap = preferences.getInt("sr_ap", 1);
  defenseConfig.sessionreset_cost.ss = preferences.getInt("sr_ss", -10);
  defenseConfig.sessionreset_cooldown = preferences.getInt("sr_cd", 90);
  defenseConfig.sessionreset_last = 0;
  
  // Mistrust Mode defaults
  defenseConfig.mistrust_cost.dp = preferences.getInt("mt_dp", 40);
  defenseConfig.mistrust_cost.ap = preferences.getInt("mt_ap", 2);
  defenseConfig.mistrust_cost.ss = preferences.getInt("mt_ss", -20);
  defenseConfig.mistrust_cooldown = preferences.getInt("mt_cd", 600);
  defenseConfig.mistrust_last = 0;
  
  preferences.end();
  
  // Initialize arrays
  for (int i = 0; i < 32; i++) {
    activeRules[i].active = false;
    recentRequestIds[i] = "";
  }
  for (int i = 0; i < 16; i++) {
    rateLimits[i].ip = "";
    rateLimits[i].requestCount = 0;
  }
  
  Serial.println("[DEFENSE] Defense system initialized");
  Serial.printf("[DEFENSE] Resources: DP=%d/%d AP=%d/%d Stability=%d/%d\n",
                defenseConfig.current_dp, defenseConfig.max_dp,
                defenseConfig.current_ap, defenseConfig.max_ap,
                defenseConfig.current_stability, defenseConfig.max_stability);
}

void saveDefenseConfig() {
  preferences.begin("defense", false);
  preferences.putInt("max_dp", defenseConfig.max_dp);
  preferences.putInt("max_ap", defenseConfig.max_ap);
  preferences.putInt("max_stab", defenseConfig.max_stability);
  preferences.putInt("ipb_dp", defenseConfig.ipblock_cost.dp);
  preferences.putInt("ipb_ap", defenseConfig.ipblock_cost.ap);
  preferences.putInt("ipb_ss", defenseConfig.ipblock_cost.ss);
  preferences.putInt("ipb_cd", defenseConfig.ipblock_cooldown);
  preferences.putInt("rl_dp", defenseConfig.ratelimit_cost.dp);
  preferences.putInt("rl_ap", defenseConfig.ratelimit_cost.ap);
  preferences.putInt("rl_ss", defenseConfig.ratelimit_cost.ss);
  preferences.putInt("rl_cd", defenseConfig.ratelimit_cooldown);
  preferences.putInt("sr_dp", defenseConfig.sessionreset_cost.dp);
  preferences.putInt("sr_ap", defenseConfig.sessionreset_cost.ap);
  preferences.putInt("sr_ss", defenseConfig.sessionreset_cost.ss);
  preferences.putInt("sr_cd", defenseConfig.sessionreset_cooldown);
  preferences.putInt("mt_dp", defenseConfig.mistrust_cost.dp);
  preferences.putInt("mt_ap", defenseConfig.mistrust_cost.ap);
  preferences.putInt("mt_ss", defenseConfig.mistrust_cost.ss);
  preferences.putInt("mt_cd", defenseConfig.mistrust_cooldown);
  preferences.end();
}

// ===== HELPER FUNCTIONS =====

String generateRuleId() {
  static int ruleCounter = 0;
  ruleCounter++;
  return "r-" + String(ruleCounter);
}

bool isRequestIdRecent(String reqId) {
  if (reqId.length() == 0) return false;
  for (int i = 0; i < 32; i++) {
    if (recentRequestIds[i] == reqId) return true;
  }
  return false;
}

void addRequestId(String reqId) {
  if (reqId.length() == 0) return;
  // Ring buffer
  for (int i = 31; i > 0; i--) {
    recentRequestIds[i] = recentRequestIds[i-1];
  }
  recentRequestIds[0] = reqId;
}

bool checkCooldown(DefenseType type) {
  unsigned long now = millis() / 1000;
  unsigned long last = 0;
  int cooldown = 0;
  
  switch (type) {
    case DEFENSE_IP_BLOCK:
      last = defenseConfig.ipblock_last;
      cooldown = defenseConfig.ipblock_cooldown;
      break;
    case DEFENSE_RATE_LIMIT:
      last = defenseConfig.ratelimit_last;
      cooldown = defenseConfig.ratelimit_cooldown;
      break;
    case DEFENSE_SESSION_RESET:
      last = defenseConfig.sessionreset_last;
      cooldown = defenseConfig.sessionreset_cooldown;
      break;
    case DEFENSE_MISTRUST_MODE:
      last = defenseConfig.mistrust_last;
      cooldown = defenseConfig.mistrust_cooldown;
      break;
    default:
      return true;
  }
  
  return (now - last) >= cooldown;
}

void updateLastActivation(DefenseType type) {
  unsigned long now = millis() / 1000;
  switch (type) {
    case DEFENSE_IP_BLOCK:
      defenseConfig.ipblock_last = now;
      break;
    case DEFENSE_RATE_LIMIT:
      defenseConfig.ratelimit_last = now;
      break;
    case DEFENSE_SESSION_RESET:
      defenseConfig.sessionreset_last = now;
      break;
    case DEFENSE_MISTRUST_MODE:
      defenseConfig.mistrust_last = now;
      break;
    default:
      break;
  }
}

DefenseCost getCost(DefenseType type) {
  switch (type) {
    case DEFENSE_IP_BLOCK:
      return defenseConfig.ipblock_cost;
    case DEFENSE_RATE_LIMIT:
      return defenseConfig.ratelimit_cost;
    case DEFENSE_SESSION_RESET:
      return defenseConfig.sessionreset_cost;
    case DEFENSE_MISTRUST_MODE:
      return defenseConfig.mistrust_cost;
    default:
      return {0, 0, 0};
  }
}

bool canAfford(DefenseType type) {
  DefenseCost cost = getCost(type);
  return (defenseConfig.current_dp >= cost.dp && 
          defenseConfig.current_ap >= cost.ap);
}

void consumeResources(DefenseType type) {
  DefenseCost cost = getCost(type);
  defenseConfig.current_dp -= cost.dp;
  defenseConfig.current_ap -= cost.ap;
  defenseConfig.current_stability += cost.ss; // ss is negative
  
  // Clamp
  if (defenseConfig.current_dp < 0) defenseConfig.current_dp = 0;
  if (defenseConfig.current_ap < 0) defenseConfig.current_ap = 0;
  if (defenseConfig.current_stability < 0) defenseConfig.current_stability = 0;
}

void tickDefenseResources() {
  // Slowly regenerate resources (called from loop periodically)
  static unsigned long lastRegen = 0;
  unsigned long now = millis();
  
  if (now - lastRegen > 5000) { // Every 5 seconds
    if (defenseConfig.current_dp < defenseConfig.max_dp) {
      defenseConfig.current_dp += 2;
      if (defenseConfig.current_dp > defenseConfig.max_dp) 
        defenseConfig.current_dp = defenseConfig.max_dp;
    }
    if (defenseConfig.current_ap < defenseConfig.max_ap) {
      defenseConfig.current_ap += 1;
      if (defenseConfig.current_ap > defenseConfig.max_ap) 
        defenseConfig.current_ap = defenseConfig.max_ap;
    }
    if (defenseConfig.current_stability < defenseConfig.max_stability) {
      defenseConfig.current_stability += 1;
      if (defenseConfig.current_stability > defenseConfig.max_stability)
        defenseConfig.current_stability = defenseConfig.max_stability;
    }
    lastRegen = now;
  }
}

void tickDefenseRules() {
  // Remove expired rules
  unsigned long now = millis();
  for (int i = 0; i < 32; i++) {
    if (activeRules[i].active && activeRules[i].expiresAt > 0 && now >= activeRules[i].expiresAt) {
      Serial.printf("[DEFENSE] Rule %s expired\n", activeRules[i].id.c_str());
      activeRules[i].active = false;
      activeRuleCount--;
    }
  }
}

// ===== ENFORCEMENT FUNCTIONS =====

bool isIpBlocked(String ip) {
  for (int i = 0; i < 32; i++) {
    if (activeRules[i].active && 
        activeRules[i].type == DEFENSE_IP_BLOCK && 
        activeRules[i].target == ip) {
      return true;
    }
  }
  return false;
}

bool checkRateLimit(String ip) {
  // Find or create rate limit entry
  int idx = -1;
  unsigned long now = millis();
  
  for (int i = 0; i < 16; i++) {
    if (rateLimits[i].ip == ip) {
      idx = i;
      break;
    }
  }
  
  if (idx == -1) {
    // Find empty slot
    for (int i = 0; i < 16; i++) {
      if (rateLimits[i].ip == "") {
        idx = i;
        rateLimits[i].ip = ip;
        rateLimits[i].requestCount = 0;
        rateLimits[i].windowStart = now;
        rateLimits[i].maxRequests = 10; // Default
        break;
      }
    }
  }
  
  if (idx == -1) return true; // No slots, allow
  
  // Check if window expired (60 seconds)
  if (now - rateLimits[idx].windowStart > 60000) {
    rateLimits[idx].requestCount = 0;
    rateLimits[idx].windowStart = now;
  }
  
  rateLimits[idx].requestCount++;
  
  // Check active rate limit rules
  for (int i = 0; i < 32; i++) {
    if (activeRules[i].active && 
        activeRules[i].type == DEFENSE_RATE_LIMIT &&
        (activeRules[i].target == ip || activeRules[i].target == "0.0.0.0/0")) {
      if (rateLimits[idx].requestCount > rateLimits[idx].maxRequests) {
        return false; // Rate limited
      }
    }
  }
  
  return true; // OK
}

// ===== COMMAND HANDLERS =====

String handleIptablesCommand(String args, String reqId) {
  // Parse: iptables -A INPUT -s <ip> -j DROP --duration <sec> [--dp <n>] [--ap <n>]
  args.trim();
  
  // Check for -A (add) or -D (delete)
  bool isAdd = args.indexOf("-A") >= 0;
  bool isDel = args.indexOf("-D") >= 0;
  
  if (!isAdd && !isDel) {
    return "{\"status\":\"error\",\"error\":{\"code\":400,\"msg\":\"missing -A or -D\"}}";
  }
  
  // Extract IP
  int srcIdx = args.indexOf("-s ");
  if (srcIdx < 0) {
    return "{\"status\":\"error\",\"error\":{\"code\":400,\"msg\":\"missing -s <ip>\"}}";
  }
  
  String ip = "";
  int spaceIdx = args.indexOf(' ', srcIdx + 3);
  if (spaceIdx > 0) {
    ip = args.substring(srcIdx + 3, spaceIdx);
  } else {
    ip = args.substring(srcIdx + 3);
  }
  ip.trim();
  
  if (isAdd) {
    // Check idempotency
    if (isRequestIdRecent(reqId)) {
      return "{\"status\":\"ok\",\"msg\":\"duplicate request (cached)\",\"id\":\"" + reqId + "\"}";
    }
    
    // Check cooldown
    if (!checkCooldown(DEFENSE_IP_BLOCK)) {
      return "{\"status\":\"error\",\"error\":{\"code\":429,\"msg\":\"cooldown active\"}}";
    }
    
    // Check resources
    if (!canAfford(DEFENSE_IP_BLOCK)) {
      return "{\"status\":\"error\",\"error\":{\"code\":403,\"msg\":\"insufficient resources\"}}";
    }
    
    // Parse duration
    int duration = 30; // default
    int durIdx = args.indexOf("--duration ");
    if (durIdx >= 0) {
      String durStr = args.substring(durIdx + 11);
      durStr.trim();
      spaceIdx = durStr.indexOf(' ');
      if (spaceIdx > 0) durStr = durStr.substring(0, spaceIdx);
      duration = durStr.toInt();
    }
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < 32; i++) {
      if (!activeRules[i].active) {
        slot = i;
        break;
      }
    }
    
    if (slot == -1) {
      return "{\"status\":\"error\",\"error\":{\"code\":507,\"msg\":\"rule table full\"}}";
    }
    
    // Add rule
    activeRules[slot].id = generateRuleId();
    activeRules[slot].requestId = reqId;
    activeRules[slot].type = DEFENSE_IP_BLOCK;
    activeRules[slot].target = ip;
    activeRules[slot].createdAt = millis();
    activeRules[slot].expiresAt = millis() + (duration * 1000);
    activeRules[slot].cost_dp = defenseConfig.ipblock_cost.dp;
    activeRules[slot].cost_ap = defenseConfig.ipblock_cost.ap;
    activeRules[slot].active = true;
    activeRuleCount++;
    
    consumeResources(DEFENSE_IP_BLOCK);
    updateLastActivation(DEFENSE_IP_BLOCK);
    addRequestId(reqId);
    
    Serial.printf("[DEFENSE] IP Block added: %s (rule %s, expires in %ds)\n", 
                  ip.c_str(), activeRules[slot].id.c_str(), duration);
    
    return "{\"status\":\"ok\",\"id\":\"" + reqId + "\",\"result\":{\"rule_id\":\"" + 
           activeRules[slot].id + "\",\"expires\":" + String(activeRules[slot].expiresAt) + "}}";
           
  } else { // Delete
    // Find and remove rule
    for (int i = 0; i < 32; i++) {
      if (activeRules[i].active && 
          activeRules[i].type == DEFENSE_IP_BLOCK && 
          activeRules[i].target == ip) {
        activeRules[i].active = false;
        activeRuleCount--;
        Serial.printf("[DEFENSE] IP Block removed: %s\n", ip.c_str());
        return "{\"status\":\"ok\",\"id\":\"" + reqId + "\",\"msg\":\"rule removed\"}";
      }
    }
    return "{\"status\":\"error\",\"error\":{\"code\":404,\"msg\":\"rule not found\"}}";
  }
}

String handleTcCommand(String args, String reqId) {
  // Parse: tc qdisc add rate-limit --src <ip/range> --rps <n> --duration <sec>
  args.trim();
  
  bool isAdd = args.indexOf("add") >= 0;
  bool isDel = args.indexOf("del") >= 0;
  
  if (!isAdd && !isDel) {
    return "{\"status\":\"error\",\"error\":{\"code\":400,\"msg\":\"missing add/del\"}}";
  }
  
  // Extract src
  int srcIdx = args.indexOf("--src ");
  String src = "0.0.0.0/0";
  if (srcIdx >= 0) {
    int spaceIdx = args.indexOf(' ', srcIdx + 6);
    if (spaceIdx > 0) {
      src = args.substring(srcIdx + 6, spaceIdx);
    } else {
      src = args.substring(srcIdx + 6);
    }
    src.trim();
  }
  
  if (isAdd) {
    if (isRequestIdRecent(reqId)) {
      return "{\"status\":\"ok\",\"msg\":\"duplicate request\",\"id\":\"" + reqId + "\"}";
    }
    
    if (!checkCooldown(DEFENSE_RATE_LIMIT)) {
      return "{\"status\":\"error\",\"error\":{\"code\":429,\"msg\":\"cooldown active\"}}";
    }
    
    if (!canAfford(DEFENSE_RATE_LIMIT)) {
      return "{\"status\":\"error\",\"error\":{\"code\":403,\"msg\":\"insufficient resources\"}}";
    }
    
    int duration = 60;
    int durIdx = args.indexOf("--duration ");
    if (durIdx >= 0) {
      String durStr = args.substring(durIdx + 11);
      durStr.trim();
      int spaceIdx = durStr.indexOf(' ');
      if (spaceIdx > 0) durStr = durStr.substring(0, spaceIdx);
      duration = durStr.toInt();
    }
    
    int slot = -1;
    for (int i = 0; i < 32; i++) {
      if (!activeRules[i].active) {
        slot = i;
        break;
      }
    }
    
    if (slot == -1) {
      return "{\"status\":\"error\",\"error\":{\"code\":507,\"msg\":\"rule table full\"}}";
    }
    
    activeRules[slot].id = generateRuleId();
    activeRules[slot].requestId = reqId;
    activeRules[slot].type = DEFENSE_RATE_LIMIT;
    activeRules[slot].target = src;
    activeRules[slot].createdAt = millis();
    activeRules[slot].expiresAt = millis() + (duration * 1000);
    activeRules[slot].cost_dp = defenseConfig.ratelimit_cost.dp;
    activeRules[slot].cost_ap = defenseConfig.ratelimit_cost.ap;
    activeRules[slot].active = true;
    activeRuleCount++;
    
    consumeResources(DEFENSE_RATE_LIMIT);
    updateLastActivation(DEFENSE_RATE_LIMIT);
    addRequestId(reqId);
    
    Serial.printf("[DEFENSE] Rate Limit added: %s (rule %s, expires in %ds)\n",
                  src.c_str(), activeRules[slot].id.c_str(), duration);
    
    return "{\"status\":\"ok\",\"id\":\"" + reqId + "\",\"result\":{\"rule_id\":\"" + 
           activeRules[slot].id + "\"}}";
           
  } else {
    for (int i = 0; i < 32; i++) {
      if (activeRules[i].active && 
          activeRules[i].type == DEFENSE_RATE_LIMIT && 
          activeRules[i].target == src) {
        activeRules[i].active = false;
        activeRuleCount--;
        return "{\"status\":\"ok\",\"id\":\"" + reqId + "\",\"msg\":\"removed\"}";
      }
    }
    return "{\"status\":\"error\",\"error\":{\"code\":404,\"msg\":\"not found\"}}";
  }
}

String handleSessionCommand(String args, String reqId) {
  // Parse: session reset --ip <ip> [--reason <text>]
  args.trim();
  
  if (args.indexOf("reset") < 0) {
    return "{\"status\":\"error\",\"error\":{\"code\":400,\"msg\":\"unknown session command\"}}";
  }
  
  int ipIdx = args.indexOf("--ip ");
  if (ipIdx < 0) {
    return "{\"status\":\"error\",\"error\":{\"code\":400,\"msg\":\"missing --ip\"}}";
  }
  
  String ip = "";
  int spaceIdx = args.indexOf(' ', ipIdx + 5);
  if (spaceIdx > 0) {
    ip = args.substring(ipIdx + 5, spaceIdx);
  } else {
    ip = args.substring(ipIdx + 5);
  }
  ip.trim();
  
  if (isRequestIdRecent(reqId)) {
    return "{\"status\":\"ok\",\"msg\":\"duplicate request\",\"id\":\"" + reqId + "\"}";
  }
  
  if (!checkCooldown(DEFENSE_SESSION_RESET)) {
    return "{\"status\":\"error\",\"error\":{\"code\":429,\"msg\":\"cooldown active\"}}";
  }
  
  if (!canAfford(DEFENSE_SESSION_RESET)) {
    return "{\"status\":\"error\",\"error\":{\"code\":403,\"msg\":\"insufficient resources\"}}";
  }
  
  // Find and remove session
  int removed = 0;
  for (auto it = activeSessions.begin(); it != activeSessions.end(); ) {
    if (it->second.ipAddress == ip) {
      it = activeSessions.erase(it);
      removed++;
    } else {
      ++it;
    }
  }
  
  consumeResources(DEFENSE_SESSION_RESET);
  updateLastActivation(DEFENSE_SESSION_RESET);
  addRequestId(reqId);
  
  Serial.printf("[DEFENSE] Session reset: %s (%d sessions removed)\n", ip.c_str(), removed);
  
  return "{\"status\":\"ok\",\"id\":\"" + reqId + "\",\"result\":{\"sessions_removed\":" + 
         String(removed) + "}}";
}

String handleDefenseStatus() {
  String json = "{\"status\":\"ok\",\"result\":{";
  json += "\"resources\":{";
  json += "\"dp\":" + String(defenseConfig.current_dp) + ",";
  json += "\"dp_max\":" + String(defenseConfig.max_dp) + ",";
  json += "\"ap\":" + String(defenseConfig.current_ap) + ",";
  json += "\"ap_max\":" + String(defenseConfig.max_ap) + ",";
  json += "\"stability\":" + String(defenseConfig.current_stability) + ",";
  json += "\"stability_max\":" + String(defenseConfig.max_stability);
  json += "},";
  json += "\"active_rules\":" + String(activeRuleCount) + ",";
  json += "\"rules\":[";
  
  bool first = true;
  for (int i = 0; i < 32; i++) {
    if (activeRules[i].active) {
      if (!first) json += ",";
      json += "{\"id\":\"" + activeRules[i].id + "\",";
      json += "\"type\":\"" + String(activeRules[i].type) + "\",";
      json += "\"target\":\"" + activeRules[i].target + "\",";
      json += "\"expires\":" + String(activeRules[i].expiresAt) + "}";
      first = false;
    }
  }
  json += "]}}";
  
  return json;
}

String handleDefenseConfig(String args) {
  args.trim();
  
  if (args.indexOf("show") >= 0) {
    String json = "{\"status\":\"ok\",\"config\":{";
    json += "\"max_dp\":" + String(defenseConfig.max_dp) + ",";
    json += "\"max_ap\":" + String(defenseConfig.max_ap) + ",";
    json += "\"max_stability\":" + String(defenseConfig.max_stability) + ",";
    json += "\"ipblock\":{\"dp\":" + String(defenseConfig.ipblock_cost.dp) + 
            ",\"ap\":" + String(defenseConfig.ipblock_cost.ap) + 
            ",\"cooldown\":" + String(defenseConfig.ipblock_cooldown) + "},";
    json += "\"ratelimit\":{\"dp\":" + String(defenseConfig.ratelimit_cost.dp) + 
            ",\"ap\":" + String(defenseConfig.ratelimit_cost.ap) + 
            ",\"cooldown\":" + String(defenseConfig.ratelimit_cooldown) + "},";
    json += "\"sessionreset\":{\"dp\":" + String(defenseConfig.sessionreset_cost.dp) + 
            ",\"ap\":" + String(defenseConfig.sessionreset_cost.ap) + 
            ",\"cooldown\":" + String(defenseConfig.sessionreset_cooldown) + "}";
    json += "}}";
    return json;
  }
  
  if (args.indexOf("set") >= 0) {
    // Parse key=value pairs
    if (args.indexOf("dp=") >= 0) {
      int idx = args.indexOf("dp=") + 3;
      int endIdx = args.indexOf(' ', idx);
      String val = (endIdx > 0) ? args.substring(idx, endIdx) : args.substring(idx);
      defenseConfig.max_dp = val.toInt();
      defenseConfig.current_dp = defenseConfig.max_dp;
    }
    if (args.indexOf("ap=") >= 0) {
      int idx = args.indexOf("ap=") + 3;
      int endIdx = args.indexOf(' ', idx);
      String val = (endIdx > 0) ? args.substring(idx, endIdx) : args.substring(idx);
      defenseConfig.max_ap = val.toInt();
      defenseConfig.current_ap = defenseConfig.max_ap;
    }
    if (args.indexOf("stability=") >= 0) {
      int idx = args.indexOf("stability=") + 10;
      int endIdx = args.indexOf(' ', idx);
      String val = (endIdx > 0) ? args.substring(idx, endIdx) : args.substring(idx);
      defenseConfig.max_stability = val.toInt();
      defenseConfig.current_stability = defenseConfig.max_stability;
    }
    
    saveDefenseConfig();
    return "{\"status\":\"ok\",\"msg\":\"config updated\"}";
  }
  
  return "{\"status\":\"error\",\"error\":{\"code\":400,\"msg\":\"unknown config command\"}}";
}

String handleIptablesList() {
  String output = "Chain INPUT (policy ACCEPT)\n";
  output += "target     source               expires\n";
  
  unsigned long now = millis();
  for (int i = 0; i < 32; i++) {
    if (activeRules[i].active && activeRules[i].type == DEFENSE_IP_BLOCK) {
      output += "DROP       " + activeRules[i].target;
      // Pad to 20 chars
      while (output.length() % 40 < 20) output += " ";
      if (activeRules[i].expiresAt > now) {
        output += String((activeRules[i].expiresAt - now) / 1000) + "s";
      } else {
        output += "expired";
      }
      output += "\n";
    }
  }
  
  if (activeRuleCount == 0) {
    output += "(no rules)\n";
  }
  
  return output;
}

String handleTcShow() {
  String output = "qdisc rate-limit:\n";
  
  for (int i = 0; i < 32; i++) {
    if (activeRules[i].active && activeRules[i].type == DEFENSE_RATE_LIMIT) {
      output += "  src: " + activeRules[i].target;
      output += " (rule " + activeRules[i].id + ")\n";
    }
  }
  
  if (activeRuleCount == 0) {
    output += "(no rate limits active)\n";
  }
  
  return output;
}

// Main command dispatcher
String handleDefenseLine(String line) {
  line.trim();
  
  // Generate request ID if not present
  String reqId = "";
  int idIdx = line.indexOf("--id ");
  if (idIdx >= 0) {
    int spaceIdx = line.indexOf(' ', idIdx + 5);
    reqId = (spaceIdx > 0) ? line.substring(idIdx + 5, spaceIdx) : line.substring(idIdx + 5);
    reqId.trim();
  } else {
    reqId = "auto-" + String(millis());
  }
  
  String lineLower = line;
  lineLower.toLowerCase();
  
  // Check for JSON
  if (line.startsWith("{")) {
    // TODO: JSON parsing (would use ArduinoJson here)
    return "{\"status\":\"error\",\"error\":{\"code\":501,\"msg\":\"JSON not yet implemented\"}}";
  }
  
  // Human commands
  if (lineLower.startsWith("iptables")) {
    String args = line.substring(8);
    return handleIptablesCommand(args, reqId);
  }
  else if (lineLower.startsWith("tc ")) {
    String args = line.substring(3);
    return handleTcCommand(args, reqId);
  }
  else if (lineLower.startsWith("session ")) {
    String args = line.substring(8);
    return handleSessionCommand(args, reqId);
  }
  else if (lineLower.startsWith("defense status")) {
    return handleDefenseStatus();
  }
  else if (lineLower.startsWith("defense config")) {
    String args = line.substring(14);
    return handleDefenseConfig(args);
  }
  else {
    return "{\"status\":\"error\",\"error\":{\"code\":400,\"msg\":\"unknown command\"}}";
  }
}
