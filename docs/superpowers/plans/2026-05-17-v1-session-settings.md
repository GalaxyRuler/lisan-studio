# V1 Session Settings Persistence Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Persist the minimal workbench session shape that later MainWindow restore can consume.

**Architecture:** Add a pure `SettingsStore` snapshot for project root, open file paths, active file index, and selected bottom panel. Keep this slice UI-free so restore behavior can be tested separately.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Save/load project root.
- Save/load open file paths.
- Save/load active file index.
- Save/load selected bottom panel id.

Excluded:

- MainWindow restore on startup.
- Dirty buffer journal/recovery.
- Split-pane layout persistence.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Write failing SettingsStore tests for defaults and persisted session snapshot.
- [ ] Add `SavedWorkbenchSession`.
- [ ] Implement save/load through `QSettings`.
- [ ] Run focused tests, full validation, and `git diff --check`.
