# ADR-0007: Cross-Platform Core Policy

## Status

Accepted

## Context

The first beta is a native Windows MSI. The product direction now targets a
professional IDE that can become cross-platform soon, while keeping Windows as
the first validated release lane.

## Decision

Core IDE services must use Qt abstractions for paths, processes, settings,
shortcuts, file watching, UI state, and text handling. Windows-specific MSI,
PowerShell, registry, shortcut, and screenshot behavior stays in packaging and
QA lanes, not in cross-platform workbench services.

## Consequences

Windows remains first-class and must not regress. New core code should avoid
hard-coded separators, shell command strings, and Windows-only assumptions
unless the file is explicitly packaging or Windows QA code.

## Verification

Core tests should use Qt path/process APIs and temporary directories. Packaging
and installed-app claims continue to require the Windows Homelab lane.
