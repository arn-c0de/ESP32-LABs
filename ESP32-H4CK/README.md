# ESP32-H4CK - "SecureNet Solutions" Vulnerable Lab

![ESP32-LAB - SecureNet Solutions](images/ESP32-LAB-SecureNet%20Solutions.png)

Version: **1.0.1** (English) — **Quick Start:** see [QUICKSTART.md](QUICKSTART.md) for a step-by-step guide.

## Project Overview

ESP32-H4CK is an intentionally vulnerable IoT web application built for cybersecurity training and hands-on penetration testing exercises. It runs on an ESP32 microcontroller and provides a realistic-looking company website ("SecureNet Solutions") while exposing hidden endpoints, admin panels, and a wide range of intentionally insecure services for safe lab practice.

> ⚠️ WARNING: This project intentionally contains many security vulnerabilities. Use only in isolated, controlled lab environments. Do NOT connect to production networks or the internet.

---

## What's New in 1.0.1 ✅
- Added **version endpoint**: `/api/version` returning firmware metadata.
- New **Advanced Vulnerability Module** with: unrestricted file upload (`/api/upload`), SSRF (`/api/fetch`), XXE (`/api/xml-parse`), race-condition wallet (`/api/wallet/withdraw`), session fixation (`/api/auth/session-fixation`), HTTP Parameter Pollution (`/api/user/email`), open redirect, clickjacking test, and IDOR-enhanced document access (`/api/documents`).
- **Privilege Escalation training** integrated in Telnet (SUID discovery, `sudo`/`su` bypasses, `LD_PRELOAD`/`PATH` hijacking hints, cron job injection simulation).
- Recon endpoints for learning: `/.git/config`, `/.env`, `/backup/*`, and an endpoint discovery API `/api/endpoints`.
- Improved lab UX: serial monitor helper (`monitor.sh`), `build.sh` and `upload.sh` enhancements, `STATION_MODE` support to run AP-only.

---

## Key Features & Services 🔧
- HTTP/HTTPS Web Server (Port 80/443)
  - Realistic company pages (home, about, products, support) and hidden vulnerable endpoints
  - Admin and Guest roles, session management, file serving

- RESTful API (`/api/*`)
  - System info, authentication, endpoint discovery, administrative actions

- WebSocket Shell (`/shell.html`, `ws://<device>/shell`)
  - Interactive commands, simulated command execution

- Telnet Service (Port 23)
  - Multiple concurrent clients, weak auth options, **privilege escalation lessons**

- Filesystem (LittleFS)
  - Web assets, uploads, backups and deliberate sensitive file exposure

---

## Intentional Vulnerabilities (Full list) 🧪

<details>
  <summary><strong>Show full vulnerability categories and implemented routes (click to reveal)</strong></summary>

- **A01: Broken Access Control / Privilege Escalation**
  - `/api/admin/users-export` — Exports all users (CSV) **NO AUTH** in vulnerable mode
  - `/api/admin/logs` — Exposes system logs **NO AUTH**
  - `/api/admin/sessions` — Lists active sessions (IDs, usernames, IPs)
  - `/api/admin/config-update` — POST to change WiFi/JWT config (weak access control)
  - `/api/system/reboot` — Reboot endpoint (DOS vector)
  - `/vuln/user` — IDOR: access arbitrary user by id
  - `/vuln/user-profile` — IDOR: returns SSN, API key for arbitrary user
  - `/api/documents` — IDOR: access documents without auth

- **A02: Cryptographic Failures & Secret Exposure**
  - `/api/jwt-debug` — Exposes `JWT_SECRET_STR`, accepts weak algs, exploitation hints
  - `/.env` — Direct exposure of secrets via filesystem route
  - `/api/cookies/info` — Documents insecure cookie settings

