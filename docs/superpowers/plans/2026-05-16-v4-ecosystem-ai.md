# V4 Ecosystem Remote AI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add extensions, remote development, AI assistance, notebooks, and collaboration without weakening user control or Arabic-first correctness.

**Architecture:** Add extension and AI capabilities only after command, workspace trust, language, debug, Git, task, and permission foundations exist. Isolate extension execution from the main process and require explicit user approval for AI file edits.

**Tech Stack:** C++17, Qt 6 Widgets/Test, isolated extension host process, remote transports, bundled/runtime language APIs, permission and audit fixtures.

---

## Tasks

- [ ] Define extension manifest, isolated host process, lifecycle, safe mode, API versioning, and contribution points for commands, menus, views, themes, snippets, languages, tasks, debuggers, and settings.
- [ ] Add extension permissions for file, process, terminal, network, secrets, and workspace access.
- [ ] Add remote SSH, WSL where useful, container workspaces, remote explorer, search, Git, terminal, tasks, language service, and debug transport.
- [ ] Add AI side panel, inline suggestions, explain/fix/refactor actions, and agent mode with plan, diff preview, approval, secret exclusion, and audit log.
- [ ] Add `.apy` notebooks or interactive documents with cells, output persistence, state view, and export.
- [ ] Add read-only review mode, comments/review workflow, and live collaboration only after a security model is tested.
- [ ] Test extension lifecycle/permissions/crashes, remote routes, AI context and secret exclusion, notebook execution/export, and Homelab isolation guarantees.
