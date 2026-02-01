#!/bin/bash
# upload.sh - Upload compiled sketch to ESP32 with interactive port selection

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

# Ensure .env exists - create automatically when missing
if [ ! -f ".env" ]; then
  echo "⚠️  No .env file found."

  # Option A: Use environment variables if available
  if [ -n "$WIFI_SSID" ] && [ -n "$WIFI_PASSWORD" ]; then
    read -p "Use WIFI_SSID/WIFI_PASSWORD from environment to create .env? (Y/n) " resp
    resp=${resp:-Y}
    if [[ "$resp" =~ ^[Yy] ]]; then
      cat > .env <<EOF
WIFI_SSID="$WIFI_SSID"
WIFI_PASSWORD="$WIFI_PASSWORD"
AP_SSID="${AP_SSID:-ESP32-H4CK-AP}"
AP_PASSWORD="${AP_PASSWORD:-vulnerable}"
STATION_MODE="${STATION_MODE:-false}"
JWT_SECRET="${JWT_SECRET:-weak_secret_key_123}"
EOF
      echo "✅ Created .env from environment variables"
    fi

  # Option B: Copy from .env.example if present
  elif [ -f ".env.example" ]; then
    read -p "Create .env from .env.example? (Y/n) " resp
    resp=${resp:-Y}
    if [[ "$resp" =~ ^[Yy] ]]; then
      cp .env.example .env
      echo "✅ Created .env from .env.example"
      read -p "Edit .env now? (Y/n) " editnow
      editnow=${editnow:-N}
      if [[ "$editnow" =~ ^[Yy] ]]; then
        ${EDITOR:-nano} .env
      fi
    fi

  # Option C: Interactive creation
  else
    echo "No .env.example found. Creating minimal .env interactively."
    read -p "WIFI_SSID: " ssid
    read -p "WIFI_PASSWORD: " spass
    read -p "AP_SSID (default: ESP32-H4CK-AP): " apssid
    apssid=${apssid:-ESP32-H4CK-AP}
    read -p "AP_PASSWORD (default: vulnerable): " appass
    appass=${appass:-vulnerable}
    read -p "STATION_MODE - Connect to WiFi? (y/N): " stmode
    if [[ "$stmode" =~ ^[Yy] ]]; then
      stmode="true"
    else
      stmode="false"
    fi
    cat > .env <<EOF
WIFI_SSID="$ssid"
WIFI_PASSWORD="$spass"
AP_SSID="$apssid"
AP_PASSWORD="$appass"
STATION_MODE="$stmode"
JWT_SECRET="weak_secret_key_123"
EOF
    echo "✅ Created .env interactively"
  fi
fi

# Load .env and validate
set -o allexport
if [ -f ".env" ]; then
  # shellcheck source=/dev/null
  source .env
fi
set +o allexport

if [ -z "$WIFI_SSID" ] || [ -z "$WIFI_PASSWORD" ]; then
  echo "⚠️  .env missing WIFI_SSID or WIFI_PASSWORD."
  read -p "Edit .env now? (Y/n) " editnow
  editnow=${editnow:-Y}
  if [[ "$editnow" =~ ^[Yy] ]]; then
    ${EDITOR:-nano} .env
    # reload
    set -o allexport
    # shellcheck source=/dev/null
    source .env
    set +o allexport
  else
    echo "Aborting: WIFI credentials required."; exit 1
  fi
fi

# List available ports
echo "🔍 Available serial ports:"
PORTS=$(arduino-cli board list | grep -E "tty|COM" | awk '{print $1}')

if [ -z "$PORTS" ]; then
    echo "❌ No serial ports found! Connect your ESP32 and try again."
    exit 1
fi

# Display ports with numbers
PORTS_ARRAY=($PORTS)
for i in "${!PORTS_ARRAY[@]}"; do
    echo "  $((i+1))) ${PORTS_ARRAY[$i]}"
done

# Prompt user to select
echo ""
read -p "Select port number (1-${#PORTS_ARRAY[@]}): " CHOICE

# Validate input
if ! [[ "$CHOICE" =~ ^[0-9]+$ ]] || [ "$CHOICE" -lt 1 ] || [ "$CHOICE" -gt "${#PORTS_ARRAY[@]}" ]; then
    echo "❌ Invalid selection!"
    exit 1
fi

PORT="${PORTS_ARRAY[$((CHOICE-1))]}"
echo "✅ Selected port: $PORT"
echo ""

# Build
echo "🔨 Building..."
./build.sh

echo ""
echo "🚀 Uploading to $PORT..."

# Upload
arduino-cli upload \
  -b esp32:esp32:esp32 \
  -p "$PORT" \
  .

echo ""
echo "✅ Upload successful!"
echo "🔌 Open serial monitor at 115200 baud to see logs"
