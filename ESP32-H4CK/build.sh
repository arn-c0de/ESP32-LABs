#!/bin/bash
# build.sh - Compile ESP32-H4CK with credentials from .env

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

# Check if .env exists
if [ ! -f ".env" ]; then
    echo "❌ .env file not found!"
    echo "📋 Copy .env.example to .env and fill in your credentials:"
    echo "   cp .env.example .env"
    exit 1
fi

# Load .env variables
export $(cat .env | grep -v '^#' | xargs)

# Validate required variables
if [ -z "$WIFI_SSID" ] || [ -z "$WIFI_PASSWORD" ]; then
    echo "❌ WIFI_SSID and WIFI_PASSWORD must be set in .env"
    exit 1
fi

echo "✅ Building ESP32-H4CK with credentials from .env..."
echo "📡 WiFi SSID: $WIFI_SSID"

# Build compiler flags with proper quoting for string defines
# Format: -DKEY="value" for string macros
DEFINES="-DWIFI_SSID=\"${WIFI_SSID}\" \
-DWIFI_PASSWORD=\"${WIFI_PASSWORD}\" \
-DAP_SSID=\"${AP_SSID}\" \
-DAP_PASSWORD=\"${AP_PASSWORD}\" \
-DJWT_SECRET=\"${JWT_SECRET}\""

# Compile with arduino-cli
arduino-cli compile \
  -b esp32:esp32:esp32 \
  . \
  --build-property "compiler.cpp.extra_flags=-I$HOME/Arduino/libraries/ArduinoJson/src $DEFINES"

echo "✅ Build successful!"
echo "📦 Compiled binary ready at build/"
