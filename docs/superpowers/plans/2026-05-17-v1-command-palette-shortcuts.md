# V1 Command Palette Shortcut Display Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show persisted shortcut overrides in the command palette instead of stale default shortcuts.

**Architecture:** Keep command metadata unchanged. `MainWindow::openCommandPalette` loads the persisted shortcut model once and renders the effective shortcut for each command row.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Use persisted shortcut overrides for command palette row metadata.
- Use persisted shortcut overrides for the visible shortcut label.
- Fall back to command defaults when persisted shortcut JSON is missing or invalid.

Excluded:

- Shortcut editor dialog UI.
- Import/export UI.
- Workspace shortcut settings.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add failing MainWindow test for command palette effective shortcut display.
- [ ] Load `ShortcutSettingsModel` in command palette rendering.
- [ ] Render effective shortcut text in item metadata and labels.
- [ ] Run focused tests, full validation, and `git diff --check`.
