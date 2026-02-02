#!/usr/bin/env bash
set -euo pipefail

# Serial console: send commands WITHOUT showing device output.
# This allows you to run monitor.sh in one terminal and console.sh in another.
# Usage: ./console.sh [PORT] [BAUD] [CMD | --test | -t]

BAUD="${2:-115200}"
PORT="${1:-}"
CMD_TO_SEND="${3:-}"

case "$CMD_TO_SEND" in
  --test|-t) CMD_TO_SEND="/test" ;;
esac

cleanup() {
  [[ -n "${_CONSOLE_TMP:-}" ]] && rm -f "$_CONSOLE_TMP"
}
trap cleanup EXIT

# Gather candidate ports (deduplicated)
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
    echo "Example: ./console.sh /dev/ttyUSB0 115200" >&2
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

# Send initial command if provided
send_initial_command() {
  if [[ -n "$CMD_TO_SEND" ]]; then
    echo "[console] Sending: $CMD_TO_SEND"
    sleep 0.1
    printf '%s\r\n' "$CMD_TO_SEND" > "$PORT" || echo "Failed to write command to $PORT" >&2
    sleep 0.2
  fi
}

# --- Python console (send-only) ---
start_python_console() {
  _CONSOLE_TMP=$(mktemp /tmp/console_XXXXXX.py)
  cat > "$_CONSOLE_TMP" <<'PYEOF'
import sys, os, serial, select, termios, tty, signal, json

HISTORY_FILE = os.path.expanduser("~/.serial_console_history")
MAX_HISTORY = 500

port, baud = sys.argv[1], int(sys.argv[2])

try:
    ser = serial.Serial(port, baud, timeout=0.1)
except serial.SerialException as e:
    print(f"Cannot open {port}: {e}", file=sys.stderr)
    sys.exit(1)

try:
    tty_fd = os.open("/dev/tty", os.O_RDWR)
    tty_file = os.fdopen(tty_fd, "r", buffering=1)
except OSError:
    print("No controlling terminal available.", file=sys.stderr)
    ser.close()
    sys.exit(1)

old_settings = termios.tcgetattr(tty_fd)
tty.setraw(tty_fd)

running = True

# -- Command history --
def load_history():
    try:
        with open(HISTORY_FILE, "r") as f:
            h = json.load(f)
            if isinstance(h, list):
                return h[-MAX_HISTORY:]
    except (FileNotFoundError, json.JSONDecodeError, OSError):
        pass
    return []

def save_history(hist):
    try:
        with open(HISTORY_FILE, "w") as f:
            json.dump(hist[-MAX_HISTORY:], f)
    except OSError:
        pass

history = load_history()
hist_idx = len(history)
saved_line = ""

# -- Line editor state --
line_buf = []
cursor = 0

def w(s):
    sys.stdout.write(s)
    sys.stdout.flush()

def redraw_line():
    w('\r[console]> ' + ''.join(line_buf))
    trail = len(line_buf) - cursor
    if trail > 0:
        w(f'\x1b[{trail}D')

def set_line(text):
    global line_buf, cursor
    line_buf = list(text)
    cursor = len(line_buf)
    redraw_line()

def restore_and_exit():
    global running
    running = False
    termios.tcsetattr(tty_fd, termios.TCSADRAIN, old_settings)
    ser.close()

def sigint_handler(_sig, _frame):
    w('\r\nExiting...\r\n')
    restore_and_exit()
    sys.exit(0)

signal.signal(signal.SIGINT, sigint_handler)

print(f"Console for {port} @ {baud} baud (send-only, no device output shown)", flush=True)
print("Arrow Up/Down = history, Left/Right = move cursor", flush=True)
print("Type commands and press Enter. Use /exit or /bye to quit.", flush=True)
print("", flush=True)

w("[console]> ")

def read_char():
    """Read one character from the terminal, with escape sequence handling."""
    ch = tty_file.read(1)
    if ch == '\x1b':
        if select.select([tty_file], [], [], 0.05)[0]:
            ch2 = tty_file.read(1)
            if ch2 == '[':
                ch3 = tty_file.read(1)
                if ch3 == 'A': return 'UP'
                if ch3 == 'B': return 'DOWN'
                if ch3 == 'C': return 'RIGHT'
                if ch3 == 'D': return 'LEFT'
                if ch3 == 'H': return 'HOME'
                if ch3 == 'F': return 'END'
                if ch3 == '3':
                    if select.select([tty_file], [], [], 0.05)[0]:
                        tty_file.read(1)
                    return 'DEL'
                return None
            return None
    return ch

try:
    while running:
        if select.select([tty_file], [], [], 0.1)[0]:
            ch = read_char()
            if ch is None:
                continue
            if not ch:
                break

            if ch == '\x03':  # Ctrl-C
                w('\r\nExiting...\r\n')
                break

            elif ch == 'UP':
                if hist_idx > 0:
                    if hist_idx == len(history):
                        saved_line = ''.join(line_buf)
                    hist_idx -= 1
                    set_line(history[hist_idx])

            elif ch == 'DOWN':
                if hist_idx < len(history):
                    hist_idx += 1
                    if hist_idx == len(history):
                        set_line(saved_line)
                    else:
                        set_line(history[hist_idx])

            elif ch == 'LEFT':
                if cursor > 0:
                    cursor -= 1
                    w('\x1b[D')

            elif ch == 'RIGHT':
                if cursor < len(line_buf):
                    cursor += 1
                    w('\x1b[C')

            elif ch == 'HOME' or ch == '\x01':
                if cursor > 0:
                    w(f'\x1b[{cursor}D')
                    cursor = 0

            elif ch == 'END' or ch == '\x05':
                trail = len(line_buf) - cursor
                if trail > 0:
                    w(f'\x1b[{trail}C')
                    cursor = len(line_buf)

            elif ch == 'DEL':
                if cursor < len(line_buf):
                    line_buf.pop(cursor)
                    redraw_line()

            elif ch == '\x7f' or ch == '\x08':
                if cursor > 0:
                    cursor -= 1
                    line_buf.pop(cursor)
                    redraw_line()

            elif ch == '\x15':  # Ctrl-U
                line_buf.clear()
                cursor = 0
                redraw_line()

            elif ch == '\x0b':  # Ctrl-K
                line_buf = line_buf[:cursor]
                redraw_line()

            elif ch == '\n':
                line = ''.join(line_buf)
                w('\r\n')

                # Check for local exit commands
                if line.strip() in ('/exit', '/bye'):
                    w('Exiting...\r\n')
                    break

                # Add to history
                if line.strip() and (not history or history[-1] != line):
                    history.append(line)
                    save_history(history)
                hist_idx = len(history)
                saved_line = ""

                # Send to serial
                if line.strip():
                    ser.write((line + '\r\n').encode())
                    w(f"[sent: {line}]\r\n")

                line_buf.clear()
                cursor = 0
                w("[console]> ")

            elif len(ch) == 1 and ch >= ' ':
                line_buf.insert(cursor, ch)
                cursor += 1
                redraw_line()

except KeyboardInterrupt:
    w('\r\nExiting...\r\n')
finally:
    restore_and_exit()
PYEOF
  exec python3 "$_CONSOLE_TMP" "$PORT" "$BAUD"
}

# Start console
if python3 -c "import serial" >/dev/null 2>&1; then
  echo "Using Python console for $PORT @ $BAUD (send-only)"
  send_initial_command
  start_python_console

elif command -v socat >/dev/null 2>&1; then
  echo "Using socat for $PORT @ $BAUD (send-only)"
  send_initial_command
  # socat: FILE for writing, /dev/null for discarding device input
  exec socat STDIO FILE:"$PORT",raw,b"$BAUD",echo=0

else
  echo "No supported tool found. Install python3-serial or socat." >&2
  exit 4
fi
