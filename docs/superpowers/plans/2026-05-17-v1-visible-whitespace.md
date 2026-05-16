# V1 Visible Whitespace Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make visible whitespace an explicit editor setting and command instead of an implicit constructor-only behavior.

**Architecture:** Add a small `EditorSurface` API that toggles `QTextOption::ShowTabsAndSpaces`, then expose it through the command registry and View menu. Keep the default enabled to preserve the current editor behavior.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Editor-level visible whitespace getter/setter.
- Default visible whitespace remains enabled.
- Command palette and View menu command for toggling whitespace.

Excluded:

- Persistent user setting.
- Separate tab/space rendering styles.
- Indentation guides.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Write failing editor test for toggling `ShowTabsAndSpaces`.
- [ ] Add `EditorSurface::setVisibleWhitespaceEnabled` and `isVisibleWhitespaceEnabled`.
- [ ] Write failing MainWindow tests for command registry/menu exposure.
- [ ] Add `editor.toggleVisibleWhitespace` command and View menu row.
- [ ] Run focused tests, full validation, and `git diff --check`.
