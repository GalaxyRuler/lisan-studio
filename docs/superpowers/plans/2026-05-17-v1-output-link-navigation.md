# V1 Output Link Navigation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let users navigate from output panel file links back into the editor.

**Architecture:** Keep terminal/process execution out of scope. Reuse the existing rendered output text and `TerminalLinkParser`; `MainWindow` owns the workbench action of opening the file and moving the editor cursor.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Parse the current rendered output panel text for `path:line:column` links.
- Open the target file and move to the parsed line/column.
- Support direct invocation and double-click from the output panel viewport.

Excluded:

- Terminal process execution.
- Rich text output rendering.
- Filesystem watching.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add a failing MainWindow test for opening an output link at the cursor.
- [ ] Add a private output-link navigation slot.
- [ ] Wire output-panel double-clicks to that slot.
- [ ] Run focused tests, full validation, and `git diff --check`.
