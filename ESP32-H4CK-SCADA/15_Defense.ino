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
// Note: All Defense structures are declared in main .ino file

// Rate limiting tracking (legacy, for defense rules)
struct RateLimitEntry {
  String ip;
  int requestCount;
  unsigned long windowStart;
  int maxRequests;
};

// ===== GLOBAL STATE (declared in main .ino file) =====
extern DefenseConfig defenseConfig;
extern DefenseRule activeRules[32];
extern int activeRuleCount;

// Local state
String recentRequestIds[32];  // For idempotency
int recentIdCount = 0;
RateLimitEntry rateLimits[16]; // Track rate limits per IP (legacy)
int rateLimitCount = 0;

TokenBucket tokenBuckets[64];
BlockEntry blockEntries[64];
NotFoundEntry notFoundEntries[32];
LoginBackoffEntry loginBackoff[32];
AlertEntry defenseAlerts[64];
int defenseAlertIndex = 0;

static const uint16_t TOKEN_BUCKET_RATE = 5;   // requests per second
static const uint16_t TOKEN_BUCKET_BURST = 10; // burst size
static const int VIOLATION_THRESHOLD = 3;
static const unsigned long VIOLATION_QUIET_MS = 300000; // 5 minutes

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
  for (int i = 0; i < 64; i++) {
    tokenBuckets[i].ip = "";
    tokenBuckets[i].tokens = TOKEN_BUCKET_BURST;
    tokenBuckets[i].lastRefill = 0;
    tokenBuckets[i].rate = TOKEN_BUCKET_RATE;
    tokenBuckets[i].burst = TOKEN_BUCKET_BURST;
    tokenBuckets[i].lastSeen = 0;
    tokenBuckets[i].violations = 0;
    tokenBuckets[i].blockLevel = 0;
    tokenBuckets[i].lastViolation = 0;
  }
  for (int i = 0; i < 64; i++) {
    blockEntries[i].ip = "";
    blockEntries[i].expiresAt = 0;
    blockEntries[i].permanent = false;
    blockEntries[i].reason = "";
    blockEntries[i].by = "";
    blockEntries[i].createdAt = 0;
  }
  for (int i = 0; i < 32; i++) {
    notFoundEntries[i].ip = "";
    notFoundEntries[i].count = 0;
    notFoundEntries[i].windowStart = 0;
    notFoundEntries[i].lastSeen = 0;
    loginBackoff[i].ip = "";
    loginBackoff[i].failures = 0;
    loginBackoff[i].lastFailure = 0;
    loginBackoff[i].lockedUntil = 0;
  }
  for (int i = 0; i < 64; i++) {
    defenseAlerts[i].timestamp = 0;
    defenseAlerts[i].type = "";
    defenseAlerts[i].ip = "";
    defenseAlerts[i].details = "";
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

uint32_t ipToUint(const String &ip) {
  int parts[4] = {0, 0, 0, 0};
  if (sscanf(ip.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) != 4) {
    return 0;
  }
  for (int i = 0; i < 4; i++) {
    if (parts[i] < 0 || parts[i] > 255) return 0;
  }
  return ((uint32_t)parts[0] << 24) | ((uint32_t)parts[1] << 16) | ((uint32_t)parts[2] << 8) | (uint32_t)parts[3];
}

bool ipMatchesTarget(const String &ip, const String &target) {
  if (target == "0.0.0.0/0") return true;
  int slash = target.indexOf('/');
  if (slash < 0) {
    return ip == target;
  }
  String base = target.substring(0, slash);
  String prefixStr = target.substring(slash + 1);
  int prefix = prefixStr.toInt();
  if (prefix < 0 || prefix > 32) return false;
  uint32_t ipVal = ipToUint(ip);
  uint32_t baseVal = ipToUint(base);
  if (ipVal == 0 || baseVal == 0) return false;
  uint32_t mask = (prefix == 0) ? 0 : (0xFFFFFFFFu << (32 - prefix));
  return (ipVal & mask) == (baseVal & mask);
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

void addDefenseAlert(const String &type, const String &ip, const String &details) {
  defenseAlerts[defenseAlertIndex].timestamp = millis();
  defenseAlerts[defenseAlertIndex].type = type;
  defenseAlerts[defenseAlertIndex].ip = ip;
  defenseAlerts[defenseAlertIndex].details = details;
  defenseAlertIndex = (defenseAlertIndex + 1) % 64;
}

String getDefenseAlertsJSON() {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.createNestedArray("alerts");
  for (int i = 0; i < 64; i++) {
    int idx = (defenseAlertIndex + i) % 64;
    if (defenseAlerts[idx].timestamp == 0) continue;
    JsonObject a = arr.createNestedObject();
    a["timestamp"] = defenseAlerts[idx].timestamp;
    a["type"] = defenseAlerts[idx].type;
    a["ip"] = defenseAlerts[idx].ip;
    a["details"] = defenseAlerts[idx].details;
  }
  String output;
  serializeJson(doc, output);
  return output;
}

int findTokenBucketIndex(const String &ip) {
  for (int i = 0; i < 64; i++) {
    if (tokenBuckets[i].ip == ip) return i;
  }
  return -1;
}

int allocateTokenBucketSlot(const String &ip) {
  int empty = -1;
  unsigned long oldest = 0xFFFFFFFFUL;
  int oldestIdx = -1;
  for (int i = 0; i < 64; i++) {
    if (tokenBuckets[i].ip == "") {
      empty = i;
      break;
    }
    if (tokenBuckets[i].lastSeen < oldest) {
      oldest = tokenBuckets[i].lastSeen;
      oldestIdx = i;
    }
  }
  int idx = (empty >= 0) ? empty : oldestIdx;
  tokenBuckets[idx].ip = ip;
  tokenBuckets[idx].tokens = TOKEN_BUCKET_BURST;
  tokenBuckets[idx].lastRefill = millis();
  tokenBuckets[idx].rate = TOKEN_BUCKET_RATE;
  tokenBuckets[idx].burst = TOKEN_BUCKET_BURST;
  tokenBuckets[idx].lastSeen = millis();
  tokenBuckets[idx].violations = 0;
  tokenBuckets[idx].blockLevel = 0;
  tokenBuckets[idx].lastViolation = 0;
  return idx;
}

void refillTokenBucket(TokenBucket &bucket) {
  unsigned long now = millis();
  if (bucket.lastRefill == 0) {
    bucket.lastRefill = now;
    return;
  }
  unsigned long elapsedMs = now - bucket.lastRefill;
  if (elapsedMs < 1000) return;
  unsigned long addTokens = (elapsedMs / 1000) * bucket.rate;
  if (addTokens > 0) {
    bucket.tokens = min((int)bucket.burst, bucket.tokens + (int)addTokens);
    bucket.lastRefill = now;
  }
}

void pruneExpiredBlocks() {
  unsigned long now = millis();
  for (int i = 0; i < 64; i++) {
    if (blockEntries[i].ip != "" && !blockEntries[i].permanent && blockEntries[i].expiresAt > 0 && now >= blockEntries[i].expiresAt) {
      blockEntries[i].ip = "";
      blockEntries[i].expiresAt = 0;
      blockEntries[i].permanent = false;
      blockEntries[i].reason = "";
      blockEntries[i].by = "";
      blockEntries[i].createdAt = 0;
    }
  }
}

void addBlock(const String &ip, unsigned long seconds, bool permanent, String by) {
  pruneExpiredBlocks();
  unsigned long now = millis();
  String reason = (by == "system") ? "auto-rate-limit" : "manual";
  if (permanent) {
    Serial.printf("[DEFENSE] Block added (permanent): %s by %s\n", ip.c_str(), by.c_str());
  } else {
    Serial.printf("[DEFENSE] Block added (%lus): %s by %s\n", seconds, ip.c_str(), by.c_str());
  }
  addDefenseAlert("block", ip, reason);
  // Update existing entry if present
  for (int i = 0; i < 64; i++) {
    if (blockEntries[i].ip == ip) {
      if (blockEntries[i].permanent) return;
      blockEntries[i].permanent = permanent;
      blockEntries[i].expiresAt = permanent ? 0 : now + (seconds * 1000);
      blockEntries[i].reason = reason;
      blockEntries[i].by = by;
      blockEntries[i].createdAt = now;
      return;
    }
  }

  int empty = -1;
  unsigned long oldest = 0xFFFFFFFFUL;
  int oldestIdx = -1;
  for (int i = 0; i < 64; i++) {
    if (blockEntries[i].ip == "") {
      empty = i;
      break;
    }
    if (!blockEntries[i].permanent && blockEntries[i].createdAt < oldest) {
      oldest = blockEntries[i].createdAt;
      oldestIdx = i;
    }
  }
  int idx = (empty >= 0) ? empty : oldestIdx;
  blockEntries[idx].ip = ip;
  blockEntries[idx].permanent = permanent;
  blockEntries[idx].expiresAt = permanent ? 0 : now + (seconds * 1000);
  blockEntries[idx].reason = reason;
  blockEntries[idx].by = by;
  blockEntries[idx].createdAt = now;
}

void removeBlock(const String &ip) {
  for (int i = 0; i < 64; i++) {
    if (blockEntries[i].ip == ip) {
      blockEntries[i].ip = "";
      blockEntries[i].expiresAt = 0;
      blockEntries[i].permanent = false;
      blockEntries[i].reason = "";
      blockEntries[i].by = "";
      blockEntries[i].createdAt = 0;
      Serial.printf("[DEFENSE] Block removed: %s\n", ip.c_str());
      addDefenseAlert("unblock", ip, "manual");
      return;
    }
  }
}

bool isBlockedByEntry(const String &ip) {
  pruneExpiredBlocks();
  for (int i = 0; i < 64; i++) {
    if (blockEntries[i].ip == ip) {
      if (blockEntries[i].permanent) return true;
      if (blockEntries[i].expiresAt == 0) return true;
      if (millis() < blockEntries[i].expiresAt) return true;
    }
  }
  return false;
}

void recordViolation(TokenBucket &bucket, const String &ip) {
  unsigned long now = millis();
  if (bucket.lastViolation > 0 && (now - bucket.lastViolation) > VIOLATION_QUIET_MS) {
    bucket.violations = 0;
  }
  bucket.lastViolation = now;
  bucket.violations++;
  addDefenseAlert("rate_limit", ip, "token bucket exceeded");
  if (bucket.violations >= VIOLATION_THRESHOLD) {
    if (bucket.blockLevel == 0) {
      addBlock(ip, 300, false, "system");
      bucket.blockLevel = 1;
    } else if (bucket.blockLevel == 1) {
      addBlock(ip, 1800, false, "system");
      bucket.blockLevel = 2;
    } else {
      addBlock(ip, 0, true, "system");
      bucket.blockLevel = 3;
    }
    bucket.violations = 0;
  }
}

bool consumeTokens(const String &ip, int cost) {
  if (ip == "") return true;
  if (isBlockedByEntry(ip)) return false;

  int idx = findTokenBucketIndex(ip);
  if (idx < 0) {
    idx = allocateTokenBucketSlot(ip);
  }
  TokenBucket &bucket = tokenBuckets[idx];
  bucket.lastSeen = millis();
  refillTokenBucket(bucket);

  if (bucket.tokens >= cost) {
    bucket.tokens -= cost;
    return true;
  }

  recordViolation(bucket, ip);
  return false;
}

bool checkNotFoundBackoff(const String &ip) {
  unsigned long now = millis();
  int idx = -1;
  for (int i = 0; i < 32; i++) {
    if (notFoundEntries[i].ip == ip) {
      idx = i;
      break;
    }
  }
  if (idx < 0) {
    for (int i = 0; i < 32; i++) {
      if (notFoundEntries[i].ip == "") {
        idx = i;
        notFoundEntries[i].ip = ip;
        notFoundEntries[i].count = 0;
        notFoundEntries[i].windowStart = now;
        notFoundEntries[i].lastSeen = now;
        break;
      }
    }
  }
  if (idx < 0) return true;
  NotFoundEntry &entry = notFoundEntries[idx];
  entry.lastSeen = now;
  if (now - entry.windowStart > 1000) {
    entry.windowStart = now;
    entry.count = 0;
  }
  entry.count++;
  return entry.count <= 20;
}

bool checkLoginBackoff(const String &ip) {
  unsigned long now = millis();
  for (int i = 0; i < 32; i++) {
    if (loginBackoff[i].ip == ip) {
      if (loginBackoff[i].lockedUntil > now) return false;
      return true;
    }
  }
  return true;
}

void recordLoginFailure(const String &ip) {
  unsigned long now = millis();
  int idx = -1;
  for (int i = 0; i < 32; i++) {
    if (loginBackoff[i].ip == ip) {
      idx = i;
      break;
    }
  }
  if (idx < 0) {
    for (int i = 0; i < 32; i++) {
      if (loginBackoff[i].ip == "") {
        idx = i;
        loginBackoff[i].ip = ip;
        loginBackoff[i].failures = 0;
        loginBackoff[i].lastFailure = 0;
        loginBackoff[i].lockedUntil = 0;
        break;
      }
    }
  }
  if (idx < 0) return;
  LoginBackoffEntry &entry = loginBackoff[idx];
  entry.failures++;
  entry.lastFailure = now;
  if (entry.failures >= 5) {
    entry.lockedUntil = now + 30000; // 30s backoff
    entry.failures = 0;
    addDefenseAlert("login_backoff", ip, "backoff applied (30s)");
  }
}

void resetLoginFailures(const String &ip) {
  for (int i = 0; i < 32; i++) {
    if (loginBackoff[i].ip == ip) {
      loginBackoff[i].failures = 0;
      loginBackoff[i].lastFailure = 0;
      loginBackoff[i].lockedUntil = 0;
      return;
    }
  }
}

bool tryReserveConnection(const String &ip) {
  (void)ip;
  if (activeConnections >= MAX_HTTP_CONNECTIONS) {
    return false;
  }
  activeConnections++;
  return true;
}

void releaseConnection() {
  if (activeConnections > 0) {
    activeConnections--;
  }
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
  
  if (now - lastRegen > 10000) { // Every 10 seconds
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
  if (isBlockedByEntry(ip)) {
    return true;
  }
  for (int i = 0; i < 32; i++) {
    if (activeRules[i].active && 
        activeRules[i].type == DEFENSE_IP_BLOCK && 
        ipMatchesTarget(ip, activeRules[i].target)) {
      return true;
    }
  }
  return false;
}

bool checkRateLimit(String ip) {
  if (!consumeTokens(ip, 1)) {
    return false;
  }

  // Find or create legacy rate limit entry (for defense rules)
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
        ipMatchesTarget(ip, activeRules[i].target)) {
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

String handleAdminDefenseStatus() {
  DynamicJsonDocument doc(8192);
  doc["status"] = "ok";

  JsonArray blocks = doc.createNestedArray("blocks");
  unsigned long now = millis();
  for (int i = 0; i < 64; i++) {
    if (blockEntries[i].ip == "") continue;
    if (!blockEntries[i].permanent && blockEntries[i].expiresAt > 0 && now >= blockEntries[i].expiresAt) continue;
    JsonObject b = blocks.createNestedObject();
    b["ip"] = blockEntries[i].ip;
    b["permanent"] = blockEntries[i].permanent;
    b["reason"] = blockEntries[i].reason;
    b["by"] = blockEntries[i].by;
    b["created_at"] = blockEntries[i].createdAt;
    b["expires_at"] = blockEntries[i].expiresAt;
    if (blockEntries[i].permanent) {
      b["remaining_s"] = -1;
    } else if (blockEntries[i].expiresAt > now) {
      b["remaining_s"] = (blockEntries[i].expiresAt - now) / 1000;
    } else {
      b["remaining_s"] = 0;
    }
  }

  JsonArray buckets = doc.createNestedArray("buckets");
  for (int i = 0; i < 64; i++) {
    if (tokenBuckets[i].ip == "") continue;
    JsonObject t = buckets.createNestedObject();
    t["ip"] = tokenBuckets[i].ip;
    t["tokens"] = tokenBuckets[i].tokens;
    t["rate"] = tokenBuckets[i].rate;
    t["burst"] = tokenBuckets[i].burst;
    t["last_seen"] = tokenBuckets[i].lastSeen;
    t["violations"] = tokenBuckets[i].violations;
    t["block_level"] = tokenBuckets[i].blockLevel;
    t["last_violation"] = tokenBuckets[i].lastViolation;
  }

  String output;
  serializeJson(doc, output);
  return output;
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
