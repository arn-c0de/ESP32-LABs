// ============================================================
// 16_Leaderboard.ino — Scoring, Stats, JSON Export
// ============================================================

void leaderboardInit() {
  Serial.println("[LEADERBOARD] Initializing leaderboard...");
  Serial.printf("[LEADERBOARD] Scoring: %s | Enabled: %s\n",
    SCORING_MODE == SIMPLE ? "SIMPLE" : SCORING_MODE == WEIGHTED ? "WEIGHTED" : "TIERED",
    LEADERBOARD_ENABLED ? "YES" : "NO");
}

void leaderboardUpdate() {
  // Periodic cleanup or stat aggregation if needed
}

// ===== Get leaderboard =====
String getLeaderboard(const String& sortBy, int limit) {
  if (!LEADERBOARD_ENABLED) {
    return "{\"enabled\":false}";
  }

  // Build scored player list
  struct ScoredPlayer {
    String username;
    int    pathsFound;
    int    score;
    int    timeMin;
    String difficulty;
    bool   complete;
  };

  ScoredPlayer scored[MAX_PLAYERS];
  int scoredCount = 0;

  for (int i = 0; i < playerCount; i++) {
    if (!players[i].active) continue;

    ScoredPlayer& sp = scored[scoredCount++];
    sp.username = players[i].username;
    sp.score    = calculateScore(&players[i]);
    sp.timeMin  = (millis() - players[i].startedAt) / 60000;
    sp.difficulty = (DIFFICULTY == EASY) ? "EASY" :
                    (DIFFICULTY == NORMAL) ? "NORMAL" : "HARD";

    sp.pathsFound = 0;
    for (int p = 0; p < PATH_COUNT; p++) {
      if (players[i].pathsFound[p]) sp.pathsFound++;
    }
    sp.complete = (sp.pathsFound >= EXPLOITS_REQUIRED_TO_WIN);
  }

  // Sort by score (descending) - simple bubble sort
  for (int i = 0; i < scoredCount - 1; i++) {
    for (int j = 0; j < scoredCount - i - 1; j++) {
      bool swap = false;
      if (sortBy == "time") {
        swap = scored[j].timeMin > scored[j + 1].timeMin;
      } else if (sortBy == "paths") {
        swap = scored[j].pathsFound < scored[j + 1].pathsFound;
      } else {
        swap = scored[j].score < scored[j + 1].score;
      }
      if (swap) {
        ScoredPlayer tmp = scored[j];
        scored[j]     = scored[j + 1];
        scored[j + 1] = tmp;
      }
    }
  }

  // Build JSON
  JsonDocument doc;
  doc["enabled"]    = true;
  doc["sort"]       = sortBy.isEmpty() ? "score" : sortBy;
  doc["total_players"] = scoredCount;

  JsonArray arr = doc["leaderboard"].to<JsonArray>();
  int n = min(scoredCount, limit);
  for (int i = 0; i < n; i++) {
    JsonObject entry = arr.add<JsonObject>();
    entry["rank"]       = i + 1;
    entry["username"]   = scored[i].username;
    entry["difficulty"] = scored[i].difficulty;
    entry["exploits"]   = String(scored[i].pathsFound) + "/" + String(PATH_COUNT);
    entry["time_min"]   = scored[i].timeMin;
    entry["score"]      = scored[i].score;
    entry["complete"]   = scored[i].complete;
  }

  JsonObject scoring = doc["scoring_formula"].to<JsonObject>();
  scoring["base"]              = BASE_SCORE;
  scoring["per_exploit"]       = 500;
  scoring["time_bonus_max"]    = 3000;
  scoring["time_penalty_rate"] = 5;
  scoring["per_incident"]      = 25;
  scoring["per_evasion"]       = 100;
  scoring["per_hint_penalty"]  = -10;

  String out;
  serializeJson(doc, out);
  return out;
}

