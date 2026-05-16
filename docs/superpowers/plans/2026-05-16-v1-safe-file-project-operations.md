# V1 Safe File Project Operations Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Harden project-tree file operations before project-wide replace by centralizing preflight checks and routing visible file/project actions through command IDs.

**Architecture:** Add a small pure `ProjectFileOperations` service for path/name/collision/root-scope checks, then wire MainWindow project-tree actions and command registry metadata through the same operation surface. This slice avoids project-wide replace and avoids GUI/MSI validation; it creates the command and safety foundation that replace preview will consume next.

**Tech Stack:** C++17, Qt 6 Core/Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Pure file-operation preflight checks for create, rename, delete, reveal, copy path, and open containing folder.
- Tests for invalid child names, path escaping, root protection, and collision handling.
- Command registry IDs for project/file lifecycle actions.
- Context action `commandId` properties to keep visible surfaces drift-free.
- Copy path and open containing folder actions.
- MainWindow uses the preflight helper before create/rename/delete operations.

Excluded:

- Project-wide replace.
- Full asynchronous file watcher.
- Reload/compare UI for external modification.
- MSI/installed GUI validation.

## Tasks

### Task 1: Project File Operation Preflight Service

**Files:**

- Create: `src/ProjectFileOperations.h`
- Create: `src/ProjectFileOperations.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/TestProjectSearchRuntime.cpp`

- [ ] Write failing tests for safe child path, rename collision, root delete protection, and containing-folder target.
- [ ] Run `acs_project_runtime_tests` and confirm compile failure on missing header.
- [ ] Implement `ProjectFileOperations` with `childTarget`, `renameTarget`, `canDelete`, `pathForClipboard`, and `containingFolder`.
- [ ] Run `.\scripts\validate.ps1`, `git diff --check`, commit `feat: add project file operation preflight`.

### Task 2: Command Registry Coverage For Project/File Operations

**Files:**

- Modify: `src/MainWindow.cpp`
- Modify: `tests/TestMainWindow.cpp`

- [ ] Write failing tests that command palette exposes `project.refresh`, `project.file.new`, `project.folder.new`, `project.item.open`, `project.item.rename`, `project.item.deleteWithPrompt`, `project.item.reveal`, `project.item.copyPath`, and `project.item.openContainingFolder`.
- [ ] Write failing tests that project tree actions expose matching `commandId` properties.
- [ ] Register the commands in `MainWindow::registerWorkbenchCommands()`.
- [ ] Add `commandId` properties to project tree actions.
- [ ] Run validation and commit `feat: add project file operation commands`.

### Task 3: Copy/Open Containing Folder And Preflight Integration

**Files:**

- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
- Test: `tests/TestMainWindow.cpp`

- [ ] Write failing tests for `projectTreeCopyPathAction` copying the current path to the clipboard.
- [ ] Add `projectTreeCopyPathAction` and `projectTreeOpenContainingFolderAction`.
- [ ] Route create/rename/delete through `ProjectFileOperations` preflight results.
- [ ] Keep delete confirmation intact and root deletion blocked.
- [ ] Run validation and commit `feat: wire safe project tree operations`.

## Final Verification

- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"`.
- [ ] Run `git diff --check`.
- [ ] Confirm `git status --short` is clean.
- [ ] Do not run GUI/MSI validation on active `WHITEDRAGON`.
