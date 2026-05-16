# V1 Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Route current workbench commands through stable APIs so future IDE features do not grow `MainWindow` by default.

**Architecture:** Add a command registry first, then extract focused services only when a touched area needs one. Preserve Qt object names and Arabic/RTL behavior while moving behavior out of ad hoc UI code.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Tasks

- [ ] Add command registry data types and tests for stable command IDs, Arabic labels, categories, shortcuts, enablement, and trigger callbacks.
- [ ] Register current workbench commands: new, open file, save, save as, open folder, run, lint, format, cancel run, project search, settings, and command palette.
- [ ] Replace command palette's local command list with command registry data while keeping object names stable.
- [ ] Add a regression test that every visible command surface has a matching registry ID.
- [ ] Extract editor session state after the command registry is green.
- [ ] Extract project operations, search dispatch, runtime orchestration, and settings construction in separate green slices.
- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"` after each slice.