- **A03: Injection (SQL, Command, XSS, XXE)**
  - `/vuln/search?q=` — SQL Injection (boolean, UNION, stacked queries)
  - `/vuln/ping?host=` — Command injection via ping parameter
  - `/vuln/comment` (POST) & `/vuln/comments` (GET) — Stored XSS
  - `/api/xml-parse` — XXE demonstration (can return local file contents)
  - `/vuln/deserialize` — Insecure deserialization of JSON

- **A04/A05: Misconfiguration & Information Disclosure**
  - `/.git/config` — Git repo information leak
  - `/backup/*` — Backup files including DB dumps and `.env.backup`
  - `/robots.txt` — Lists sensitive/hidden paths
  - HTTP headers intentionally disclose Server/X-Powered-By/X-Framework/X-Device/X-Firmware

- **A06/A07: Broken Authentication & Session Management**
  - Default weak credentials (admin/admin, guest/guest)
  - `/api/auth/bruteforce-test` — No rate limiting for brute-force practice
  - `/api/auth/session-fixation` — Accepts attacker-provided session_id
  - `/vuln/session` — Predictable session IDs

- **A08: Insecure Deserialization & Unsafe Deserialization**
  - `/vuln/deserialize` — Processes unvalidated JSON (simulate unsafe object handling)

- **A09: Logging & Monitoring Falters**
  - `/api/admin/logs` — Logs available without auth
  - Telnet/WebSocket/HTTP requests are logged verbosely for forensic practice

