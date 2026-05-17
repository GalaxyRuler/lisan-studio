# V1 Shortcut Application Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Apply persisted shortcut overrides to live workbench actions on startup.

**Architecture:** Keep shortcut validation in `ShortcutSettingsModel` and persistence in `SettingsStore`. `MainWindow` loads the persisted JSON after constructing command surfaces and updates `QAction` shortcuts by stable command ID.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Load user shortcut settings during workbench construction.
- Apply effective shortcuts to all live `QAction` instances that declare a registered command ID.
- Keep invalid shortcut JSON non-fatal.

Excluded:

- Shortcut editor dialog UI.
- Workspace-specific shortcut overrides.
- Import/export UI.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add failing MainWindow test for persisted shortcut override application.
- [ ] Add `MainWindow::applyShortcutSettings`.
- [ ] Call it after command surfaces are created.
- [ ] Run focused tests, full validation, and `git diff --check`.
