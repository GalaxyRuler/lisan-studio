# ADR-0009: Extension And AI Permission Model

## Status

Accepted

## Context

Extensions, remote development, and AI assistance are part of the full IDE end
state. They can also read files, run tools, use terminals, access network
resources, and expose private project context if added without boundaries.

## Decision

Extensions and AI features are late-stage capabilities gated by explicit
permissions, user-visible scopes, secret exclusion, diff preview for file edits,
approval before agentic changes, and auditability. The extension host must be
isolated from the main process. AI features must use the same command, file,
diagnostic, test, and isolated-runner boundaries exposed to other trusted integrations.

## Consequences

Do not add public marketplace, agent mode, or broad extension execution before
core editor, language, debug, Git, task, workspace trust, and permission
foundations exist.

## Verification

Tests must cover extension lifecycle, permission prompts, denied access, host
crashes, AI context selection, secret exclusion, diff generation, and approval
gates before release.
