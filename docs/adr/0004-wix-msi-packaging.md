# ADR-0004: WiX MSI Packaging

## Status

Accepted

## Context

The first release is a private Windows beta. The installer should be boring,
auditable, and locally reproducible.

## Decision

Use WiX v7 to build a per-user MSI from a staged directory. Use `windeployqt6`
to stage Qt runtime files before WiX runs.

## Alternatives Considered

- Qt Installer Framework: useful later for componentized or cross-platform
  installation.
- NSIS/Inno Setup: deferred because WiX is already available locally and MSI is
  easier to validate for the first beta.

## Consequences

The package script owns staging and WiX invocation. Auto-update and public
distribution remain out of scope for v0.1.0 beta.

## Verification

The packaging script must produce `artifacts\ArabicCodeStudioQt-0.1.0-beta.msi`
from staged files.

