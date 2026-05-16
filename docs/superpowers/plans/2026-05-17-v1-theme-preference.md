# V1 Theme Preference Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first user theme preference boundary before full light/dark application work.

**Architecture:** Persist a small theme preference string in `SettingsStore`, expose it through `SettingsDialogModel`, and keep the current app styling unchanged in this slice.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Default theme preference.
- Persisted theme preference.
- Arabic theme label in settings dialog state.
- Safe fallback for invalid preference values.

Excluded:

- Full light theme stylesheet.
- Theme switching controls in the dialog.
- Runtime palette migration.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing SettingsStore and SettingsDialogModel theme tests.
- [x] Add `themePreference` and `setThemePreference`.
- [x] Map theme preference to Arabic labels.
- [x] Fall back to dark for invalid values.
- [x] Run focused tests, full validation, and `git diff --check`.
