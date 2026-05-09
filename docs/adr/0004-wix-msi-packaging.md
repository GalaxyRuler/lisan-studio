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

## Alternatives Considered

- Qt Installer Framework: useful later for componentized or cross-platform
  installation.
- NSIS/Inno Setup: deferred because WiX is already available locally and MSI is
  easier to validate for the first beta.

## Consequences

The package script owns staging and WiX invocation. Auto-update and public
distribution remain out of scope for v0.1.0 beta.

## Verification

The packaging script must stage `LisanStudio.exe` and produce
`artifacts\LisanStudio-0.1.0-beta.msi` from staged files. The MSI smoke script
must install the MSI, run installed smoke, confirm shortcuts and license
payloads, and uninstall cleanly.
