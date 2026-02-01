# Contributing to ESP32-LABs

Thank you for your interest in contributing to ESP32-LABs. This project is intended for educational and lab use; contributions that improve documentation, add new labs, fix bugs, or improve maintainability are welcome.

## How to contribute

1. Fork the repository and create a feature branch (e.g., `feature/add-new-lab`).
2. Commit changes in logical, well-described increments. Use clear commit messages with a short subject and optional body.
3. Ensure your changes build and any new lab verifies using the steps in `QUICKSTART.md`.
4. Open a pull request (PR) describing the change, motivation, and testing performed. Reference related issues when applicable.

## Development workflow

- Branch naming: `feature/` or `fix/` prefixes are preferred.
- Build and test: Run `./build.sh` to confirm compilation. Use `./upload.sh` and `./monitor.sh` to validate runtime behavior on a test device.
- Documentation: Every lab must include a `README.md` and `QUICKSTART.md` with clear setup and safety instructions.
- Code style: Follow common C/C++/Arduino conventions (clear naming, comments for non-obvious logic, minimal global state where possible).

## Adding a new lab

When adding a new lab directory, include the following:
- `README.md` — project overview, safety warnings, list of endpoints or exercises.
- `QUICKSTART.md` — concise setup steps, configuration parameters, and verification checklist.
- Source files (.ino, .cpp/.h), `data/` web assets, and any required build scripts.
- Tests or verification steps that a maintainer can follow.

## Pull request checklist

- [ ] Branch builds cleanly (`./build.sh`).
- [ ] Documentation added/updated (`README.md`, `QUICKSTART.md`).
- [ ] New features accompanied by usage or safety notes.
- [ ] No sensitive credentials committed (.env is ignored).
- [ ] Tests or manual verification steps provided.

## Communication

- Use the issue tracker to discuss large or breaking changes before implementing them.
- For questions or to report contribution problems, contact the project maintainer: arn-c0de@protonmail.com

## Code of conduct

Please follow the project's [Code of Conduct](CODE_OF_CONDUCT.md). By contributing you agree to follow inclusive, civil behavior when interacting in issues and PRs.