#!/bin/bash
# ESP32-SCADA Build Script
# Usage: ./build.sh [port]

set -e

SKETCH_DIR="$(cd "$(dirname "$0")" && pwd)"
SKETCH_NAME="ESP32-H4CK-SCADA"
BOARD="esp32:esp32:esp32"
PARTITION="partitions.csv"
PORT="${1:-/dev/ttyUSB0}"
BAUD=921600

echo "=== ESP32-SCADA Build ==="
echo "Sketch: $SKETCH_DIR/$SKETCH_NAME.ino"
echo "Board:  $BOARD"
echo "Port:   $PORT"
echo ""

# Quick library check
echo "[0/2] Checking libraries..."
MISSING_LIBS=()
for lib in "ESP Async WebServer" "Async TCP" "ArduinoJson"; do
    if ! arduino-cli lib list | grep -q "^$lib"; then
        MISSING_LIBS+=("$lib")
    fi
done

if [ ${#MISSING_LIBS[@]} -gt 0 ]; then
    echo "⚠️  Missing libraries: ${MISSING_LIBS[*]}"
    echo "Run ./upload.sh to install libraries automatically"
    exit 1
fi

# Compile
echo "[1/2] Compiling..."
arduino-cli compile \
  --fqbn "$BOARD" \
  --build-property "build.partitions=$PARTITION" \
  --build-property "upload.maximum_size=1966080" \
  "$SKETCH_DIR"

echo ""
echo "[2/2] Build complete!"
echo ""
echo "To upload: ./upload.sh $PORT"
echo "To monitor: arduino-cli monitor -p $PORT -c baudrate=115200"
