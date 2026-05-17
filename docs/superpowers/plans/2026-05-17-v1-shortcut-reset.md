# V1 Shortcut Reset Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let users recover from custom keymaps by resetting shortcuts to defaults.

**Architecture:** Keep reset as a workbench helper that clears persisted shortcut JSON and reapplies command defaults to live actions. The settings page gets a stable reset button.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Add `resetShortcutSettingsToDefaults`.
- Clear persisted shortcut settings.
- Reapply default shortcuts to live actions.
- Add a settings-page reset button.

Excluded:

- Inline shortcut editing UI.
- Per-workspace keymaps.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add failing MainWindow reset test.
- [ ] Implement reset helper.
- [ ] Wire the settings-page reset button.
- [ ] Run focused tests, full validation, and `git diff --check`.
