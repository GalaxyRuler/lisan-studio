# V1 Runtime Run History Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a pure run-history foundation for future rerun/history UI without expanding runtime execution behavior.

**Architecture:** Add `RuntimeHistory` to `acs_core`. It records bounded, most-recent-first snapshots from `RuntimeLaunchPlan`, preserving action, title, file path, working directory, and command summary data. This slice does not add process execution routes, terminal behavior, or GUI history UI.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- `RuntimeHistoryEntry` data shape.
- Bounded most-recent-first recording.
- Clear history support.
- Pure service tests in `acs_project_runtime_tests`.

Excluded:

- Rerun command UI.
- Named run configurations.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Write failing pure run-history tests.
- [x] Add `RuntimeHistory` to `acs_core`.
- [x] Implement bounded most-recent-first storage.
- [x] Run focused tests, full validation, and `git diff --check`.
