# V1 Shortcut Override Helper Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the validated helper that future shortcut editor controls can use to change one command binding.

**Architecture:** `MainWindow` delegates command-id and conflict validation to `ShortcutSettingsModel`, persists successful changes through `SettingsStore`, and reapplies shortcuts to live actions immediately.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Add `setShortcutOverrideForCommand`.
- Persist a valid single-command override.
- Reject conflicts without changing persisted settings.
- Apply successful overrides to live actions.

Excluded:

- Inline shortcut editing widgets.
- Per-workspace keymaps.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add failing MainWindow helper tests.
- [ ] Implement helper through `ShortcutSettingsModel`.
- [ ] Run focused tests, full validation, and `git diff --check`.
