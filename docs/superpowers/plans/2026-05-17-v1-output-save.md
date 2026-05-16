# V1 Output Transcript Save Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let users save the output transcript as UTF-8 text without adding terminal/task behavior.

**Architecture:** Add a thin `MainWindow` command for `output.saveAs`. The dialog-facing slot delegates to a path-based save helper so tests can verify UTF-8 output writing without GUI automation.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- `output.saveAs` command.
- Visible RTL menu row.
- Path-based output transcript save helper.
- MainWindow test that saves Arabic output text to UTF-8.

Excluded:

- Output filtering.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing output transcript save test and command expectation.
- [x] Add save command/menu row and path helper.
- [x] Use atomic UTF-8 file writing for output transcript.
- [x] Run focused tests, full validation, and `git diff --check`.
