# V1 Workspace Settings Store Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a project-local workspace settings file and trust data shape before terminal/task execution work.

**Architecture:** Introduce a pure `WorkspaceSettingsStore` that reads and writes `.lisan-workspace/settings.json` below the project root. Missing or invalid files fall back to safe defaults. This slice defines data only; it does not execute workspace-provided commands.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- `.lisan-workspace/settings.json` path helper.
- Safe defaults for missing/invalid files.
- Workspace trust flag.
- Trim-on-save preference shape.
- Default run working-directory shape.

Excluded:

- UI settings page.
- Applying settings to editors.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Write failing pure store tests.
- [ ] Add `WorkspaceSettingsStore` to `acs_core`.
- [ ] Implement JSON load/save with safe invalid fallback.
- [ ] Run focused tests, full validation, and `git diff --check`.
