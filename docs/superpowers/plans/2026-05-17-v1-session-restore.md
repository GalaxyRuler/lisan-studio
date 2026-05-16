# V1 MainWindow Session Restore Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore and save the minimal workbench session snapshot: project root, open files, active editor, and selected bottom panel.

**Architecture:** Consume `SettingsStore::savedWorkbenchSession` in `MainWindow` after UI construction. Save a snapshot from `MainWindow::closeEvent`, using stable bottom panel ids. Keep dirty-buffer recovery and split-pane persistence for later slices.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Constructor overload with explicit settings path for deterministic tests.
- Restore existing project root.
- Restore existing open files and active tab.
- Restore selected bottom panel.
- Save session on close.

Excluded:

- Dirty untitled-buffer crash recovery.
- Split editor panes.
- Missing-file conflict UI beyond skipping missing files.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Write failing MainWindow tests for restore and close-time save.
- [ ] Add explicit settings-path constructor overload.
- [ ] Add `restoreWorkbenchSession` and `saveWorkbenchSession`.
- [ ] Add bottom-panel id mapping helpers.
- [ ] Override `closeEvent` to save the snapshot after unsaved guards allow close.
- [ ] Run focused tests, full validation, and `git diff --check`.
