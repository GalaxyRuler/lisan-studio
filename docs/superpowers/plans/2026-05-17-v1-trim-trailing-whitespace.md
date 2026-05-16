# V1 Trim Trailing Whitespace Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an explicit trim-trailing-whitespace-on-save setting that is off by default and safe when enabled.

**Architecture:** Keep the save-time transform inside `EditorSurface` before it delegates to `DocumentFileIO::saveUtf8Atomically`. The editor buffer should reflect the exact saved text and be marked clean after a successful save.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Editor-level trim-on-save getter/setter.
- Default disabled behavior.
- Save-time trimming of trailing spaces and tabs.
- Command palette and View menu command for toggling the setting.

Excluded:

- Persistent user/workspace setting.
- Format-on-save integration.
- Parser-aware string/comment exceptions.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Write failing editor tests for default-off preservation and enabled trimming.
- [ ] Add `EditorSurface::setTrimTrailingWhitespaceOnSave`.
- [ ] Apply the transform before atomic save and keep the editor buffer clean after save.
- [ ] Write failing MainWindow command/menu tests.
- [ ] Add `editor.toggleTrimTrailingWhitespace` command.
- [ ] Run focused tests, full validation, and `git diff --check`.
