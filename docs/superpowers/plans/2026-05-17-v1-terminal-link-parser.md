# V1 Terminal Link Parser Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add pure terminal/output file-link detection before real terminal process UI work.

**Architecture:** Add `TerminalLinkParser` in `acs_core`. It detects file path plus optional line/column spans in terminal text and returns structured links for future clickable output/terminal surfaces.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Windows absolute path detection with optional line and column.
- Forward-slash Windows path detection.
- Span offsets for future clickable rendering.

Excluded:

- Starting terminal processes.
- Clickable UI rendering.
- Filesystem existence checks.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing pure terminal link parser tests.
- [x] Add `TerminalLinkParser` to `acs_core`.
- [x] Parse path, line, column, and text span.
- [x] Run focused tests, full validation, and `git diff --check`.
