# Security Policy

## Supported Versions
I actively maintain the code in this repository. When reporting a vulnerability, please specify the affected version (tag or commit hash).

## Reporting a Vulnerability (public issues)
I do not maintain an email contact or a private Security Advisory channel. If you discover a potential security issue, please open a new GitHub Issue in this repository with the title:

`SECURITY: <short description>`

Do NOT include sensitive data (encryption keys, unredacted drive images, private personal data) in the public issue.

When opening the issue, include as much of the following as you can without sharing secrets:

- Affected version(s) — tag or commit hash
- A concise description of the issue and its potential impact (e.g., data loss, privilege escalation)
- Minimal reproduction steps or sample commands (avoid real keys, real device images)
- Relevant logs or error messages (redact sensitive fields)
- Any suggestions for a fix or mitigation
- Optional: a contact handle (GitHub username) if you want direct follow-up

If you need to share sensitive proof-of-concept data privately, open the public issue first and request a secure channel in a follow-up comment. The maintainers will respond and provide guidance (for example, a method to submit encrypted data). Do NOT post sensitive attachments directly in the issue.

## Response Process
- Acknowledgement: We aim to acknowledge valid reports within 7 calendar days.
- Triage: The maintainers will triage, classify severity, and provide an estimated timeline.
- Fix: We aim to provide a fix or mitigation within 30 days for high-severity issues when feasible. Complex or critical issues may take longer; we will communicate progress.

## Disclosure Policy
- Coordinated disclosure is preferred. We will work with reporters to avoid public disclosure of exploit details until fixes are available.
- If a reporter publicly discloses a vulnerability without coordination, we will still work to resolve it and publish the fix and affected versions.

## What to include and what not to include
Include:
- Clear steps to reproduce (with redacted secrets)
- Version/commit information
- Expected vs actual behavior
- Logs and error messages (redacted)

Do NOT include:
- Live encryption keys
- Full drive images or unredacted personal data
- Any other secret material

## Acknowledgements
If you would like credit for responsibly disclosing a security issue, we will credit you in the release notes unless you request anonymity.

## Project-specific notes
This tool performs operations that can irreversibly modify or destroy data (formatting, wiping, encryption). Issues affecting device selection validation, key handling, command injection, or permission checks are high priority.

## Contact / Reporting
Preferred reporting method: Open a GitHub Issue titled `SECURITY: ...` in this repository:
https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux/.github/ISSUE_TEMPLATE

If you open an issue and need to share sensitive materials, request a secure channel in the issue and the maintainers will respond with instructions.
