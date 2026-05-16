# V1 Theme Application Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Apply the persisted dark/light theme preference to the main workbench stylesheet.

**Architecture:** Keep theme application in `MainWindow` for this narrow slice. The stored `SettingsStore` preference selects a dark or light root stylesheet, and the settings dialog reapplies it after saving.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Main window applies persisted light preference at startup.
- Main window marks the active theme as a stable property for tests/future UI.
- Settings dialog reapplies theme after Apply.

Excluded:

- Per-widget custom palette migration outside the existing root stylesheet.
- Full visual screenshot validation.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing MainWindow tests for persisted and applied theme preference.
- [x] Extract stylesheet selection behind a theme application helper.
- [x] Add a light stylesheet variant while preserving dark default behavior.
- [x] Reapply theme after settings dialog Apply.
- [x] Run focused tests, full validation, and `git diff --check`.
