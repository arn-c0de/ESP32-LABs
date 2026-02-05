#!/usr/bin/env bash
set -euo pipefail

# Serial monitor helper
# Usage: ./monitor.sh [PORT] [BAUD] [CMD | --test | -t]
# If PORT is omitted the script lists available serial ports and asks for selection.

BAUD="${2:-115200}"
PORT="${1:-}"
CMD_TO_SEND="${3:-}"

# Resolve shorthand flags
case "$CMD_TO_SEND" in
  --test|-t) CMD_TO_SEND="/test" ;;
esac

cleanup() {
  # Remove temp file if it exists
  [[ -n "${_MONITOR_TMP:-}" ]] && rm -f "$_MONITOR_TMP"
}
trap cleanup EXIT

# Kill any process holding the port (optional, only when port is locked)
release_port() {
  local p="$1"
  if ! fuser "$p" >/dev/null 2>&1; then
    return 0
  fi
  echo "Port $p is locked by another process."
  read -r -p "Kill the process holding it? [y/N]: " ans
  case "$ans" in
    [yY]*)
      fuser -k "$p" 2>/dev/null || true
      sleep 0.5
      if fuser "$p" >/dev/null 2>&1; then
        echo "Failed to release $p." >&2; exit 5
      fi
      echo "Port released."
      ;;
    *)
      echo "Aborting." >&2; exit 5
      ;;
  esac
}

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
    echo "Example: ./monitor.sh /dev/ttyUSB0 115200" >&2
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

# Check for port lock before proceeding
release_port "$PORT"

# Send an initial command to the device if specified
send_initial_command() {
  if [[ -n "$CMD_TO_SEND" ]]; then
    echo "Sending initial command to $PORT: $CMD_TO_SEND"
    sleep 0.1
    printf '%s\r\n' "$CMD_TO_SEND" > "$PORT" || echo "Failed to write command to $PORT" >&2
    sleep 0.2
  fi
}

# --- Python monitor (preferred) ---
# Write the Python script to a temp file so that stdin remains the terminal.
# This fixes the termios "Inappropriate ioctl for device" error that occurs
# when the script is fed via heredoc on stdin.
start_python_monitor() {
  _MONITOR_TMP=$(mktemp /tmp/monitor_XXXXXX.py)
  cat > "$_MONITOR_TMP" <<'PYEOF'
import sys, os, serial, select, termios, tty, threading, time, signal, json

HISTORY_FILE = os.path.expanduser("~/.serial_monitor_history")
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
hist_idx = len(history)  # points past end = "new line"
saved_line = ""  # stash current input when browsing history

# -- Line editor state --
line_buf = []   # list of chars
cursor = 0      # position within line_buf

def w(s):
    sys.stdout.write(s)
    sys.stdout.flush()

def get_term_width():
    try:
        import struct, fcntl
        res = fcntl.ioctl(tty_fd, termios.TIOCGWINSZ, b'\x00' * 8)
        return struct.unpack('HHHH', res)[1]
    except Exception:
        return 80

def redraw_line():
    """Clear current line and redraw the buffer with cursor at the right position."""
    w('\r\x1b[K' + ''.join(line_buf))
    # Move cursor back to correct position
    trail = len(line_buf) - cursor
    if trail > 0:
        w(f'\x1b[{trail}D')

def set_line(text):
    """Replace entire line buffer with text and put cursor at end."""
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

print(f"Connected to {port} @ {baud} baud", flush=True)
print("Arrow Up/Down = history, Left/Right = move cursor", flush=True)
print("", flush=True)

def read_serial():
    while running:
        try:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                sys.stdout.buffer.write(data)
                sys.stdout.flush()
        except (serial.SerialException, OSError):
            break
        time.sleep(0.01)

thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

def read_char():
    """Read one character from the terminal, with escape sequence handling."""
    ch = tty_file.read(1)
    if ch == '\x1b':
        # Possible escape sequence - peek for more
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
                    # Delete key: ESC [ 3 ~
                    if select.select([tty_file], [], [], 0.05)[0]:
                        tty_file.read(1)  # consume '~'
                    return 'DEL'
                return None  # unknown sequence, ignore
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

            elif ch == 'HOME' or ch == '\x01':  # Home or Ctrl-A
                if cursor > 0:
                    w(f'\x1b[{cursor}D')
                    cursor = 0

            elif ch == 'END' or ch == '\x05':  # End or Ctrl-E
                trail = len(line_buf) - cursor
                if trail > 0:
                    w(f'\x1b[{trail}C')
                    cursor = len(line_buf)

            elif ch == 'DEL':
                if cursor < len(line_buf):
                    line_buf.pop(cursor)
                    redraw_line()

            elif ch == '\x7f' or ch == '\x08':  # Backspace
                if cursor > 0:
                    cursor -= 1
                    line_buf.pop(cursor)
                    redraw_line()

            elif ch == '\x15':  # Ctrl-U: clear line
                line_buf.clear()
                cursor = 0
                redraw_line()

            elif ch == '\x0b':  # Ctrl-K: kill to end of line
                line_buf = line_buf[:cursor]
                redraw_line()

            elif ch == '\n':
                line = ''.join(line_buf)
                w('\r\n')

                # Check for local exit commands
                if line.strip() in ('/exit', '/bye'):
                    w('Exiting...\r\n')
                    break

                # Add to history (skip duplicates of last entry and empty lines)
                if line.strip() and (not history or history[-1] != line):
                    history.append(line)
                    save_history(history)
                hist_idx = len(history)
                saved_line = ""

                # Send to serial
                ser.write((line + '\r\n').encode())
                line_buf.clear()
                cursor = 0

            elif len(ch) == 1 and ch >= ' ':
                # Printable character - insert at cursor position
                line_buf.insert(cursor, ch)
                cursor += 1
                redraw_line()

except KeyboardInterrupt:
    w('\r\nExiting...\r\n')
finally:
    restore_and_exit()
PYEOF
  exec python3 "$_MONITOR_TMP" "$PORT" "$BAUD"
}

# Choose available monitor tool
if python3 -c "import serial" >/dev/null 2>&1; then
  echo "Using Python monitor for $PORT @ $BAUD"
  echo "Type /exit or /bye to quit. Ctrl-C also works."
  send_initial_command
  start_python_monitor

elif command -v socat >/dev/null 2>&1; then
  echo "Using socat for $PORT @ $BAUD (exit with Ctrl-C)"
  send_initial_command
  exec socat -d -d FILE:"$PORT",raw,b"$BAUD",echo=0 STDIO,raw,echo=1

elif command -v picocom >/dev/null 2>&1; then
  echo "Using picocom for $PORT @ $BAUD (exit with Ctrl-A Ctrl-Q)"
  send_initial_command
  exec picocom -b "$BAUD" "$PORT"

elif command -v screen >/dev/null 2>&1; then
  echo "Using screen for $PORT @ $BAUD (exit with Ctrl-A k)"
  send_initial_command
  exec screen "$PORT" "$BAUD"

else
  echo "No supported serial monitor found (python3-serial, socat, picocom, or screen)." >&2
  echo "Install: sudo apt install python3-serial socat picocom screen" >&2
  exit 4
fi