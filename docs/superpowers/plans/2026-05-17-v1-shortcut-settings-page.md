# V1 Shortcut Settings Page Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first keyboard-shortcut settings surface so users can inspect current command bindings before editing/import/export lands.

**Architecture:** Reuse `CommandRegistry`, `ShortcutSettingsModel`, and persisted settings. `MainWindow` renders a read-only list of commands with effective shortcut text and stable import/export buttons reserved for the next slice.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Add an RTL shortcuts category to the settings dialog.
- Render registered commands with effective shortcut metadata.
- Add stable import/export button object names.

Excluded:

- Shortcut editing controls.
- Import/export behavior.
- Workspace-specific shortcuts.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add failing settings-dialog shortcut page tests.
- [ ] Add shortcut category to `SettingsDialogModel`.
- [ ] Add read-only shortcuts page to `MainWindow::openSettings`.
- [ ] Run focused tests, full validation, and `git diff --check`.
