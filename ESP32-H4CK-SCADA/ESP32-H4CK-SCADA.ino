/*
 * ESP32-SCADA - Professional Industrial SCADA Security Lab
 * =========================================================
 * Red Team focused SCADA training platform for ESP32.
 * 4 production lines, 20+ sensors, 6 exploit paths.
 *
 * Version: 2.0
 * Platform: ESP32
 * License: Educational Use Only
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <mbedtls/md.h>
#include "include/common.h"

// External WebSocket object (defined in 03_WebServer.ino)
extern AsyncWebSocket ws;

// Forward declarations for all modules
void configInit();
void wifiInit();
void webServerInit();
void authInit();
void databaseInit();
void physicsInit();
void sensorsInit();
void actuatorsInit();
void alarmsInit();
void safetyInit();
void incidentsInit();
void defenseInit();
void vulnerabilitiesInit();
void gameplayInit();
void hintsInit();
void leaderboardInit();

void physicsUpdate();
void sensorsUpdate();
void actuatorsUpdate();
void alarmsUpdate();
void safetyUpdate();
void incidentsUpdate();
void defenseUpdate();
void gameplayUpdate();
void leaderboardUpdate();
void serialCommandHandler();
void databaseAutoSave();

// Global timing
unsigned long lastSensorUpdate = 0;
unsigned long lastAlarmCheck = 0;
unsigned long lastIncidentCheck = 0;
unsigned long lastDefenseUpdate = 0;
unsigned long lastAutoSave = 0;
unsigned long lastPhysicsUpdate = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("╔══════════════════════════════════════════╗");
  Serial.println("║   ESP32-SCADA Industrial Security Lab    ║");
  Serial.println("║   Version 2.0 | Red Team Training        ║");
  Serial.println("╚══════════════════════════════════════════╝");
  Serial.println();

  // Phase 1: Core systems
  configInit();
  databaseInit();

  // Phase 2: Network
  wifiInit();
  authInit();

  // Phase 3: Industrial simulation
  physicsInit();
  sensorsInit();
  actuatorsInit();
  alarmsInit();
  safetyInit();

  // Phase 4: Gameplay
  incidentsInit();
  defenseInit();
  vulnerabilitiesInit();
  gameplayInit();
  hintsInit();
  leaderboardInit();

  // Phase 5: Web interface will be initialized in loop()
  Serial.println("[SETUP] Core initialization complete. Web server will start shortly...");

  Serial.println();
  Serial.println("[INFO]  Type 'help' in Serial for commands.");
  Serial.println();
}

void loop() {
  // Deferred initialization - run once after setup() completes
  static bool deferredInitDone = false;
  if (!deferredInitDone) {
    delay(1000); // Give WiFi time to fully stabilize
    
    Serial.println();
    Serial.println("[DEFERRED INIT] Starting web server...");
    
    // Initialize WebServer
    webServerInit();
    yield();
    delay(200);
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("  System Ready!");
    Serial.println("========================================");
    Serial.printf("  Access Point: ESP32-H4CK-AP\n");
    Serial.printf("  Password: vulnerable\n");
    Serial.printf("  Web: http://%s/\n", wifiGetIP().c_str());
    Serial.printf("  Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.println("========================================");
    Serial.println();
    
    deferredInitDone = true;
    return; // Skip normal loop processing on first iteration
  }

  unsigned long now = millis();

  // WebSocket cleanup: every 5s to prevent memory leaks
  static unsigned long lastWsCleanup = 0;
  if (now - lastWsCleanup >= 5000) {
    lastWsCleanup = now;
    ws.cleanupClients();
    yield();
  }

  // Physics engine: every 100ms
  if (now - lastPhysicsUpdate >= 100) {
    lastPhysicsUpdate = now;
    physicsUpdate();
    yield();
  }

  // Sensor readings: every 2s
  if (now - lastSensorUpdate >= 2000) {
    lastSensorUpdate = now;
    sensorsUpdate();
    yield();
    actuatorsUpdate();
    yield();
  }

  // Alarm checks: every 5s
  if (now - lastAlarmCheck >= 5000) {
    lastAlarmCheck = now;
    alarmsUpdate();
    yield();
    safetyUpdate();
    yield();
  }

  // Incident engine: every 10s
  if (now - lastIncidentCheck >= 10000) {
    lastIncidentCheck = now;
    incidentsUpdate();
    yield();
  }

  // Defense update: every 5s
  if (now - lastDefenseUpdate >= 5000) {
    lastDefenseUpdate = now;
    defenseUpdate();
    yield();
  }

  // Gameplay & leaderboard: every 30s
  static unsigned long lastGameUpdate = 0;
  if (now - lastGameUpdate >= 30000) {
    lastGameUpdate = now;
    gameplayUpdate();
    yield();
    leaderboardUpdate();
    yield();
  }

  // Auto-save to LittleFS: every 60s
  if (now - lastAutoSave >= 60000) {
    lastAutoSave = now;
    databaseAutoSave();
    yield();
  }

  // Memory monitoring - restart if critically low
  static unsigned long lastMemCheck = 0;
  if (now - lastMemCheck >= 10000) {
    lastMemCheck = now;
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 15000) {
      Serial.printf("[CRITICAL] Low memory: %d bytes. Restarting...\n", freeHeap);
      delay(1000);
      ESP.restart();
    }
  }

  // Serial command handler (non-blocking)
  if (Serial.available()) {
    serialCommandHandler();
  }

  // Feed watchdog and yield
  yield();
  delay(10);
}
