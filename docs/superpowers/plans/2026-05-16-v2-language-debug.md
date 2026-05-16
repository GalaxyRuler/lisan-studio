# V2 Language Intelligence And Debug Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `.apy` feel like a real IDE language with diagnostics, navigation, refactoring, tests, and debugging.

**Architecture:** Define a runtime-backed `.apy` language-service contract before building UI features that depend on it. The IDE owns the Qt client and user experience; the runtime owns parser, diagnostic, formatting, symbol, test, and debug protocol data.

**Tech Stack:** C++17, Qt 6 Widgets/Test, bundled Python runtime, `lughat-althuban`, golden `.apy` fixtures.

---

## Tasks

- [ ] Define `.apy` language-service responses for diagnostics, symbols, completion, hover, signature help, references, rename, format, tests, and debug hooks.
- [ ] Add an IDE `LanguageServiceClient` with timeout, stale-state, and unavailable-service handling.
- [ ] Add diagnostics from unsaved buffers, completion popup, hover, signature help, go-to/peek definition, references, rename preview, outline, workspace symbols, semantic highlighting, code actions, and format document/selection.
- [ ] Add `.apy` test discovery, Test Explorer, run all/file/selected, rerun failed, and failure navigation.
- [ ] Add debug adapter boundary, breakpoints, stepping, variables, watch, call stack, debug console, restart, stop, and exception breaks.
- [ ] Add golden runtime fixtures and Qt UI tests for every language/debug feature.
