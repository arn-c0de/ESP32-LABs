# SSH Protocol NOT Supported on ESP32

## Why Real SSH Doesn't Work on ESP32

### Technical Requirements for SSH-2.0

Real SSH protocol (OpenSSH compatible) requires:

1. **Cryptographic Operations**
   - Diffie-Hellman Key Exchange (1024-4096 bit)
   - Multiple cipher support (AES-128/256, ChaCha20-Poly1305, 3DES)
   - HMAC algorithms (SHA1, SHA256, SHA512)
   - RSA/Ed25519 host keys
   - Random number generation (CSPRNG)

2. **Memory Requirements**
   - ~50-100KB RAM **per active SSH connection**
   - Host key storage (~2-4KB)
   - Session state buffers
   - Crypto context data

3. **Protocol Complexity**
   - Binary packet framing
   - Sequence number tracking
   - Multiple authentication methods
   - Channel multiplexing
   - Terminal emulation (PTY)

### ESP32 Limitations

- **Total RAM**: 320KB (shared with WiFi stack, web server, etc.)
- **Available for app**: ~80-120KB after WiFi/networking
- **Per-connection overhead**: SSH would consume 40-80% of free RAM
- **CPU**: 240MHz dual-core (slow for intensive crypto)

### Why Existing "ESP32 SSH" Libraries Don't Work Well

1. **libssh** - Requires 200KB+ RAM, designed for Linux/x86
2. **Minimal SSH implementations** - Skip critical security features
3. **Fake SSH servers** - Accept connections but can't complete handshake

**Result**: OpenSSH clients get `Bad packet length` or `MAC incorrect` errors.

---

## Alternatives for ESP32-H4CK

### ✅ Option 1: Telnet (Port 23) - **RECOMMENDED**
```bash
telnet 192.168.4.1 23
# Login: admin / admin
```

**Advantages**:
- Works perfectly on ESP32
- Full command shell with PrivEsc
- Low memory overhead (~2KB per connection)
- Same user experience as SSH

**Disadvantages**:
- No encryption (but this is a lab environment)
- Port scanners identify it as telnet

---

### ✅ Option 2: WebSocket Shell (Port 80)
```bash
# Open browser to:
http://192.168.4.1/shell.html
```

**Advantages**:
- Modern web-based terminal
- Works from any browser
- Real-time bidirectional communication
- Can add TLS if needed

---

### ✅ Option 3: REST API with JWT Auth
```bash
curl -X POST http://192.168.4.1/api/login \
  -d '{"username":"admin","password":"admin"}'
# Returns JWT token for authenticated requests
```

---

## For Learning SSH Attacks

### Port Forwarding to Real SSH Server
Forward ESP32 traffic to a real SSH server on another machine:

```bash
# On Linux machine with SSH:
iptables -t nat -A PREROUTING -p tcp --dport 22 -j DNAT --to 192.168.1.10:22

# Or use socat:
socat TCP-LISTEN:22,fork TCP:real-ssh-server:22
```

### Run SSH in Docker Container
ESP32 can trigger Docker containers with full SSH:

```bash
docker run -d -p 2222:22 \
  -e SSH_ENABLE_ROOT=true \
  -e SSH_ENABLE_PASSWORD_AUTH=true \
  panubo/sshd
  
# Then connect:
ssh root@192.168.4.1 -p 2222
```

---

## Summary

**ESP32 cannot run real SSH-2.0 protocol due to hardware limitations.**

Use **Telnet (Port 23)** for direct terminal access in this vulnerable lab environment.

The educational value (privilege escalation, command injection, etc.) is identical between SSH and Telnet for this training platform.
