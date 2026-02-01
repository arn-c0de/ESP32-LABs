#!/usr/bin/env bash

# Simple serial monitor helper
# Usage: ./monitor.sh [PORT] [BAUD]
# If PORT is omitted the script lists available serial ports and asks for selection.

BAUD=${2:-115200}
PORT=${1:-}

# Gather candidate ports (deduplicated)
declare -a candidates
declare -A seen
for p in /dev/ttyUSB* /dev/ttyACM* /dev/serial/by-id/*; do
  [ -e "$p" ] || continue
  # Resolve symlink if necessary
  if [ -L "$p" ]; then
    realp=$(readlink -f "$p")
  else
    realp="$p"
  fi
  # Normalize and dedupe
  realp=$(readlink -f "$realp")
  if [ -z "${seen[$realp]}" ]; then
    seen[$realp]=1
    candidates+=("$realp")
  fi
done

if [ -z "$PORT" ]; then
  if [ ${#candidates[@]} -eq 0 ]; then
    echo "⚠️  No serial ports found. Please connect a device or provide a port as an argument."
    echo "Example: ./monitor.sh /dev/ttyUSB0 115200"
    exit 1
  fi

  echo "🔍 Available serial ports:"
  i=1
  for p in "${candidates[@]}"; do
    echo "  $i) $p"
    i=$((i+1))
  done

  read -p "Select port number (1-$((i-1))) or type a path: " sel
  if [[ "$sel" =~ ^[0-9]+$ ]]; then
    idx=$((sel-1))
    if [ $idx -ge 0 ] && [ $idx -lt ${#candidates[@]} ]; then
      PORT=${candidates[$idx]}
    else
      echo "Invalid selection"; exit 2
    fi
  else
    PORT=$sel
  fi
fi

if [ ! -e "$PORT" ]; then
  echo "⚠️  Port \"$PORT\" does not exist."; exit 3
fi

# Choose available monitor tool
if command -v picocom >/dev/null 2>&1; then
  echo "✅ Using picocom for $PORT @ $BAUD"
  exec picocom -b $BAUD "$PORT"
elif python -c "import serial" >/dev/null 2>&1; then
  echo "✅ Using python -m serial.tools.miniterm for $PORT @ $BAUD"
  exec python -m serial.tools.miniterm "$PORT" $BAUD
elif command -v screen >/dev/null 2>&1; then
  echo "✅ Using screen for $PORT @ $BAUD (Exit with Ctrl-A k)"
  exec screen "$PORT" $BAUD
else
  echo "⚠️  No supported serial monitor found (picocom, python-serial, or screen)."
  echo "Install on Debian/Ubuntu: sudo apt install picocom python3-serial screen"
  exit 4
fi