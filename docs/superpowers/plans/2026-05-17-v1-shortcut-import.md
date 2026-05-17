# V1 Shortcut Import Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Import shortcut settings JSON safely and apply the new bindings immediately.

**Architecture:** `ShortcutSettingsModel` validates imported command IDs and conflicts before `SettingsStore` persists the JSON. `MainWindow` exposes a testable import helper and wires the settings-page import button to a file picker.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Add `importShortcutSettingsFromPath`.
- Reject invalid JSON, unknown commands, empty shortcuts, and conflicts through the existing model.
- Persist valid imported JSON and reapply shortcuts to live actions.
- Wire the settings-page import button.

Excluded:

- Inline shortcut editing UI.
- Workspace-specific keymaps.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add failing MainWindow import tests.
- [ ] Implement validated import helper.
- [ ] Enable and wire the import button.
- [ ] Run focused tests, full validation, and `git diff --check`.
