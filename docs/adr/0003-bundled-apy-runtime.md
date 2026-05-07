# ADR-0003: Bundled Apy Runtime

## Status

Accepted

## Context

The beta installer must run `.apy` files on a fresh machine without relying on
user PATH, global Python, or manually installed packages.

## Decision

Package a controlled Python runtime under `runtime\python` and install
`lughat-althuban` into that staged runtime during packaging. The app executes
runtime actions with explicit program/argument arrays through `QProcess`.

## Alternatives Considered

- Use system Python: rejected because PATH and package drift would create
  support failures.
- First-run download/setup: rejected for beta because network and permission
  failures would block first launch.

## Consequences

The installer is larger, but run/lint/format behavior is predictable.

## Verification

Packaging must stage `runtime\python\python.exe`; runtime tests verify explicit
argv construction and no shell-command concatenation.

