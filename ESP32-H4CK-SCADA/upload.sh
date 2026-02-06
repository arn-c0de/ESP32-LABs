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

# Enable git URL support in arduino-cli
echo "🔧 Configuring arduino-cli..."
arduino-cli config set library_manager_only false 2>/dev/null || true

# Check and install Arduino libraries
echo "📚 Checking Arduino libraries..."

# Required libraries with git URLs as fallback
declare -A LIBS=(
    ["ESPAsyncWebServer"]="https://github.com/mathieucarbou/ESPAsyncWebServer.git"
    ["AsyncTCP"]="https://github.com/mathieucarbou/AsyncTCP.git"
)

# Special handling for ArduinoJson - require v7+ for JsonDocument support
echo "  Checking ArduinoJson..."
ARDUINOJSON_VERSION=$(arduino-cli lib list | grep "^ArduinoJson" | awk '{print $2}' || echo "")
if [ -z "$ARDUINOJSON_VERSION" ]; then
    echo "    📥 Installing ArduinoJson v7..."
    arduino-cli lib install "ArduinoJson@7.2.0" 2>/dev/null || arduino-cli lib install "ArduinoJson" 2>/dev/null
    echo "    ✅ ArduinoJson installed"
elif [[ "$ARDUINOJSON_VERSION" =~ ^6\. ]]; then
    echo "    ⚠️  ArduinoJson v$ARDUINOJSON_VERSION detected (v7+ required)"
    echo "    📥 Upgrading to v7..."
    arduino-cli lib upgrade ArduinoJson 2>/dev/null || arduino-cli lib install "ArduinoJson@7.2.0" 2>/dev/null
    echo "    ✅ ArduinoJson upgraded"
else
    echo "    ✅ ArduinoJson $ARDUINOJSON_VERSION already installed"
fi

for lib_name in "${!LIBS[@]}"; do
    echo "  Checking $lib_name..."
    
    if ! arduino-cli lib list | grep -q "^$lib_name"; then
        echo "    📥 Installing $lib_name..."
        if arduino-cli lib install "$lib_name" 2>/dev/null; then
            echo "    ✅ $lib_name installed from registry"
        else
            echo "    ⚠️  Package manager failed, installing from git..."
            if arduino-cli lib install --git-url "${LIBS[$lib_name]}" 2>/dev/null; then
                echo "    ✅ $lib_name installed from git"
            else
                echo "    ❌ Failed to install $lib_name"
                echo "    Try manually: arduino-cli lib install --git-url ${LIBS[$lib_name]}"
                exit 1
            fi
        fi
    else
        echo "    ✅ $lib_name already installed"
    fi
done

echo "✅ All Arduino libraries ready"
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
AP_SSID="${AP_SSID:-ESP32-SCADA-AP}"
AP_PASSWORD="${AP_PASSWORD:-scada2026}"
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
    read -p "AP_SSID (default: ESP32-SCADA-AP): " apssid
    apssid=${apssid:-ESP32-SCADA-AP}
    read -p "AP_PASSWORD (default: scada2026): " appass
    appass=${appass:-scada2026}
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

# Inject .env values into 01_Config.ino before building
echo "🔧 Injecting .env configuration into 01_Config.ino..."

CONFIG_FILE="01_Config.ino"
CONFIG_BACKUP="01_Config.ino.bak"

# Create backup
cp "$CONFIG_FILE" "$CONFIG_BACKUP"

# Use sed to replace the hardcoded values with .env values
sed -i "s|^const char\* WIFI_SSID     = \".*\";|const char* WIFI_SSID     = \"${WIFI_SSID}\";|" "$CONFIG_FILE"
sed -i "s|^const char\* WIFI_PASSWORD = \".*\";|const char* WIFI_PASSWORD = \"${WIFI_PASSWORD}\";|" "$CONFIG_FILE"
sed -i "s|^const char\* AP_SSID       = \".*\";|const char* AP_SSID       = \"${AP_SSID}\";|" "$CONFIG_FILE"
sed -i "s|^const char\* AP_PASSWORD   = \".*\";|const char* AP_PASSWORD   = \"${AP_PASSWORD}\";|" "$CONFIG_FILE"

# Update JWT_SECRET if defined in .env
if [ -n "$JWT_SECRET" ]; then
  sed -i "s|^const char\* JWT_SECRET              = \".*\";|const char* JWT_SECRET              = \"${JWT_SECRET}\";|" "$CONFIG_FILE"
fi

# Update WIFI_STA_MODE based on STATION_MODE from .env
if [ "$STATION_MODE" = "true" ]; then
  sed -i "s|^bool        WIFI_STA_MODE  = .*;|bool        WIFI_STA_MODE  = true;|" "$CONFIG_FILE"
else
  sed -i "s|^bool        WIFI_STA_MODE  = .*;|bool        WIFI_STA_MODE  = false;|" "$CONFIG_FILE"
fi

echo "✅ Configuration injected from .env"
echo ""

# Cleanup function to restore backup on exit
cleanup() {
  if [ -f "$CONFIG_BACKUP" ]; then
    echo "🔄 Restoring original 01_Config.ino..."
    mv "$CONFIG_BACKUP" "$CONFIG_FILE"
  fi
}
trap cleanup EXIT

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
PARTITION_OFFSET="0x1F0000"
PARTITION_SIZE_DEC=2162688

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
