#!/bin/bash
# upload.sh - Upload compiled sketch to ESP32 with interactive port selection

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

# Create virtual environment if it doesn't exist
if [ ! -d ".venv" ]; then
    echo "🔧 Creating Python virtual environment..."
    # Prefer creating venv with --upgrade-deps when available so pip/setuptools/wheel are bootstrapped
    if python3 -m venv --help 2>/dev/null | grep -q -- '--upgrade-deps'; then
        python3 -m venv --upgrade-deps .venv
    else
        python3 -m venv .venv
    fi
    echo "✅ Virtual environment created"
fi

# Activate virtual environment
# shellcheck source=/dev/null
source .venv/bin/activate

# Use explicit venv python/pip to avoid OS-managed pip (PEP 668) errors
VENV_PY="$PROJECT_DIR/.venv/bin/python"
VENV_PIP="$PROJECT_DIR/.venv/bin/pip"

# Ensure pip exists and is up-to-date
export PIP_DISABLE_PIP_VERSION_CHECK=1
echo "🔄 Upgrading pip in virtualenv..."
$VENV_PY -m pip install --upgrade pip setuptools wheel 2>/dev/null || true

# Install requirements
echo "📦 Installing Python requirements into virtualenv..."
$VENV_PY -m pip install -q -r requirements.txt || true

# Check dependencies first
echo "🔍 Checking dependencies..."

MKLITTLEFS=""
ESPTOOL="esptool.py"

# Find mklittlefs in Arduino tools (comes with esp32 core)
MKLITTLEFS=$(find "$HOME/.arduino15/packages/esp32/tools/mklittlefs" -name "mklittlefs" -type f -executable 2>/dev/null | head -n 1)
if [ -z "$MKLITTLEFS" ]; then
    MKLITTLEFS=$(which mklittlefs 2>/dev/null || true)
fi

# Check if mklittlefs exists
if [ -z "$MKLITTLEFS" ]; then
    echo "❌ mklittlefs not found in Arduino tools!"
    echo ""
    echo "Please install Arduino ESP32 core:"
    echo "  arduino-cli core install esp32:esp32"
    exit 1
fi

echo "✅ All dependencies available"
echo ""

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
echo "🚀 Uploading firmware to $PORT..."

# Upload firmware
arduino-cli upload \
  -b esp32:esp32:esp32 \
  -p "$PORT" \
  .

echo ""
echo "✅ Firmware upload successful!"
echo ""

# Upload filesystem
echo "🚀 Uploading filesystem (LittleFS)..."

LITTLEFS_IMAGE="littlefs.bin"
PARTITION_OFFSET="0x2D0000"
PARTITION_SIZE_DEC=1245184

echo "  Creating LittleFS image..."
"$MKLITTLEFS" -c data -b 4096 -p 256 -s $PARTITION_SIZE_DEC "$LITTLEFS_IMAGE" 2>/dev/null

echo "  Flashing filesystem..."
"$ESPTOOL" --chip esp32 --port "$PORT" --baud 921600 \
    write_flash -z $PARTITION_OFFSET "$LITTLEFS_IMAGE" 2>/dev/null

rm -f "$LITTLEFS_IMAGE"
echo "✅ Filesystem upload successful!"

echo ""
echo "✅ All uploads complete!"
echo "🔌 Open serial monitor at 115200 baud to see logs"
