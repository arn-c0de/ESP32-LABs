---
name: "Bug report"
about: "Report a reproducible bug in a lab. Please include the lab name, firmware/build details, steps to reproduce, and any logs or screenshots."
title: "[BUG] "
labels: ["bug", "needs-triage"]
assignees: []
---

**Which lab does this affect?**
- [ ] ESP32-H4CK
- [ ] (other) - specify: __________

**Short summary (one line):**

**Describe the bug**
A clear and concise description of what the bug is.

**Where/when does it occur?**
- Firmware version (if known):
- Build date / commit SHA:
- Board / hardware model:
- Running mode: Station / Access Point / Both
- Frequency: Always / Sometimes / Intermittent
- Steps performed immediately before the issue appears:

**Steps to reproduce**
Provide a numbered list of steps that reproduce the issue. Include exact requests, payloads, or commands when relevant.
1. 
2. 
3. 

**Expected behavior**
Explain what you expected to happen.

**Actual behavior**
Explain what actually happened, including error messages and observed side effects.

**Logs and console output**
Paste relevant serial logs, web server responses, or telnet output inside fenced code blocks. Truncate or redact any sensitive data.

```
<insert logs here>
```

**Screenshots**
Attach screenshots if they help demonstrate the problem. Recommended formats: PNG, JPG. If you attach images, also include a short caption and indicate where in the reproduction steps the screenshot was taken.

Example markdown to include an image in the issue body:

```
![Screenshot description](path/to/screenshot.png)
*Caption: brief context (e.g., "Serial output at boot")*
```

**Workaround (if any)**
Describe any known workaround or temporary mitigation.

**Additional context**
Add any other context about the problem here (related issues, suspected root cause, environment notes).

---

**Checklist for reporters**
- [ ] I have included the lab name above
- [ ] I have provided steps to reproduce
- [ ] I have attached logs or a screenshot where possible
- [ ] I confirm the report does not include secrets or private credentials
