# V1 Status Indicators Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add first-class status bar indicators for the active editor and workbench placeholders.

**Architecture:** Keep this as a narrow `MainWindow` slice. Add stable QLabel object names in the status bar and update them from the current `EditorSurface`; do not add Git/runtime execution behavior.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Encoding indicator.
- Line-ending indicator.
- Indentation indicator.
- Language mode indicator.
- Runtime idle indicator.
- Git placeholder indicator.

Excluded:

- Git repository detection.
- Runtime health probing in the status bar.
- Shortcut editor and theme switching.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing MainWindow tests for status indicator labels.
- [x] Add status labels with stable object names.
- [x] Update editor-derived indicators when the active editor or text changes.
- [x] Keep runtime and Git as explicit placeholders.
- [x] Run focused tests, full validation, and `git diff --check`.
