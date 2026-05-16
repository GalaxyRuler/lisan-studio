# V1 Workspace Settings Application Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Apply safe project-local workspace settings to editor sessions when a project is loaded.

**Architecture:** Keep `WorkspaceSettingsStore` as the pure persistence boundary. `MainWindow` loads settings after a project root is accepted, stores the current `WorkspaceSettings`, and applies editor-scoped preferences to every existing and newly-created `EditorSurface`. This slice still defines no terminal/task execution behavior.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Load `.lisan-workspace/settings.json` from the accepted project root.
- Apply `editor.trimTrailingWhitespaceOnSave` to open editors.
- Apply the setting to editors created after the project is loaded.
- Preserve safe defaults when workspace settings are missing or invalid.

Excluded:

- UI for editing workspace settings.
- Workspace trust prompts.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Write failing `MainWindow` test for workspace trim-on-save application.
- [x] Add a `MainWindow` workspace settings member and editor application helper.
- [x] Load settings on project load and apply them to existing/new editor tabs.
- [x] Run focused tests, full validation, and `git diff --check`.
