# V1 Breadcrumb Placeholder Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a lightweight breadcrumb path bar and symbol placeholder above the editor.

**Architecture:** Keep this as a `MainWindow` UI slice. The breadcrumb reflects the active editor path, while the symbol placeholder is intentionally static until V2 language services provide real symbols.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Breadcrumb bar above editor tabs.
- Active file path label with stable object name.
- Symbol placeholder label with stable object name.

Excluded:

- Symbol extraction.
- Clickable breadcrumbs.
- Language-service integration.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing MainWindow test for breadcrumb and symbol placeholder labels.
- [x] Add breadcrumb bar UI above editor tabs.
- [x] Update breadcrumb when the active editor changes or receives a path.
- [x] Keep symbol label as an explicit V2 placeholder.
- [x] Run focused tests, full validation, and `git diff --check`.
