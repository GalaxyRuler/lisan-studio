# ADR-0004: WiX MSI Packaging

## Status

Accepted

## Context

The first release is a private Windows beta. The installer should be boring,
auditable, and locally reproducible.

## Decision

Use WiX v7 to build a per-user MSI from a staged directory. Use `windeployqt6`
to stage Qt runtime files before WiX runs. Package third-party license payloads
with the staged files. During private beta, allow same-version upgrades so
repeated `0.1.0` test builds replace earlier beta packages instead of leaving
multiple Windows Installer clients for shortcut components.

### Per-user uninstall registration

Rely on Windows Installer's product-code uninstall registration as the canonical
Windows Apps entry. Do not author a parallel stable
`HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\LisanStudio` key in
WiX. Set `MSIINSTALLPERUSER=1` explicitly so the per-user package is registered
under the current user's uninstall hive instead of the machine uninstall hive.
Keep installer build metadata under the app-owned `HKCU\Software\LisanStudio`
key rather than under the Windows Apps uninstall key.

The full evidence and trade-off analysis lives in
`docs/investigations/wix-dual-uninstall-registration-2026-05-19.md`.

## Alternatives Considered

- Qt Installer Framework: useful later for componentized or cross-platform
  installation.
- NSIS/Inno Setup: deferred because WiX is already available locally and MSI is
  easier to validate for the first beta.
- Removing only the stable HKCU uninstall component without
  `MSIINSTALLPERUSER=1`: rejected because the investigation showed Windows
  Installer still registered the product-code entry under HKLM WOW6432Node.
- Keeping the stable HKCU component and setting `ARPSYSTEMCOMPONENT`: rejected
  for now because it hides Add/Remove Programs metadata but does not prove the
  raw duplicate registry entry disappears.
- Switching to `perMachine`: rejected because Lisan Studio remains a per-user
  beta install under `LocalAppDataFolder`.
- Relaxing the upgrade test to allow dual registration: rejected because the
  duplicate entries are the defect the test exposed.

## Consequences

The package script owns staging and WiX invocation. Auto-update and public
distribution remain out of scope for v0.1.0 beta.

## Verification

The packaging script must stage `LisanStudio.exe` and produce
`artifacts\LisanStudio-0.1.0-beta.msi` from staged files. The MSI smoke script
must install the MSI, run installed smoke, confirm shortcuts and license
payloads, and uninstall cleanly.
