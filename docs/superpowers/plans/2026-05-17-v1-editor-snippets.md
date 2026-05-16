# V1 Editor Snippets Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a small Arabic-first snippet foundation that can be tested independently and inserted through the command system.

**Architecture:** Introduce a pure `ApySnippetService` in `acs_core` for static `.apy` snippet metadata, then wire one safe insertion command in `MainWindow` that inserts through the active `EditorSurface` cursor.

**Tech Stack:** C++17, Qt 6 Core/Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Static Arabic-first `.apy` snippet catalog.
- Lookup by stable snippet id.
- Snippet cursor offset metadata.
- Command-palette insertion for the print snippet.
- Focused tests for service and UI insertion.

Excluded:

- User-authored snippet persistence.
- Placeholder tab stops or multi-cursor snippet expansion.
- Workspace-scoped snippets.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

### Task 1: Pure Snippet Catalog

**Files:**

- Add: `src/ApySnippetService.h`
- Add: `src/ApySnippetService.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/TestProjectSearchRuntime.cpp`

- [ ] Write failing tests for Arabic-first snippet listing and lookup.
- [ ] Run focused `acs_project_runtime_tests` and confirm red on missing service.
- [ ] Implement `ApySnippetService` with stable ids and cursor offsets.
- [ ] Run focused tests and commit `feat: add apy snippet catalog`.

### Task 2: Command Palette Insertion

**Files:**

- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
- Test: `tests/TestMainWindow.cpp`

- [ ] Write failing tests that the command palette exposes the snippet command.
- [ ] Write failing test that insertion places `اطبع("")` in the active editor with the cursor between quotes.
- [ ] Register command `snippet.insertPrint`.
- [ ] Implement `insertSnippetById`.
- [ ] Run focused tests and commit `feat: wire editor snippet insertion`.

## Final Verification

- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"`.
- [ ] Run `git diff --check`.
- [ ] Confirm `git status --short` is clean.
- [ ] Do not run GUI/MSI validation on active `WHITEDRAGON`.
