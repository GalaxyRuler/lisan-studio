# V1 Terminal Profile Model Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a pure terminal profile and launch-plan boundary before any integrated terminal process execution.

**Architecture:** Introduce `TerminalProfileModel` in `acs_core`. It describes a default PowerShell profile with an explicit program and argument list, and builds a launch plan that is blocked until workspace trust is true. This slice does not start shells.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Terminal profile data shape.
- Default PowerShell profile with explicit arguments.
- Trust-gated terminal launch plan.
- Pure service tests.

Excluded:

- Starting terminal processes.
- Terminal widget UI.
- Shell-string command construction.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing pure terminal profile tests.
- [x] Add `TerminalProfileModel` to `acs_core`.
- [x] Implement explicit profile and trust-gated launch plan.
- [x] Run focused tests, full validation, and `git diff --check`.
