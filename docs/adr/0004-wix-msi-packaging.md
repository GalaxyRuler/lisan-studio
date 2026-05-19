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
WiX. Keep installer build metadata under the app-owned
`HKCU\Software\LisanStudio` key rather than under the Windows Apps uninstall
key.

Windows Installer strips an authored `MSIINSTALLPERUSER=1` property from this
self-contained MSI at install time, so the MSI cannot force current-user
product-code registration without a command-line property, transform, or bundle.
`Package/@Scope="perUser"` still keeps the installed payload under
`LocalAppDataFolder`, but the product-code uninstall entry is registered under
`HKLM\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{ProductCode}`.
For V1, accept that native MSI behavior and assert the install leaves exactly
one Windows Apps entry for `Lisan Studio` in that hive.

The full evidence and trade-off analysis lives in
`docs/investigations/wix-dual-uninstall-registration-2026-05-19.md`. The
decisive install-log evidence is the Homelab smoke run
`prompt14-rerun-smoke-20260519T184438` (`install.log`: `PROPERTY CHANGE:
Deleting MSIINSTALLPERUSER property`).

## Alternatives Considered

- Qt Installer Framework: useful later for componentized or cross-platform
  installation.
- NSIS/Inno Setup: deferred because WiX is already available locally and MSI is
  easier to validate for the first beta.
- Requiring HKCU product-code registration from the self-contained MSI: rejected
  because the Homelab install log showed Windows Installer deleting the authored
  `MSIINSTALLPERUSER` property before registration.
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
