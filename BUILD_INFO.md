# Build & Tooling Information (auto-generated)

**Date:** 2026-02-06
**Repository:** ESP32-LABs (root)

---

## Quick Overview ✅
- Arduino ESP32 Core: **esp32:esp32 2.0.10 (installed)** — newer version available: **3.3.6**
- `arduino-cli`: **1.4.1** (`/home/arn/bin/arduino-cli`)
- PlatformIO: **not found** (no `platformio.ini` in the repo)
- Important Arduino libraries (installed): **Async TCP 3.4.10**, **ESP Async WebServer 3.9.6**, **ArduinoJson 6.21.5**
- Arduino tools in `~/.arduino15/packages/esp32/tools`: **esptool_py 4.5.1**, **mklittlefs 3.0.0-gnu12-dc7f933**
- Python `esptool` (in SNS venv): **esptool 5.1.0**

---

## Detailed Information 🔧

### Arduino CLI
- Version: `arduino-cli 1.4.1` (install path: `/home/arn/bin/arduino-cli`)

### ESP32 Arduino Core
- Installed: `esp32:esp32` **2.0.10** (directory: `~/.arduino15/packages/esp32/hardware/esp32/2.0.10`)
- Tools: `esptool_py 4.5.1`, `mklittlefs 3.0.0-gnu12-dc7f933` (see `~/.arduino15/packages/esp32/tools/`)

### Python Environments
- Project `ESP32-H4CK-SNS` has a `.venv` with, among others:
  - `esptool 5.1.0`
  - `pyserial 3.5`

### Installed Arduino Libraries (from `arduino-cli lib list`)
- `ArduinoJson` 6.21.5 (available: 7.4.2)
- `Async TCP` 3.4.10
- `ESP Async WebServer` 3.9.6
- (see `arduino-cli lib list` for full listing)

### Project-specific Build Info
- Sketch FQBN: `esp32:esp32:esp32`
- `build.sh` uses: `arduino-cli compile --fqbn "esp32:esp32:esp32" --build-property "build.partitions=partitions.csv" --build-property "upload.maximum_size=1966080"`
- Upload script: `upload.sh` runs `arduino-cli upload -b esp32:esp32:esp32 -p <PORT> .` and searches for `mklittlefs` in Arduino tools
- Key includes in sketches: `WiFi.h`, `ESPAsyncWebServer.h`, `AsyncTCP.h`, `ArduinoJson.h`, `LittleFS.h`, `mbedtls` headers, `Preferences.h`
- WebSocket / Server: code uses WebSockets (noticeable in WebSocket and `ESPAsyncWebServer` usage)

### PlatformIO
- No `platformio.ini` found in workspace. `platformio` CLI not detected in system PATH.

---

## Notes / Suggestions 💡
- If desired, I can append full CLI outputs (`arduino-cli core list`, `arduino-cli lib list`, `pip list` from relevant venvs) into this file.
- If you want to use PlatformIO, I can create a starter `platformio.ini`.

---

*This file was auto-generated. Tell me if you want changes (e.g., more detail or additional checks).*