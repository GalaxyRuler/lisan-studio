# V1 Output Transcript Links Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let output transcripts expose parsed file links from their rendered text.

**Architecture:** Reuse `TerminalLinkParser` from `OutputTranscript`, returning offsets that match the rendered transcript for future clickable output rows. This slice does not change output UI.

**Tech Stack:** C++17, Qt 6 Core/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Link extraction from rendered transcript text.
- Respect existing transcript filters.
- Reuse terminal link parser path/line/column semantics.

Excluded:

- Clickable output UI.
- Terminal process execution.
- Filesystem existence checks.
- GUI/MSI validation on active `WHITEDRAGON`.

## Tasks

- [x] Add failing output transcript link tests.
- [x] Add `OutputTranscript::links`.
- [x] Reuse `TerminalLinkParser` against rendered transcript text.
- [x] Run focused tests, full validation, and `git diff --check`.
