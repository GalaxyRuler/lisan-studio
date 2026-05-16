# V1 Output Panel Actions Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first runtime-output workflow actions so users can copy or clear run output from command surfaces.

**Architecture:** Keep output behavior in `MainWindow` for this narrow slice, but expose actions through the command registry and visible menu command IDs so command surfaces do not drift. This slice does not add terminal execution or file-dialog-based output saving.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- `output.copy` command.
- `output.clear` command.
- Visible RTL menu rows for both actions.
- Tests that copy uses the clipboard and clear empties the output panel.
- Command palette and command-surface drift coverage.

Excluded:

- Save output to file.
- Output filtering.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing `MainWindow` output action tests.
- [x] Register output commands and visible menu rows.
- [x] Implement copy/clear slots.
- [x] Run focused tests, full validation, and `git diff --check`.
