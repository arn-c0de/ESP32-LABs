# ESP32-H4CK - "SecureNet Solutions" Vulnerable Lab

Version: **1.0.1** (English)

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

## Intentional Vulnerabilities (Highlights) 🧪

<details>
  <summary><strong>Show vulnerability categories and example routes (click to reveal)</strong></summary>

- A01: Broken Access Control — Unprotected admin endpoints (`/api/admin/*`), IDOR (`/vuln/user`, `/vuln/user-profile`, `/api/documents`).
- A02: Cryptographic Failures — Weak/accept-all JWTs and exposed secrets (`/api/jwt-debug`, `/.env`).
- A03: Injection — SQLi (`/vuln/search`), Command Injection (`/vuln/ping`), XXE (`/api/xml-parse`).
- A04/A05: Misconfiguration & Info Disclosure — `.git`, `.env`, `/backup` holdings, overly permissive CORS and headers.
- A06/A07: Broken Authentication & Session Management — Weak default creds, brute-force endpoints, session fixation and predictable session IDs.
- A08: Insecure Deserialization — `/vuln/deserialize` accepts arbitrary JSON.
- A10: Server-Side vulnerabilities — Unrestricted File Upload (`/api/upload`), SSRF (`/api/fetch`), Race Conditions (`/api/wallet/withdraw`).

</details>

> Tip: Vulnerability lists are hidden by default to avoid spoiling learning exercises. Click the header to reveal details when you're ready.

---

## Quick Start (Hardware & Software)
- Hardware: ESP32 board (4MB+ flash recommended)
- Tools: Arduino IDE (1.8+/2.x), `arduino-cli`, `picocom` or similar for serial
- Libraries: `ESPAsyncWebServer`, `AsyncTCP`, `ArduinoJson`, `LittleFS`

1. Copy `data/` to the board filesystem (ESP32 Sketch Data Upload plugin).
2. Edit `01_Config.ino` or `.env` to set `WIFI_SSID`, `WIFI_PASSWORD`, and `STATION_MODE`.
3. Build: `./build.sh` (injects `.env` into build defines)
4. Upload: `./upload.sh` (prompts for port and handles picocom conflicts)
5. Monitor: `./monitor.sh` to open serial at 115200 baud

---

## Useful Endpoints (examples)

<details>
  <summary><strong>Show example endpoints (click to reveal)</strong></summary>

- Public pages: `/`, `/about`, `/products`, `/support`, `/privacy`, `/terms`
- Recon & sensitive files: `/.git/config`, `/.env`, `/backup/`
- Vulnerabilities: `/vuln/search`, `/vuln/comments`, `/vuln/download`, `/vuln/ping`, `/vuln/transfer`
- Admin & practice APIs: `/api/jwt-debug`, `/api/endpoints`, `/api/admin/users-export`, `/api/admin/logs`, `/api/version`, `/api/upload`, `/api/fetch`, `/api/xml-parse`, `/api/wallet/withdraw`
- Telnet: `telnet <device-ip> 23` (try `sudo`, `su`, `find -perm -4000`, `vim` escape techniques)

</details>

> Note: Endpoint details are collapsible to prevent accidental spoilers or discovery by unintended viewers. Reveal only when ready to begin testing.
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

