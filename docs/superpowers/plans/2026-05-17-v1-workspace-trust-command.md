# V1 Workspace Trust Command Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an explicit workspace trust command that persists the trust flag and lets terminal readiness use the existing trust gate.

**Architecture:** `MainWindow` updates `WorkspaceSettings.trusted`, persists through `WorkspaceSettingsStore`, and reuses the existing `TerminalProfileModel` gate. This slice still does not start terminal processes.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- `workspace.trust` command and visible menu row.
- Persisted `.lisan-workspace/settings.json` trusted flag.
- MainWindow test proving terminal moves from blocked to ready after trust.

Excluded:

- Trust revocation UI.
- Terminal process execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing workspace trust command tests.
- [x] Add command/menu wiring.
- [x] Persist trust through `WorkspaceSettingsStore`.
- [x] Reuse terminal gate to show ready state after trust.
- [x] Run focused tests, full validation, and `git diff --check`.
