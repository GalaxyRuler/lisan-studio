# V1 Shortcut Editor Controls Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the shortcut settings page edit one selected command binding.

**Architecture:** The page remains simple: selecting a command populates a `QKeySequenceEdit`, and the apply button calls the already-tested `setShortcutOverrideForCommand` helper. Conflict handling stays in `ShortcutSettingsModel`.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Add key-sequence edit control and apply button.
- Populate the editor from the selected command row.
- Persist and apply the selected command override.
- Update row metadata/text after a successful change.

Excluded:

- Multi-command batch editing.
- Workspace-specific keymaps.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Add failing settings-dialog edit test.
- [ ] Add `QKeySequenceEdit` and apply button.
- [ ] Wire row selection and apply behavior.
- [ ] Run focused tests, full validation, and `git diff --check`.
