# V1 Project Replace Acceptance Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add non-destructive per-match and per-file acceptance state to the project replace preview.

**Architecture:** Extend `ProjectReplaceService` with a small pure selection model that can accept/reject rows and whole files without applying writes. Then render row checkboxes and file-level controls in the existing preview panel so the next slice can consume the accepted rows for atomic apply.

**Tech Stack:** C++17, Qt 6 Core/Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Pure selected-row model for replace preview rows.
- Accept/reject one row.
- Accept/reject every row in a file.
- Count accepted matches.
- Preview UI row checkboxes checked by default.
- File-level accept/reject buttons for the selected preview row's file.
- Tests proving toggles update UI state without writing files.

Excluded:

- Applying replacements to disk.
- Backup files or atomic write orchestration.
- External-change conflict resolution beyond existing document safety metadata.
- Regex, whole-word, or case-sensitive replacement.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

### Task 1: Pure Project Replace Selection State

**Files:**

- Modify: `src/ProjectReplaceService.h`
- Modify: `src/ProjectReplaceService.cpp`
- Test: `tests/TestProjectSearchRuntime.cpp`

- [ ] Write a failing test for a selection built from preview rows where all rows are accepted by default.
- [ ] Write a failing test for rejecting one row and rejecting an entire file.
- [ ] Run `cmake --build build --target acs_project_runtime_tests` and confirm red on missing selection API.
- [ ] Implement `ProjectReplaceSelectionState` and `ProjectReplaceService::selectionFromRows`.
- [ ] Implement `setRowAccepted`, `setFileAccepted`, `isRowAccepted`, `acceptedRows`, and `acceptedMatchCount`.
- [ ] Run `.\scripts\validate.ps1`, `git diff --check`, commit `feat: add project replace selection state`.

### Task 2: Preview Row Checkboxes

**Files:**

- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
- Test: `tests/TestMainWindow.cpp`

- [ ] Write a failing test that preview rows render `projectReplaceAcceptCheckBox` widgets checked by default.
- [ ] Write a failing test that unchecking a row checkbox updates the row item's accepted data and does not write the file.
- [ ] Add an in-memory preview selection state to `MainWindow`.
- [ ] Render a checkbox in each project replace preview row.
- [ ] Wire checkbox toggles into the selection state.
- [ ] Run `.\scripts\validate.ps1`, `git diff --check`, commit `feat: add replace preview row acceptance`.

### Task 3: File-Level Acceptance Controls

**Files:**

- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
- Test: `tests/TestMainWindow.cpp`

- [ ] Write a failing test that file-level reject/accept buttons toggle all preview rows for that file.
- [ ] Add `projectReplaceAcceptFileButton` and `projectReplaceRejectFileButton` to the preview UI.
- [ ] Wire buttons to the current preview row's file using the selection state.
- [ ] Update row checkboxes after file-level toggles.
- [ ] Run `.\scripts\validate.ps1`, `git diff --check`, commit `feat: add replace preview file acceptance`.

## Final Verification

- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"`.
- [ ] Run `git diff --check`.
- [ ] Confirm `git status --short` is clean.
- [ ] Do not run GUI/MSI validation on active `WHITEDRAGON`.
