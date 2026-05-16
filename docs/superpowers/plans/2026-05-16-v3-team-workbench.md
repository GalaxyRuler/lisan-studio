# V3 Team Workbench Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Lisan Studio viable for shared professional `.apy` repositories.

**Architecture:** Build Git, workspaces, profiles, terminal profiles, and support tooling on top of the V1 command/state foundations and V2 language/task/debug services. Destructive workflows require confirmation and tests.

**Tech Stack:** C++17, Qt 6 Widgets/Test, Git CLI integration through explicit process arguments, CMake, PowerShell QA scripts.

---

## Tasks

- [ ] Add Git repository detection, branch/status indicator, Source Control view, staging, unstaging, commit, amend, fetch, pull, push, history, blame, and merge-conflict UI.
- [ ] Add `.lisan-workspace`, multi-root folders, per-folder settings/tasks/launch configs/language roots, and workspace trust.
- [ ] Add profiles for app development, teaching, minimal editor, and internal QA with import/export.
- [ ] Add multiple terminals, terminal profiles, environment activation, terminal search, and clickable file/URL links.
- [ ] Add update channel plan, rollback policy, crash/log bundle with secret redaction, offline installer notes, and enterprise install notes.
- [ ] Test with temporary Git repositories, workspace fixtures, terminal profile fixtures, and Homelab installed-app regression when producing release evidence.
