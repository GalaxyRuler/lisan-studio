# V1 Bracket Matching Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Highlight matching brackets and quotes in mixed Arabic/English `.apy` text without disturbing existing find highlights.

**Architecture:** Keep matching inside `EditorSurface`, using logical QTextDocument positions. Merge bracket-match extra selections with the existing find-match selections so both editor features can coexist.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- ASCII delimiter pairs: `()`, `[]`, `{}`.
- ASCII quotes: `'` and `"`.
- Arabic guillemets: `«»`.
- Cursor-adjacent matching using logical text positions.
- Test-only bracket selection count.
- Preservation of existing find highlight selections.

Excluded:

- Parser-aware string/comment skipping.
- Error squiggles for unmatched delimiters.
- Multi-cursor matching.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [ ] Write failing editor tests for Arabic/mixed delimiter matching.
- [ ] Write failing editor test that find highlights remain while bracket matching is active.
- [ ] Add bracket-match state and test accessor to `EditorSurface`.
- [ ] Merge find and bracket extra selections in one renderer.
- [ ] Update bracket matches on cursor movement and document edits.
- [ ] Run focused editor tests, full validation, and `git diff --check`.
