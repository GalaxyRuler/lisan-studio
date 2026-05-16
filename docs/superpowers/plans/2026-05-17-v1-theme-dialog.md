# V1 Theme Dialog Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the settings dialog edit and persist the user theme preference.

**Architecture:** Reuse `SettingsStore::themePreference` and keep full stylesheet switching out of scope. The dialog exposes an RTL combo box with dark/light values and saves the selected data key on accept.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Theme preference combo in editor settings.
- Persist dark/light selection on apply.
- Stable object name for UI tests.

Excluded:

- Applying a light stylesheet.
- Per-workspace theme override.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing MainWindow settings-dialog theme persistence test.
- [x] Replace read-only theme label with RTL combo box.
- [x] Save combo item data through `SettingsStore`.
- [x] Preserve existing font settings behavior.
- [x] Run focused tests, full validation, and `git diff --check`.
