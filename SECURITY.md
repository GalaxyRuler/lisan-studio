# Security Policy

## Supported Versions

Security reports should target the current `main` branch and the latest GitHub
Release.

## Reporting A Vulnerability

Open a private GitHub security advisory if available for this repository, or
contact the maintainer through the account listed on the GitHub project.

Please include:

- affected version or commit
- operating system and installation method
- reproduction steps
- expected and actual behavior
- impact assessment
- logs or screenshots with secrets removed

Do not include private keys, tokens, certificates, production credentials, or
personal data in a public issue.

## Secret Handling

The repository must not contain:

- `.env*` files
- private keys
- certificates or signing material
- API tokens
- credential stores
- private runtime configuration

If secret material is committed, treat it as exposed. Revoke or rotate the
credential before relying on a scrubbed commit.

## Installer Trust

Lisan Studio installers are unsigned unless the release explicitly says
otherwise. Unsigned installers can trigger Windows SmartScreen warnings and may
be blocked by managed enterprise policies. See
[docs/adr/0010-msi-code-signing.md](docs/adr/0010-msi-code-signing.md).
