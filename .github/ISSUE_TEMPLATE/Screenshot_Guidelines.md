Screenshot Guidelines

Use these guidelines when adding screenshots to an issue report or pull request.

- Preferred formats: PNG or JPG.
- Keep screenshots focused: crop to the relevant area (console output, error dialog, request/response body).
- Include a short caption explaining what the screenshot shows and at which step it was taken.
- If including logs, prefer attaching a small text file or pasting a truncated, redacted snippet in a code block instead of large screenshots.
- Avoid including sensitive data such as passwords, JWT tokens, or private keys. If such data is required for reproduction, redact it before sharing.

Example:

![Serial boot output](path/to/screenshot.png)
*Caption: Serial output showing boot messages and the error at 00:02:15.*

If you need help capturing a screenshot on your platform, please include the operating system and tool you used to take the screenshot so maintainers can provide specific guidance.