# V1 Project Replace Preview Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a non-destructive project-wide replace preview that finds candidate replacements across safe project files and open buffers without writing files.

**Architecture:** Create a pure `ProjectReplaceService` that returns preview rows and per-file summaries, then expose those rows through the existing bottom search/results panel and command registry. This slice deliberately stops before bulk writes; the next slice can add per-match/per-file acceptance and atomic apply after the preview contract is stable.

**Tech Stack:** C++17, Qt 6 Core/Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Preview rows for replacement candidates across project files.
- UTF-8 `.apy` and mixed Arabic/English matching.
- Ignored directory and file-size behavior aligned with `SearchService`.
- Open dirty editor text participates as an immediate preview source and de-duplicates against disk rows.
- Command registry ID `replace-in-project`.
- Minimal RTL preview UI in the existing search results panel.

Excluded:

- Writing changes to disk.
- Per-match and per-file accept/reject checkboxes.
- Regex, whole-word, or case-sensitive controls.
- File watcher conflict UI beyond the existing document safety metadata.
- GUI/MSI validation on active `WHITEDRAGON`.

## File Structure

- Create: `src/ProjectReplaceService.h`
- Create: `src/ProjectReplaceService.cpp`
  - Pure project/text replace preview discovery.
- Modify: `src/SearchService.h`
- Modify: `src/SearchService.cpp`
  - Reuse openable-file scanning constants through compatible behavior, not a shared refactor in this slice.
- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
  - Add command and minimal project replace preview rendering.
- Modify: `CMakeLists.txt`
  - Add service files to `acs_core`.
- Modify: `tests/TestProjectSearchRuntime.cpp`
  - Pure service tests.
- Modify: `tests/TestMainWindow.cpp`
  - Command/UI integration tests.

---

## Task 1: Pure Project Replace Preview Service

**Files:**

- Create: `src/ProjectReplaceService.h`
- Create: `src/ProjectReplaceService.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/TestProjectSearchRuntime.cpp`

- [ ] Write failing tests for Arabic/mixed preview rows, ignored directory skipping, and replacement text previews.
- [ ] Run `cmake --build build --target acs_project_runtime_tests` and `.\build\acs_project_runtime_tests.exe` to confirm red on missing `ProjectReplaceService.h`.
- [ ] Implement `ProjectReplacePreviewRow`, `ProjectReplaceFileSummary`, and `ProjectReplacePreview`.
- [ ] Implement `previewText(path, text, query, replacement, limit)` and `previewProject(root, query, replacement, limit)`.
- [ ] Run `.\scripts\validate.ps1`, `git diff --check`, commit `feat: add project replace preview service`.

## Task 2: Immediate Dirty Editor Preview Merge

**Files:**

- Modify: `src/ProjectReplaceService.h`
- Modify: `src/ProjectReplaceService.cpp`
- Test: `tests/TestProjectSearchRuntime.cpp`

- [ ] Write a failing test that `mergePreviewRows(immediate, project)` preserves immediate rows first and removes duplicate file/line rows from disk.
- [ ] Run `acs_project_runtime_tests` and confirm red on missing merge API.
- [ ] Implement `mergePreviewRows`.
- [ ] Run `.\scripts\validate.ps1`, `git diff --check`, commit `feat: merge project replace previews`.

## Task 3: MainWindow Command And Preview UI

**Files:**

- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
- Test: `tests/TestMainWindow.cpp`

- [ ] Write failing tests that the command palette exposes `replace-in-project`.
- [ ] Write failing tests that invoking `previewProjectReplace` with find/replace inputs renders preview rows with file, line, replacement preview, and an explicit preview-only detail.
- [ ] Add compact project replace inputs near the existing project command box:
  - `projectReplaceInput`
  - `projectReplacePreviewButton`
- [ ] Register `replace-in-project`.
- [ ] Add `previewProjectReplace`, `currentEditorReplacePreviewRows`, and `renderProjectReplacePreview`.
- [ ] Run `.\scripts\validate.ps1`, `git diff --check`, commit `feat: add project replace preview ui`.

## Final Verification

- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"`.
- [ ] Run `git diff --check`.
- [ ] Confirm `git status --short` is clean.
- [ ] Do not run GUI/MSI validation on active `WHITEDRAGON`.
