# V1 Project Replace Apply Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Apply accepted project-replace preview rows with atomic UTF-8 saves while refusing stale or unsafe inputs.

**Architecture:** Extend `ProjectReplaceService` with pure apply planning and a disk apply helper that reloads each file, verifies every accepted preview row still matches the current file text, then saves with `DocumentFileIO::saveUtf8Atomically`. Wire MainWindow to only call apply after the user has preview rows and no dirty open document conflicts.

**Tech Stack:** C++17, Qt 6 Core/Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Apply accepted preview rows only.
- Refuse stale rows when current file text no longer contains the preview `before` text at the expected line.
- Group row changes per file and save once per file.
- Atomic UTF-8 save through `DocumentFileIO`.
- UI command/button for applying accepted rows after preview.
- Dirty open-buffer guard before apply.

Excluded:

- Backup `.bak` retention.
- Complex conflict UI or compare/reload view.
- Regex, whole-word, or case-sensitive replacement.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

### Task 1: Pure Safe Apply Service

**Files:**

- Modify: `src/ProjectReplaceService.h`
- Modify: `src/ProjectReplaceService.cpp`
- Test: `tests/TestProjectSearchRuntime.cpp`

- [ ] Write failing tests that accepted preview rows update temp files atomically.
- [ ] Write failing tests that stale disk content is refused and left unchanged.
- [ ] Run `acs_project_runtime_tests` and confirm red on missing apply API.
- [ ] Add `ProjectReplaceApplyResult`.
- [ ] Implement `applyAcceptedRows(rows, error)` by grouping rows per file, verifying current line text, replacing expected line text, and saving via `DocumentFileIO::saveUtf8Atomically`.
- [ ] Run `.\scripts\validate.ps1`, `git diff --check`, commit `feat: apply accepted project replace rows`.

### Task 2: MainWindow Apply Command And Dirty Guard

**Files:**

- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
- Test: `tests/TestMainWindow.cpp`

- [ ] Write failing test that `replace-in-project.applyAccepted` is exposed in command palette and command surfaces.
- [ ] Write failing test that applying checked rows updates the file and unchecked rows are skipped.
- [ ] Write failing test that a dirty open editor for an affected path blocks apply and leaves disk unchanged.
- [ ] Add `projectReplaceApplyButton`.
- [ ] Register command `replace-in-project.applyAccepted`.
- [ ] Track last preview rows in `MainWindow`.
- [ ] Add `applyAcceptedProjectReplaceRows`.
- [ ] Add dirty open-document conflict check before service apply.
- [ ] Run `.\scripts\validate.ps1`, `git diff --check`, commit `feat: wire project replace apply`.

## Final Verification

- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"`.
- [ ] Run `git diff --check`.
- [ ] Confirm `git status --short` is clean.
- [ ] Do not run GUI/MSI validation on active `WHITEDRAGON`.
