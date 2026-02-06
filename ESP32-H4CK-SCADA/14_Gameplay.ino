// ============================================================
// 14_Gameplay.ino — Multi-Path Flag Logic + HMAC Storage
// ============================================================

#include "include/common.h"

// Path names (definition)
const char* pathNames[] = {
  "IDOR", "INJECTION", "RACE", "PHYSICS", "FORENSICS", "WEAK_AUTH"
};

// PlayerState full definition (depends on PATH_COUNT)
struct PlayerState {
  String    sessionId;
  String    username;
  UserRole  role;
  bool      pathsFound[PATH_COUNT];
  int       flagsSubmitted;
  int       hintsRequested;
  int       defenseEvasions;
  int       incidentsResolved;
  unsigned long startedAt;
  bool      active;
};

#define MAX_PLAYERS 16
PlayerState players[MAX_PLAYERS];
int playerCount = 0;

// Sub-flag HMACs (generated at runtime)
String subFlagHMACs[PATH_COUNT];

void gameplayInit() {
  Serial.println("[GAMEPLAY] Initializing gameplay engine...");
  memset(players, 0, sizeof(players));
  playerCount = 0;

  // Generate sub-flag HMACs
  for (int i = 0; i < PATH_COUNT; i++) {
    String flagContent = "ESP32-SCADA-" + String(pathNames[i]) + "-2026";
    subFlagHMACs[i] = hmacSHA256(flagContent, String(SECRET_KEY));
  }

  Serial.printf("[GAMEPLAY] %d exploit paths configured. Required: %d\n",
    PATH_COUNT, EXPLOITS_REQUIRED_TO_WIN);
}

// ===== Register player =====
void gameplayRegisterPlayer(const String& sessionId, const String& username, UserRole role) {
  // Check if already registered
  for (int i = 0; i < playerCount; i++) {
    if (players[i].username == username && players[i].active) {
      players[i].sessionId = sessionId;
      return;
    }
  }

  // Find slot
  int slot = -1;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (!players[i].active) { slot = i; break; }
  }
  if (slot < 0) slot = 0;  // Overwrite first

  players[slot].sessionId = sessionId;
  players[slot].username  = username;
  players[slot].role      = role;
  memset(players[slot].pathsFound, 0, sizeof(players[slot].pathsFound));
  players[slot].flagsSubmitted    = 0;
  players[slot].hintsRequested    = 0;
  players[slot].defenseEvasions   = 0;
  players[slot].incidentsResolved = 0;
  players[slot].startedAt         = millis();
  players[slot].active            = true;
  if (slot >= playerCount) playerCount = slot + 1;

  debugLogf("GAMEPLAY", "Player registered: %s (session: %s)",
    username.c_str(), sessionId.c_str());
}

// ===== Get player by session =====
PlayerState* getPlayer(const String& sessionId) {
  for (int i = 0; i < playerCount; i++) {
    if (players[i].active && players[i].sessionId == sessionId) return &players[i];
  }
  return nullptr;
}

PlayerState* getPlayerByName(const String& username) {
  for (int i = 0; i < playerCount; i++) {
    if (players[i].active && players[i].username == username) return &players[i];
  }
  return nullptr;
}

