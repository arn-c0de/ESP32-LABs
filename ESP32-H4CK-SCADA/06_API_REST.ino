/*
 * DEPRECATED: REST API Module (SNS Banking System)
 * 
 * This module is disabled for SCADA.
 * See 06_API_SCADA.ino for the new SCADA-specific API.
 */

// Stub functions to prevent linker errors - not used in SCADA
void setupRESTRoutes() {
  // Intentionally left empty - all SCADA API routes are in 06_API_SCADA.ino
}

void handleGetSystemInfo(AsyncWebServerRequest *request) {
  // Stub - not used in SCADA
}

void handleGetUsers(AsyncWebServerRequest *request) {
  // Stub - not used in SCADA
}

void handlePostUser(AsyncWebServerRequest *request) {
  // Stub - not used in SCADA
}

void handleDeleteUser(AsyncWebServerRequest *request) {
  // Stub - not used in SCADA
}

void handlePutUser(AsyncWebServerRequest *request) {
  // Stub - not used in SCADA
}
