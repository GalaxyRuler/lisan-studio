# V1 Run Configuration Store Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Persist named run configurations in project-owned workspace metadata.

**Architecture:** Add `RuntimeRunConfigurationStore` in `acs_core`. It reads and writes `.lisan-workspace/run-configurations.json`, returning an empty safe default when missing or invalid. Writes use `QSaveFile`; the store does not execute or trust workspace-provided commands.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Run configuration JSON path helper.
- Safe default for missing/invalid JSON.
- Save/load for ID, name, action, file path, working directory, and reload behavior.
- Pure service tests.

Excluded:

- Run configuration UI.
- Automatic workspace trust prompts.
- Terminal/task execution.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing run configuration store tests.
- [x] Add `RuntimeRunConfigurationStore` to `acs_core`.
- [x] Implement JSON load/save with atomic writes and safe fallback.
- [x] Run focused tests, full validation, and `git diff --check`.
