# ADR-0008: Apy Language And Runtime Ownership

## Status

Accepted

## Context

The beta IDE can run, lint, and format `.apy` through the bundled
`lughat-althuban` runtime. A full IDE needs parser-backed diagnostics, symbols,
completion, hover, rename, formatting, tests, and debugging. Those capabilities
cannot be fabricated reliably in the Qt shell alone.

## Decision

Treat `.apy` language intelligence as a shared IDE/runtime contract. The IDE
owns the Qt client, UI integration, cache/stale-state behavior, and user
experience. The runtime owns or exposes parser, diagnostics, formatting,
symbols, test discovery, and debug protocol data. If the runtime lacks a stable
API, define it before building UI features that depend on it.

## Consequences

Language features may require coordinated changes outside this repository. The
IDE must degrade gracefully when the language service is missing, slow, stale,
or incompatible.

## Verification

Golden `.apy` fixtures must cover runtime language responses. Qt tests must
cover IDE rendering and failure states for diagnostics, completion, hover,
rename, tests, and debugger behavior.
