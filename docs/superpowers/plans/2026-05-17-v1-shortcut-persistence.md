# V1 Shortcut Persistence Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Persist shortcut settings JSON in the existing user settings store.

**Architecture:** `ShortcutSettingsModel` owns validation and JSON shape. `SettingsStore` only saves and loads the JSON object so future shortcut UI can round-trip import/export data without a second persistence backend.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Store shortcut settings JSON under user settings.
- Return an empty/default JSON object when no shortcut settings exist.
- Preserve arbitrary future JSON fields for forward-compatible import/export.

Excluded:

- Shortcut editor UI.
- Applying shortcut overrides to `QAction`.
- Workspace-specific shortcuts.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing SettingsStore shortcut JSON persistence test.
- [x] Add `shortcutSettingsJson` and `saveShortcutSettingsJson`.
- [x] Persist compact JSON through `QSettings`.
- [x] Keep missing/invalid data as an empty safe object.
- [x] Run focused tests, full validation, and `git diff --check`.
