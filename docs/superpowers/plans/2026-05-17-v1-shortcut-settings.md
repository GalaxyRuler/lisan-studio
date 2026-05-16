# V1 Shortcut Settings Model Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the pure shortcut binding model that future shortcut editor UI can use for import/export and command-registry validation.

**Architecture:** Keep this in `acs_core`. The model consumes `CommandRegistry` metadata, stores shortcut overrides by command id, serializes to JSON, and rejects unknown command ids or shortcut conflicts.

**Tech Stack:** C++17, Qt 6 Core/Gui/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Effective shortcut lookup using command defaults plus overrides.
- JSON export/import data shape.
- Validation for unknown command IDs, empty shortcuts, and conflicts.

Excluded:

- Shortcut editor dialog UI.
- Persisting shortcuts into `SettingsStore`.
- Applying overrides to live `QAction` objects.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing pure shortcut settings tests.
- [x] Add `ShortcutSettingsModel` to `acs_core`.
- [x] Implement effective shortcut lookup.
- [x] Implement JSON export/import with validation.
- [x] Run focused tests, full validation, and `git diff --check`.
