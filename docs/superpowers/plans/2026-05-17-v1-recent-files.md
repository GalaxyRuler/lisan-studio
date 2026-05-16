# V1 Recent Files Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Persist recently opened files alongside recent projects.

**Architecture:** Extend `SettingsStore` with a small recent-files list and update `MainWindow::openEditorFile` after successful opens. Keep UI surfacing for a later command/menu slice.

**Tech Stack:** C++17, Qt 6 Core/Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Recent files persistence.
- Deduping and most-recent-first ordering.
- Cap recent files at 20 entries.
- Track files opened through MainWindow.

Excluded:

- Recent files menu UI.
- Clear recent files command.
- Workspace-scoped recent files.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Write failing SettingsStore tests for recent files.
- [ ] Add `recentFiles` and `addRecentFile`.
- [ ] Write failing MainWindow test that opening a file records it.
- [ ] Wire successful `openEditorFile` to `SettingsStore`.
- [ ] Run focused tests, full validation, and `git diff --check`.
