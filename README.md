# ESP32-LABs


**ESP32-LABs** is a collection of ESP32-based educational and experimental labs for security research, training, and prototyping. The repository hosts current projects and will continue to host future labs covering offensive and defensive techniques, safe experiments, and red-team/blue-team exercises.

Repository governance: see [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md), and `LICENSE` for contribution, disclosure, and licensing information. For security reports or questions, contact: arn-c0de@protonmail.com

---

## Purpose & Scope
- Provide short, focused lab projects and examples that illustrate common networking, system, and application-level security concepts on ESP32 devices.
- Serve as a catalogue of reusable labs that instructors, students, researchers, and practitioners can adapt for teaching or testing in controlled environments.
- Encourage experimentation and responsible disclosure of issues found while using or extending the labs.

## Safety & Ethics 
These labs may intentionally include insecure code or configurations for educational purposes. Use them only in isolated, controlled environments (e.g., dedicated VLANs, lab Wi‑Fi, or air‑gapped networks). Do not use real credentials or production resources when testing.

Always obtain permission before performing security testing on systems you do not own.

## Getting Started (high level)
- Each lab has its own README or QUICKSTART with specific build and deployment instructions — start there for project-specific setup.
- Common utilities and helper scripts may be available at the repository root for convenience (e.g., build/upload helpers, environment templates).

## LABs
<img src="ESP32-H4CK-SNS/images/sns-productshop.png" alt="ESP32-LAB - SecureNet Solutions" style="float:left; margin:0 1rem 1rem 0; width:300px;">

- **ESP32-H4CK — SecureNet Lab**: A vulnerable "SecureNet Solutions" lab designed for hands-on penetration testing exercises and learning. See the lab documentation and exercises in [ESP32-H4CK-SNS/README.md](ESP32-H4CK/README.md) and the quick start guide: [ESP32-H4CK-SNS/QUICKSTART.md](ESP32-H4CK/QUICKSTART.md).

<img src="ESP32-H4CK-SCADA/images/h4ck-SCADA-dashboard.png" alt="ESP32-LAB - SCADA Industrial" style="float:left; margin:0 1rem 1rem 0; width:300px;">

- **ESP32-H4CK-SCADA — Industrial SCADA Lab**: A red team-oriented SCADA training environment simulating multi-line production with realistic sensor physics and six independent exploit paths. See the lab documentation and exercises in [ESP32-H4CK-SCADA/README.md](ESP32-H4CK-SCADA/README.md).

- More labs will be added over time — check each subfolder for its README or QUICKSTART.

## Contributing 
Contributions are encouraged:
- Open an **issue** for questions, bug reports, or feature ideas.
- Send a **pull request** with clear descriptions and, where applicable, deployment notes and safety guidance.
- Label any submissions that include potentially dangerous actions and include mitigation guidance for safe lab use.

## License & Contact
This repository is distributed under the "Educational Use License (EUL) 2026" — see the project `LICENSE` for the full terms.

Short summary (English):
- Permitted: use and hosting for **non-commercial educational purposes** (e.g., making the software available to students and pupils).
- Prohibited without permission: **redistribution, sale, sublicensing, or public distribution** of the code (source or binary).
- Modifications are allowed for internal or educational use only; **publishing modified versions requires prior written permission**.
- **Attribution required**: retain copyright and license notices and include visible credit when hosting.

For permission requests (redistribution, commercial use, or publishing modified versions), contact the copyright holder: arn-c0de@protonmail.com.

