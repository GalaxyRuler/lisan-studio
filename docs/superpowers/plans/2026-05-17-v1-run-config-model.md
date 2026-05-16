# V1 Run Configuration Model Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a pure named run configuration model before adding run configuration UI.

**Architecture:** Introduce `RuntimeRunConfigurationModel` in `acs_core`. It stores validated named configurations with stable IDs, action, file path, working directory, and reload behavior. This slice is data-only and does not execute workspace-provided commands.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- `RuntimeRunConfiguration` data shape.
- Current-file default run configuration helper.
- Add/find/remove behavior.
- Validation for blank IDs/names/files and duplicate IDs/names.

Excluded:

- Run configuration UI.
- Persistence.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing pure run configuration tests.
- [x] Add `RuntimeRunConfigurationModel` to `acs_core`.
- [x] Implement validation and add/find/remove behavior.
- [x] Run focused tests, full validation, and `git diff --check`.
