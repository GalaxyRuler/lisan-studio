# V1 Keyboard Focus Order Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make keyboard-only navigation across the primary workbench surfaces explicit and testable.

**Architecture:** Keep this as a `MainWindow` shell slice. The primary command box, project tree, editor tabs/editor, bottom panel, and status surfaces get declared focus order metadata and `setTabOrder` wiring.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Explicit focus-order metadata for primary shell surfaces.
- Tab-focus policies for keyboard navigation.
- `setTabOrder` wiring across the main loop.

Excluded:

- Pixel/UI automation.
- Screen-reader audit beyond stable widget metadata.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add failing MainWindow focus-order test.
- [ ] Set focus policies and focus-order metadata.
- [ ] Wire tab order for primary shell surfaces.
- [ ] Run focused tests, full validation, and `git diff --check`.
