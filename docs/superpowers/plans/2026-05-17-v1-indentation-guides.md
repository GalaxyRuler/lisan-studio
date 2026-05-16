# V1 Indentation Guides Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add lightweight indentation guides for `.apy` editing without changing the text model.

**Architecture:** Keep guide computation and painting in `EditorSurface`. Compute guide levels from leading spaces/tabs, default guides on, and expose a small editor toggle for future settings integration.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Editor-level indentation guide getter/setter.
- Testable guide-level computation.
- Paint overlay for visible blocks.

Excluded:

- Per-project/user persistence.
- Active indent scope highlighting.
- Parser-aware block guides.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Write failing editor tests for default toggle and guide-level computation.
- [ ] Add `EditorSurface` indentation guide API.
- [ ] Draw lightweight guide lines for visible blocks.
- [ ] Run focused tests, full validation, and `git diff --check`.
