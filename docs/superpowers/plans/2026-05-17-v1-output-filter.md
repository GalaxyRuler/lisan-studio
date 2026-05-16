# V1 Output Transcript Filter Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a pure output transcript model that can render filtered runtime output before adding UI controls.

**Architecture:** Introduce `OutputTranscript` in `acs_core`. It records labeled output chunks and renders them in the same `[label]\ntext` format currently used by `MainWindow`. Filtering is data-only: callers can include/exclude stdout, stderr, and system chunks and apply a case-insensitive text query.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Output transcript chunk data shape.
- Append, clear, and full render behavior.
- Channel filtering for stdout, stderr, and system chunks.
- Case-insensitive Arabic/English text query filtering.

Excluded:

- UI filter controls.
- Changes to runtime process execution.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing pure output transcript tests.
- [x] Add `OutputTranscript` to `acs_core`.
- [x] Implement channel and query filtering.
- [x] Run focused tests, full validation, and `git diff --check`.
