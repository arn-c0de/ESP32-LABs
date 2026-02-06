/*
 * HTTP Helper Functions
 */

void addCORSHeaders(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(200);
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  request->send(response);
}
