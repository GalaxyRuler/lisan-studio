# V1 Terminal Trust Command Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expose the first terminal command surface while preserving the workspace-trust gate.

**Architecture:** `MainWindow` uses `TerminalProfileModel` to build a PowerShell launch plan. If the workspace is not trusted, it shows the terminal panel with the trust-block reason and does not start a process. Trusted launch execution remains out of scope.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- `terminal.openPowerShell` command.
- Visible menu row and command palette coverage.
- Untrusted workspace terminal block message.

Excluded:

- Starting terminal processes.
- Terminal process lifecycle.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing MainWindow terminal trust command test.
- [x] Add command/menu wiring.
- [x] Show terminal trust block message without execution.
- [x] Run focused tests, full validation, and `git diff --check`.
