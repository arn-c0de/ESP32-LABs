#!/usr/bin/env bash
set -euo pipefail

# Simple & Reliable Serial Monitor with clean input line
# Usage: ./monitorv2.sh [PORT] [BAUD]

BAUD="${2:-115200}"
PORT="${1:-}"

cleanup() {
  [[ -n "${_MONITOR_TMP:-}" ]] && rm -f "$_MONITOR_TMP"
}
trap cleanup EXIT

# Gather candidate ports
declare -a candidates=()
declare -A seen=()
for p in /dev/ttyUSB* /dev/ttyACM* /dev/serial/by-id/*; do
  [[ -e "$p" ]] || continue
  realp=$(readlink -f "$p")
  if [[ -z "${seen[$realp]:-}" ]]; then
    seen[$realp]=1
    candidates+=("$realp")
  fi
done

if [[ -z "$PORT" ]]; then
  if [[ ${#candidates[@]} -eq 0 ]]; then
    echo "No serial ports found. Connect a device or provide a port as argument." >&2
    echo "Example: ./monitorv2.sh /dev/ttyUSB0 115200" >&2
    exit 1
  fi

  echo "Available serial ports:"
  for i in "${!candidates[@]}"; do
    echo "  $((i+1))) ${candidates[$i]}"
  done

  read -r -p "Select port number (1-${#candidates[@]}) or type a path: " sel
  if [[ "$sel" =~ ^[0-9]+$ ]]; then
    idx=$((sel - 1))
    if (( idx >= 0 && idx < ${#candidates[@]} )); then
      PORT="${candidates[$idx]}"
    else
      echo "Invalid selection." >&2; exit 2
    fi
  else
    PORT="$sel"
  fi
fi

if [[ ! -e "$PORT" ]]; then
  echo "Port \"$PORT\" does not exist." >&2; exit 3
fi

# --- Ultra-Simple Python monitor (minicom-style) ---
_MONITOR_TMP=$(mktemp /tmp/monitor_v2_XXXXXX.py)
cat > "$_MONITOR_TMP" <<'PYEOF'
#!/usr/bin/env python3
import sys, serial, select, termios, tty, signal

port, baud = sys.argv[1], int(sys.argv[2])

# Open serial port
try:
    ser = serial.Serial(port, baud, timeout=0)
except serial.SerialException as e:
    print(f"Error: Cannot open {port}: {e}", file=sys.stderr)
    sys.exit(1)

# Setup terminal
stdin_fd = sys.stdin.fileno()
old_settings = termios.tcgetattr(stdin_fd)

def restore_terminal():
    termios.tcsetattr(stdin_fd, termios.TCSADRAIN, old_settings)
    ser.close()

def signal_handler(_sig, _frame):
    print("\n\nExiting...")
    restore_terminal()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

# Set terminal to raw mode for character-by-character input
tty.setraw(stdin_fd)

print(f"\r\n╔══════════════════════════════════════════════════╗\r")
print(f"║  ESP32 Serial Monitor v2                        ║\r")
print(f"║  {port:20s} @ {baud} baud          ║\r")
print(f"╚══════════════════════════════════════════════════╝\r")
print(f"\r\nCtrl-C to exit | Type and press Enter to send\r\n\r")

try:
    while True:
        # Check if serial data is available
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            # Write directly to stdout
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()
        
        # Check if keyboard input is available
        if select.select([sys.stdin], [], [], 0.01)[0]:
            char = sys.stdin.buffer.read(1)
            
            if char == b'\x03':  # Ctrl-C
                print("\r\n\nExiting...\r")
                break
            
            # Echo to screen
            sys.stdout.buffer.write(char)
            sys.stdout.buffer.flush()
            
            # Send to serial
            ser.write(char)

except KeyboardInterrupt:
    print("\r\n\nExiting...\r")
finally:
    restore_terminal()
PYEOF

# Check for python3-serial
if ! python3 -c "import serial" >/dev/null 2>&1; then
  echo "Error: python3-serial not installed" >&2
  echo "Install: sudo apt install python3-serial" >&2
  exit 4
fi

exec python3 "$_MONITOR_TMP" "$PORT" "$BAUD"
