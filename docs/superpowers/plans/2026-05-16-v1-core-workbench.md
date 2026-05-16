# V1 Core Workbench Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Lisan Studio dependable for daily `.apy` editing, project navigation, search, replace, running, output inspection, and session recovery.

**Architecture:** Add daily-workflow features through the command registry and focused services created in V1 Foundation. Keep `QPlainTextEdit` as the editor core and preserve Arabic-first RTL shell behavior with LTR islands for code, paths, commands, and output.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, bundled Python runtime, PowerShell QA scripts.

---

## Tasks

- [ ] Add Document & Safety Foundation before find/replace: document registry, file identity, encoding/line-ending policy, unsaved-change guard, minimal file watcher, atomic save, and editor IO migration.
- [ ] Add in-file find/replace with RTL-aware match navigation and editor tests.
- [ ] Add project-wide replace with preview and per-file acceptance.
- [ ] Add snippets, bracket/quote matching, auto-indent, indentation guides, visible whitespace, and trim-trailing-whitespace setting.
- [ ] Add split editor panes, side-by-side compare, persistent tabs, recent files, recent projects, and session restore.
- [ ] Add safe file/project operations: new folder, rename, delete confirmation, reveal, copy path, file watcher reload/compare/keep-current.
- [ ] Add workspace settings separate from user settings.
- [ ] Add named run configurations, run history, rerun last command, output filter/copy/clear/save, and stronger cancellation cleanup.
- [ ] Add a real local terminal abstraction with platform shell profiles.
- [ ] Add shortcut editor, status indicators, dark/light theme foundation, focus-order tests, and keyboard navigation tests.

## Sequencing Notes

- Document/session safety is a prerequisite for find/replace and project-wide replace. Do not begin replace features until the document registry, unsaved-change guard, atomic save path, and external-change metadata are green.
- Project-wide replace must follow the safer file/project operation foundation because it is the largest V1 data-loss surface.
- Multi-cursor and column selection require a `QPlainTextEdit` feasibility spike before they are committed to V1.0 scope. If the spike is fragile, defer them to V1.5.
- Terminal work is gated by workspace settings and workspace trust primitives. Do not add terminal execution routes before trust data shapes exist.
- V1 should expose a document-edit interface that can later carry V2 language-service edits, diagnostics versions, and formatting/rename operations.