- **A10: File Upload / SSRF / Race / Others**
  - `/api/upload` — Unrestricted file upload (no extension/type checks)
  - `/api/fetch` — SSRF: fetch arbitrary URLs (including file://, localhost)
  - `/api/wallet/withdraw` — Race condition withdrawal (no locking)
  - `/api/user/email` — HTTP Parameter Pollution (HPP) practice
  - `/api/redirect` — Open redirect endpoint
  - `/api/frame-test` — Clickjacking test (no X-Frame-Options)


</details>

> Tip: Details are collapsed by default to avoid spoiling exercises. Click to reveal when you're ready to begin hands-on testing.

---

## Quick Start (Hardware & Software)

> Detailed quick start: see [QUICKSTART.md](QUICKSTART.md)

- Hardware: ESP32 board (4MB+ flash recommended)
- Tools: Arduino IDE (1.8+/2.x), `arduino-cli`, `picocom` or similar for serial
- Libraries: `ESPAsyncWebServer`, `AsyncTCP`, `ArduinoJson`, `LittleFS`

1. Copy `data/` to the board filesystem (ESP32 Sketch Data Upload plugin).
2. Edit `01_Config.ino` or `.env` to set `WIFI_SSID`, `WIFI_PASSWORD`, and `STATION_MODE`.
3. Build: `./build.sh` (injects `.env` into build defines)
4. Upload: `./upload.sh` (prompts for port and handles picocom conflicts)
5. Monitor: `./monitor.sh` to open serial at 115200 baud

---

## Useful Endpoints (complete list)

<details>
  <summary><strong>Show all implemented endpoints and short descriptions (click to reveal)</strong></summary>

### Public & UI
- `/` — Home page (SecureNet Solutions front page)
- `/about`, `/products`, `/support`, `/privacy`, `/terms` — Company pages
- `/login` — Web login page
- `/admin` — Admin UI (requires auth in normal mode)
- `/shell.html` — WebSocket shell UI

### Recon & Exposed Files
- `/.git/config` — Exposed git config (repo info)
- `/.env` — Exposed environment/config secrets
- `/backup/` — Backup files (DB dumps, `.env.backup`, private keys)
- `/robots.txt` — Lists hidden paths

### Vulnerability Practice Endpoints
- `/vuln/search?q=` — SQL Injection lab (boolean/UNION examples)
- `/vuln/comment` (POST) and `/vuln/comments` (GET) — Stored XSS lab
- `/vuln/download?file=` — Path traversal (try `../../../.env` or `/etc/passwd`)
- `/vuln/ping?host=` — Command injection practice
- `/vuln/transfer` (POST) — CSRF lab (no token required)
- `/vuln/user?id=` — IDOR (insecure direct object reference)
- `/vuln/user-profile?user_id=` — Enhanced IDOR returning sensitive fields
- `/vuln/deserialize` (POST) — Insecure deserialization testing
- `/vuln/logs` — Expose application logs
- `/vuln/session` — Create predictable sessions for hijacking tests
- `/vuln/profile` (POST) — Mass-assignment vulnerability

### REST API & Admin Practice (examples)
- `/api/login` (POST) — Authenticate (weak by default)
- `/api/logout` — End session
- `/api/info` — System information
- `/api/users` — User CRUD operations (GET/POST/DELETE/PUT)
- `/api/config` — Exposes configuration (WiFi/JWT) in lab mode
- `/api/jwt-debug` — JWT weakness analysis & example token
- `/api/endpoints` — Endpoint discovery API (returns list of routes)
- `/api/version` — Firmware/version metadata

### Advanced / Admin / Exploitable APIs
- `/api/upload` (POST) — Unrestricted file upload (upload a webshell)
- `/api/fetch?url=` — SSRF: fetch internal URLs or file:// paths
- `/api/xml-parse` (POST) — XXE practice (external entity file disclosure)
- `/api/wallet/withdraw` (POST) — Race condition lab (no locking)
- `/api/auth/session-fixation` — Accept attacker-provided session_id
- `/api/user/email` — HTTP Parameter Pollution practice
- `/api/redirect?url=` — Open redirect test
- `/api/frame-test` — Clickjacking frame test
- `/api/documents?doc_id=` — IDOR: fetch arbitrary document

### Admin Endpoints (deliberately exposed for practice)
- `/api/admin/users-export` — Export users as CSV **NO AUTH** (lab)
- `/api/admin/logs` — Expose system logs **NO AUTH**
- `/api/admin/sessions` — List active sessions
- `/api/admin/config-update` (POST) — Change config values (WiFi/JWT)
- `/api/system/reboot` — Reboot the device (DoS vector)
- `/api/auth/bruteforce-test` — Brute-force testing (no rate limit)
- `/api/cookies/info` — Cookie security info & exploit examples

### Other services
- Telnet (Port 23): `telnet <device-ip> 23` — Interactive shell with privilege escalation lessons (SUDO bypass, SUID discovery, PATH/LD_PRELOAD hijack)
- WebSocket: `ws://<device-ip>/shell` — Browser shell endpoint

</details>

> Note: Endpoints are listed for lab practice. Many admin endpoints deliberately lack authentication or have weak protections — use only in isolated lab environments.
---

## Safety & Lab Deployment Guidelines 🛡️
- Use a dedicated VLAN or isolated WiFi SSID
- Firewall devices to prevent internet exposure
- Lock physical access and label devices clearly
- Obtain signed participant agreements for courses using the lab
- Keep logs and monitor participant activity — intentional vulnerabilities do not replace responsible oversight

---

## Development & Contributing
- Add a new vulnerability flag in `01_Config.ino`
- Implement endpoints in `09_Vulnerabilities.ino` or `14_AdvancedVulns.ino`
- Add UI hints in `data/*.html` and document in this README
- Open PRs for new lessons, improvements or fixes

---

## Changelog
- 1.0.1 — Added advanced vulnerability modules, `/api/version`, privilege escalation Telnet lessons, unrestricted upload, SSRF/XXE, race conditions, session fixation, HPP, open redirect, clickjacking tests, and several recon endpoints.
- 1.0.0 — Initial public release (baseline lab)

---

## License & Ethics
Educational Use Only — NOT for production. Use only for training in a controlled environment.

---

If you want, I can also:
- Add short lab exercises per endpoint (beginner → advanced)
- Export a printable quick-lab worksheet

---

**Remember**: This is an intentionally vulnerable system. Use responsibly and only in controlled lab environments.

