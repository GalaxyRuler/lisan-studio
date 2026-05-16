# V1 Rerun Last Runtime Action Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let users rerun the most recent runtime launch through the command registry without adding terminal or task execution.

**Architecture:** Reuse `RuntimeHistory` to remember launch snapshots. `MainWindow` records a launch before starting a process, exposes `run.rerunLast`, and reruns by rebuilding the same `RuntimeLaunchPlan` through `RuntimeRunner`. Process setup is extracted into a small helper so run/lint/format/rerun share one path.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Record runtime launch plans in `MainWindow`.
- Preserve `reloadAfterSuccess` in `RuntimeHistoryEntry`.
- Add `run.rerunLast` command.
- Add a focused MainWindow rerun test.

Excluded:

- Run history UI.
- Named run configurations.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing command/behavior tests for rerun last.
- [x] Extend `RuntimeHistoryEntry` with reload behavior.
- [x] Extract launch-plan startup helper.
- [x] Record runtime launches and implement rerun last.
- [x] Run focused tests, full validation, and `git diff --check`.
