# Agent Instructions

- Inspect the project before choosing commands or patterns.
- Prefer existing scripts and conventions.
- Verify changes with the smallest relevant command first.
- Run `.\scripts\validate.ps1` before claiming source changes are ready when
  the change can affect build or behavior.
- Do not run MSI install, uninstall, upgrade, registry-mutating, or GUI
  automation checks on an active work desktop unless the user explicitly
  approves that validation path.
- Treat `.env*`, private keys, certificates, tokens, signing material, and
  credentials as off-limits.
- Do not commit generated outputs from `build/`, `stage/`, `artifacts/`,
  `out/`, or local scratch directories.
- Keep public docs free of private machine paths, private planning notes, and
  unverified release claims.
