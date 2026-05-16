# V1 Output Filter UI Wiring Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wire the pure output transcript model into `MainWindow` and expose command-driven output filters.

**Architecture:** `MainWindow` records output through `OutputTranscript` and renders the selected filter into the existing `outputPanel`. Visible command surfaces switch between all/stdout/stderr/system filters without changing the bottom panel layout.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- `output.filter.all`, `output.filter.stdout`, `output.filter.stderr`, and `output.filter.system` commands.
- Visible menu rows for filter commands.
- MainWindow transcript rendering through `OutputTranscript`.
- Test that filtering hides and restores system output.

Excluded:

- Text search box in the output panel.
- New panel layout.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing MainWindow command/filter tests.
- [x] Add transcript/filter members and render helper.
- [x] Route output writes/appends/clear through `OutputTranscript`.
- [x] Register visible filter commands.
- [x] Run focused tests, full validation, and `git diff --check`.