// ===== Serial command handler =====
void serialCommandHandler() {
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.isEmpty()) return;

  debugLogf("SERIAL", "Command: %s", input.c_str());

  // ===== Help =====
  if (input == "help") {
    Serial.println();
    Serial.println("=== ESP32-SCADA Serial Commands ===");
    Serial.println("help                          - Show this help");
    Serial.println("status                        - System status");
    Serial.println("wifi                          - WiFi status & config");
    Serial.println("defense status                - Defense module status");
    Serial.println("iptables -A INPUT -s <IP> -j DROP --duration <sec>");
    Serial.println("                              - Block IP");
    Serial.println("iptables -D INPUT -s <IP>     - Unblock IP");
    Serial.println("tc qdisc add rate-limit --src 0.0.0.0/0 --duration <sec> rate=<N>-per-min");
    Serial.println("                              - Set rate limit");
    Serial.println("session reset --ip <IP> --reason <text>");
    Serial.println("                              - Force logout IP");
    Serial.println("defense config set <key>=<val> - Set defense config");
    Serial.println("incident create <TYPE> <LINE> - Create incident");
    Serial.println("leaderboard                   - Show leaderboard");
    Serial.println("players                       - Show active players");
    Serial.println("config                        - Show current config");
    Serial.println("difficulty <EASY|NORMAL|HARD> - Change difficulty");
    Serial.println("emergency stop                - Emergency stop all");
    Serial.println("emergency reset               - Reset emergency");
    Serial.println("save                          - Force save to LittleFS");
    Serial.println("reboot                        - Restart ESP32");
    Serial.println();
    return;
  }

  // ===== System status =====
  if (input == "status") {
    Serial.println();
    Serial.println("[STATUS] System Info:");
    Serial.print("  Uptime: "); Serial.print(millis() / 1000); Serial.println(" sec");
    Serial.print("  Free Heap: "); Serial.print(ESP.getFreeHeap()); Serial.println(" bytes");
    
    Serial.println("[STATUS] Network:");
    if (WiFi.getMode() & WIFI_MODE_AP) {
      Serial.print("  AP: "); Serial.print(AP_SSID); 
      Serial.print(" @ "); Serial.println(WiFi.softAPIP());
      Serial.print("  Clients: "); Serial.println(WiFi.softAPgetStationNum());
    }
    if (WiFi.getMode() & WIFI_MODE_STA) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("  STA: Connected to "); Serial.println(WiFi.SSID());
        Serial.print("  IP: "); Serial.println(WiFi.localIP());
      } else {
        Serial.println("  STA: Not connected");
      }
    }
    
    Serial.println("[STATUS] SCADA:");
    Serial.print("  Active Alarms: "); Serial.println(getActiveAlarmCount());
    Serial.print("  Active Incidents: "); Serial.println(getActiveIncidentCount());
    Serial.print("  Active Players: "); Serial.println(playerCount);
    
    const char* diffStr = DIFFICULTY == EASY ? "EASY" : 
                          DIFFICULTY == NORMAL ? "NORMAL" : "HARD";
    Serial.print("  Difficulty: "); Serial.println(diffStr);
    Serial.println();
    return;
  }

  // ===== WiFi status =====
  if (input == "wifi") {
    Serial.println();
    Serial.println("[WIFI] Configuration:");
    Serial.print("  Mode: ");
    if (WIFI_AP_MODE && WIFI_STA_MODE) Serial.println("AP + STA");
    else if (WIFI_AP_MODE) Serial.println("AP only");
    else if (WIFI_STA_MODE) Serial.println("STA only");
    else Serial.println("None");
    
    if (WIFI_AP_MODE) {
      Serial.println("[WIFI] Access Point:");
      Serial.print("  SSID: "); Serial.println(AP_SSID);
      Serial.print("  Password: "); Serial.println(AP_PASSWORD);
      IPAddress apIP = WiFi.softAPIP();
      Serial.print("  IP: "); Serial.println(apIP.toString());
      Serial.print("  Connected Clients: "); Serial.println(WiFi.softAPgetStationNum());
    }
    
    if (WIFI_STA_MODE) {
      Serial.println("[WIFI] Station Mode:");
      Serial.print("  Target SSID: "); Serial.println(WIFI_SSID);
      Serial.print("  Status: ");
      wl_status_t status = WiFi.status();
      if (status == WL_CONNECTED) {
        Serial.println("Connected");
        IPAddress localIP = WiFi.localIP();
        IPAddress gwIP = WiFi.gatewayIP();
        Serial.print("  IP: "); Serial.println(localIP.toString());
        Serial.print("  Gateway: "); Serial.println(gwIP.toString());
        int rssi = WiFi.RSSI();
        Serial.print("  RSSI: "); Serial.print(rssi); Serial.println(" dBm");
      } else {
        Serial.println("Not connected");
      }
    }
    
    Serial.println("[WIFI] Web Interface:");
    if (WIFI_STA_MODE && WiFi.status() == WL_CONNECTED) {
      IPAddress ip = WiFi.localIP();
      Serial.print("  http://"); Serial.println(ip.toString());
    } else if (WIFI_AP_MODE) {
      IPAddress apIP = WiFi.softAPIP();
      Serial.print("  http://"); Serial.println(apIP.toString());
      Serial.print("  (Connect to AP: "); Serial.print(AP_SSID); Serial.println(")");
    }
    Serial.println();
    return;
  }

  // ===== Defense status =====
  if (input == "defense status") {
    Serial.printf("\n[DEFENSE] Enabled: %s\n", DEFENSE_ENABLED ? "YES" : "NO");
    Serial.printf("[DEFENSE] DP: %d/%d | AP: %d/10 | Stability: %d/100\n",
      defensePoints, DEFENSE_POINTS_INITIAL, actionPoints, stabilityScore);
    Serial.printf("[DEFENSE] IDS: %s | WAF: %s | Rate Limit: %s (%d/min)\n",
      IDS_ACTIVE ? "ON" : "OFF", WAF_ACTIVE ? "ON" : "OFF",
      RATE_LIMIT_ACTIVE ? "ON" : "OFF", RATE_LIMIT_PER_MINUTE);
    Serial.printf("[DEFENSE] Blocked IPs: %d\n", blockedIPCount);
    for (int i = 0; i < blockedIPCount; i++) {
      if (blockedIPs[i].active) {
        int remaining = blockedIPs[i].durationSec -
          (int)((millis() - blockedIPs[i].blockedAt) / 1000);
        Serial.printf("  - %s (%ds remaining)\n",
          blockedIPs[i].ip.c_str(), max(0, remaining));
      }
    }
    Serial.println();
    return;
  }

  // ===== IP blocking =====
  if (input.startsWith("iptables -A INPUT -s ")) {
    // Parse: iptables -A INPUT -s <IP> -j DROP --duration <sec>
    String rest = input.substring(21);
    int jIdx = rest.indexOf(" -j DROP");
    if (jIdx > 0) {
      String ip = rest.substring(0, jIdx);
      ip.trim();
      int duration = BLOCK_DURATION_SEC;
      int durIdx = rest.indexOf("--duration ");
      if (durIdx > 0) {
        duration = rest.substring(durIdx + 11).toInt();
      }
      blockIP(ip, duration);
      Serial.printf("[DEFENSE] Blocked %s for %d seconds.\n", ip.c_str(), duration);
    }
    return;
  }

  if (input.startsWith("iptables -D INPUT -s ")) {
    String ip = input.substring(21);
    ip.trim();
    unblockIP(ip);
    Serial.printf("[DEFENSE] Unblocked %s.\n", ip.c_str());
    return;
  }

  // ===== Rate limiting =====
  if (input.startsWith("tc qdisc add rate-limit")) {
    int rateIdx = input.indexOf("rate=");
    if (rateIdx > 0) {
      String rateStr = input.substring(rateIdx + 5);
      int dashIdx = rateStr.indexOf("-per-min");
      if (dashIdx > 0) rateStr = rateStr.substring(0, dashIdx);
      int rate = rateStr.toInt();
      if (rate > 0) {
        setGlobalRateLimit(rate);
        Serial.printf("[DEFENSE] Rate limit set to %d req/min.\n", rate);
      }
    }
    return;
  }

  // ===== Session reset =====
  if (input.startsWith("session reset --ip ")) {
    String rest = input.substring(19);
    int reasonIdx = rest.indexOf(" --reason ");
    String ip = (reasonIdx > 0) ? rest.substring(0, reasonIdx) : rest;
    String reason = (reasonIdx > 0) ? rest.substring(reasonIdx + 10) : "admin";
    ip.trim();
    reason.trim();
    resetSessionsForIP(ip, reason);
    Serial.printf("[DEFENSE] Sessions reset for %s: %s\n", ip.c_str(), reason.c_str());
    return;
  }

  // ===== Incident creation =====
  if (input.startsWith("incident create ")) {
    String rest = input.substring(16);
    int spaceIdx = rest.indexOf(' ');
    String typeStr = (spaceIdx > 0) ? rest.substring(0, spaceIdx) : rest;
    int line = (spaceIdx > 0) ? rest.substring(spaceIdx + 1).toInt() : 1;
    if (line < 1 || line > NUM_LINES) line = 1;

    IncidentType type = INC_STUCK_VALVE;
    for (int i = 0; i < 7; i++) {
      if (typeStr == incidentTypeNames[i]) { type = (IncidentType)i; break; }
    }
    createIncident(type, line, INC_SEV_HIGH, 0);
    Serial.printf("[INCIDENTS] Created %s on Line %d.\n", typeStr.c_str(), line);
    return;
  }

  // ===== Leaderboard =====
  if (input == "leaderboard") {
    Serial.println("\n=== Leaderboard ===");
    Serial.println("Rank | Username   | Difficulty | Exploits | Time | Score");
    Serial.println("-----|------------|------------|----------|------|------");
    for (int i = 0; i < playerCount; i++) {
      if (!players[i].active) continue;
      int paths = 0;
      for (int p = 0; p < PATH_COUNT; p++) {
        if (players[i].pathsFound[p]) paths++;
      }
      Serial.printf("%-4d | %-10s | %-10s | %d/%d      | %dm   | %d\n",
        i + 1, players[i].username.c_str(),
        DIFFICULTY == EASY ? "EASY" : DIFFICULTY == NORMAL ? "NORMAL" : "HARD",
        paths, PATH_COUNT,
        (int)((millis() - players[i].startedAt) / 60000),
        calculateScore(&players[i]));
    }
    Serial.println();
    return;
  }

  // ===== Players =====
  if (input == "players") {
    Serial.printf("\n[PLAYERS] Active: %d\n", playerCount);
    for (int i = 0; i < playerCount; i++) {
      if (!players[i].active) continue;
      int paths = 0;
      for (int p = 0; p < PATH_COUNT; p++) {
        if (players[i].pathsFound[p]) paths++;
      }
      Serial.printf("  %s (session: %s, role: %s, paths: %d/%d)\n",
        players[i].username.c_str(), players[i].sessionId.c_str(),
        roleToString(players[i].role), paths, PATH_COUNT);
    }
    Serial.println();
    return;
  }

  // ===== Config =====
  if (input == "config") {
    Serial.println(configGetJson());
    return;
  }

  // ===== Difficulty change =====
  if (input.startsWith("difficulty ")) {
    String d = input.substring(11);
    d.trim();
    d.toUpperCase();
    if (d == "EASY") DIFFICULTY = EASY;
    else if (d == "NORMAL") DIFFICULTY = NORMAL;
    else if (d == "HARD") DIFFICULTY = HARD;
    else { Serial.println("Invalid. Use: EASY, NORMAL, HARD"); return; }
    configInit();
    Serial.printf("[CONFIG] Difficulty changed to %s.\n", d.c_str());
    return;
  }

  // ===== Emergency =====
  if (input == "emergency stop") {
    emergencyStopAll();
    return;
  }
  if (input == "emergency reset") {
    resetEmergencyStop();
    return;
  }

  // ===== Defense config =====
  if (input.startsWith("defense config set ")) {
    String kv = input.substring(19);
    int eqIdx = kv.indexOf('=');
    if (eqIdx > 0) {
      String key = kv.substring(0, eqIdx);
      String val = kv.substring(eqIdx + 1);
      key.trim(); val.trim();

      if (key == "ids") IDS_ACTIVE = (val == "on" || val == "true");
      else if (key == "waf") WAF_ACTIVE = (val == "on" || val == "true");
      else if (key == "rate_limit") RATE_LIMIT_ACTIVE = (val == "on" || val == "true");
      else if (key == "ip_blocking") IP_BLOCKING_ENABLED = (val == "on" || val == "true");
      else if (key == "honeypot_alerts") HONEYPOT_ENDPOINTS_ENABLED = (val == "on" || val == "true");
      else if (key == "defense") DEFENSE_ENABLED = (val == "on" || val == "true");
      else { Serial.println("Unknown key: " + key); return; }

      Serial.printf("[DEFENSE] %s = %s\n", key.c_str(), val.c_str());
    }
    return;
  }

  // ===== Save =====
  if (input == "save") {
    databaseAutoSave();
    Serial.println("[DB] Manual save complete.");
    return;
  }

  // ===== Reboot =====
  if (input == "reboot") {
    Serial.println("[SYSTEM] Rebooting...");
    delay(500);
    ESP.restart();
    return;
  }

  Serial.println("Unknown command. Type 'help' for available commands.");
}
