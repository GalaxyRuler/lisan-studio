# V1 Auto Indent Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Enter in the editor preserve indentation and add one level after block-opening colon lines.

**Architecture:** Keep auto-indent in `EditorSurface::keyPressEvent` so it follows the actual editing path and remains testable with Qt key events. Use simple logical-line rules and avoid parser coupling.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Preserve leading spaces from the current line when pressing Enter.
- Add one four-space indent level when the trimmed current line ends with `:`.
- Preserve undo behavior by inserting through the active `QTextCursor`.

Excluded:

- Parser-aware dedent.
- Tabs/spaces preference UI.
- Smart indentation inside multi-line strings.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Write failing editor tests for colon block indentation and existing-indent preservation.
- [ ] Override `EditorSurface::keyPressEvent`.
- [ ] Insert newline plus computed indentation for Return/Enter.
- [ ] Run focused editor tests, full validation, and `git diff --check`.
