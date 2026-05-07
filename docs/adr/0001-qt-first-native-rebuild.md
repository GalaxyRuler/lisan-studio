# ADR-0001: Qt-First Native Rebuild

## Status

Accepted

## Context

The previous Arabic Code Studio prototype is treated as untrusted because it is
bug-heavy and coupled to an Emacs/WPF host strategy. The new product needs a
native installable desktop app with fewer host-integration failure modes.

## Decision

Build a clean-room C++17 / Qt 6 Windows desktop app in a new repository. Do not
copy old WPF, Emacs-host, browser-shell, or Electron code.

## Alternatives Considered

- Repair current WPF/Emacs host: rejected because existing bugs should not
  become the release foundation.
- JetBrains Platform: kept as fallback if Qt text behavior fails.
- Custom editor from zero: rejected for v0.1.0 because it would create too many
  editor-engine bugs.

## Consequences

Qt becomes the app shell and text-system foundation. The first release must
prove Arabic editing behavior before broader IDE features are trusted.

## Verification

Qt build and editor torture tests must pass before packaging or release.