// ===== Submit flag =====
String submitFlag(const String& body, const String& sessionId, const String& ip) {
  JsonDocument doc;
  if (deserializeJson(doc, body)) return jsonError("Invalid JSON", 400);

  String flag = doc["flag"] | "";
  if (flag.isEmpty()) return jsonError("Missing flag", 400);

  PlayerState* player = getPlayer(sessionId);
  if (!player) return jsonError("Session not found. Please login first.", 401);

  player->flagsSubmitted++;

  JsonDocument result;
  result["flag_submitted"] = flag;

  // Check against known flag patterns
  bool found = false;
  int pathIdx = -1;

  // IDOR flags
  if (flag.indexOf("idor_cross_line") >= 0 || flag.indexOf("idor_alarm") >= 0) {
    pathIdx = PATH_IDOR;
  }
  // Injection flags
  else if (flag.indexOf("command_injection") >= 0) {
    pathIdx = PATH_INJECTION;
  }
  // Race condition flags
  else if (flag.indexOf("race_condition") >= 0) {
    pathIdx = PATH_RACE;
  }
  // Physics / sensor flags
  else if (flag.indexOf("sensor_fault") >= 0 || flag.indexOf("sensor_tamper") >= 0) {
    pathIdx = PATH_PHYSICS;
  }
  // Forensics flags
  else if (flag.indexOf("forensics") >= 0) {
    pathIdx = PATH_FORENSICS;
  }
  // Weak auth flags
  else if (flag.indexOf("debug_endpoint") >= 0 || flag.indexOf("env_leak") >= 0 ||
           flag.indexOf("honeypot") >= 0 || flag.indexOf("whoami") >= 0) {
    pathIdx = PATH_WEAK_AUTH;
  }
  // Incident sub-flags
  else if (flag.indexOf("STUCK_VALVE") >= 0 || flag.indexOf("SENSOR_FAULT") >= 0 ||
           flag.indexOf("MOTOR_OVERLOAD") >= 0) {
    // Accept as physics/forensics path
    pathIdx = PATH_PHYSICS;
  }
  // Deserialization
  else if (flag.indexOf("insecure_deser") >= 0) {
    pathIdx = PATH_FORENSICS;
  }

  if (pathIdx >= 0) {
    if (!player->pathsFound[pathIdx]) {
      player->pathsFound[pathIdx] = true;
      found = true;
      result["correct"]  = true;
      result["path"]     = pathNames[pathIdx];
      result["message"]  = "Exploit path discovered: " + String(pathNames[pathIdx]);
      result["new_discovery"] = true;

      logDefenseEvent("FLAG_SUBMIT", ip,
        "Path unlocked: " + String(pathNames[pathIdx]) + " by " + player->username);
    } else {
      result["correct"]  = true;
      result["path"]     = pathNames[pathIdx];
      result["message"]  = "Already discovered this path.";
      result["new_discovery"] = false;
    }
  } else {
    result["correct"] = false;
    result["message"] = "Invalid flag. Keep exploring!";
    logDefenseEvent("FLAG_FAIL", ip, "Invalid flag by " + player->username);
  }

  // Progress summary
  int totalFound = 0;
  for (int i = 0; i < PATH_COUNT; i++) {
    if (player->pathsFound[i]) totalFound++;
  }
  result["paths_found"]    = totalFound;
  result["paths_required"] = EXPLOITS_REQUIRED_TO_WIN;
  result["game_complete"]  = (totalFound >= EXPLOITS_REQUIRED_TO_WIN);

  if (totalFound >= EXPLOITS_REQUIRED_TO_WIN) {
    result["congratulations"] = "You have discovered enough exploit paths! Check the leaderboard.";
    result["root_flag_hint"]  = "Combine sub-flags using HMAC to derive root flag.";
  }

  String out;
  serializeJson(result, out);
  return out;
}

// ===== Submit root flag =====
String submitRootFlag(const String& body) {
  JsonDocument doc;
  if (deserializeJson(doc, body)) return jsonError("Invalid JSON", 400);

  String flag = doc["flag"] | "";
  String expectedHmac = hmacSHA256("ESP32-SCADA-ROOT-FLAG-2026", String(SECRET_KEY));

  JsonDocument result;
  if (flag == "FLAG{" + expectedHmac.substring(0, 16) + "}") {
    result["correct"] = true;
    result["message"] = "ROOT FLAG ACCEPTED! Full system compromise achieved.";
    result["root_flag_hmac"] = expectedHmac;
  } else {
    result["correct"] = false;
    result["message"] = "Invalid root flag.";
  }

  String out;
  serializeJson(result, out);
  return out;
}

// ===== Get player progress =====
String getPlayerProgress(const String& sessionId) {
  PlayerState* player = getPlayer(sessionId);
  if (!player) return "{\"authenticated\":false}";

  JsonDocument doc;
  doc["username"]  = player->username;
  doc["role"]      = roleToString(player->role);

  JsonObject paths = doc["paths"].to<JsonObject>();
  int totalFound = 0;
  for (int i = 0; i < PATH_COUNT; i++) {
    paths[pathNames[i]] = player->pathsFound[i];
    if (player->pathsFound[i]) totalFound++;
  }

  doc["paths_found"]         = totalFound;
  doc["paths_total"]         = PATH_COUNT;
  doc["paths_required"]      = EXPLOITS_REQUIRED_TO_WIN;
  doc["flags_submitted"]     = player->flagsSubmitted;
  doc["hints_requested"]     = player->hintsRequested;
  doc["incidents_resolved"]  = player->incidentsResolved;
  doc["defense_evasions"]    = player->defenseEvasions;
  doc["time_elapsed_sec"]    = (millis() - player->startedAt) / 1000;
  doc["game_complete"]       = (totalFound >= EXPLOITS_REQUIRED_TO_WIN);
  doc["score"]               = calculateScore(player);

  String out;
  serializeJson(doc, out);
  return out;
}

// ===== Calculate score =====
int calculateScore(PlayerState* player) {
  int score = BASE_SCORE;
  int pathsFound = 0;
  for (int i = 0; i < PATH_COUNT; i++) {
    if (player->pathsFound[i]) pathsFound++;
  }

  score += pathsFound * 500;
  int timeMin = (millis() - player->startedAt) / 60000;
  score += max(0, 3000 - timeMin * 5);
  score += player->incidentsResolved * 25;
  score += player->defenseEvasions * 100;
  score -= player->hintsRequested * 10;

  return max(0, score);
}

// ===== Gameplay update =====
void gameplayUpdate() {
  // Nothing periodic needed for now
}
