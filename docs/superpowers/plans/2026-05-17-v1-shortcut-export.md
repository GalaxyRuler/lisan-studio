# V1 Shortcut Export Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make shortcut settings export real through a testable helper and settings-page button wiring.

**Architecture:** `SettingsStore` remains the source of persisted shortcut JSON. `MainWindow` writes that JSON to a user-selected path through `DocumentFileIO`, with an empty versioned shortcut object when no overrides exist.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Add `exportShortcutSettingsToPath` helper.
- Save UTF-8 JSON using the document IO helper.
- Wire the settings page export button to the helper through a save-file dialog.

Excluded:

- Shortcut import.
- Shortcut editing UI.
- Workspace-specific keymaps.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add failing MainWindow shortcut export test.
- [ ] Implement export helper.
- [ ] Enable and wire the export button.
- [ ] Run focused tests, full validation, and `git diff --check`.
