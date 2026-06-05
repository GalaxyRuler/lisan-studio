# 0012. Apython LSP backend

## Status

Accepted (2026-06-05) — implement V2 language intelligence through a translation shim around `python-lsp-server` in the `lughat-althuban` repository.

## Context

Lisan Studio edits Arabic Python (`.apy`) source. Users need IDE behavior that understands both Arabic keywords/operators and Python runtime semantics: completion, hover, diagnostics, go-to-definition, references, rename, semantic highlighting, outline, and workspace symbols.

Python already has mature LSP implementations, especially `python-lsp-server`. Apython adds a source-language layer on top of Python: Arabic keywords, Arabic-facing syntax, operator conventions, and bidi editing concerns. PEP 3131 means Python tooling can already handle non-ASCII identifiers, but it does not understand apython keyword translation or source-position mapping by itself.

The language backend also belongs with the apython runtime. Lisan Studio should consume a language server contract; it should not own the compiler/transpiler semantics.

## Decision

Create `lughat-althuban-lsp` in the `lughat-althuban` repository as a stdio LSP server. It receives `.apy` documents, translates them to Python through the existing apython pipeline, queries `python-lsp-server`, and maps results back to `.apy` source positions and Arabic-facing symbols.

Lisan Studio launches this server as an external process and treats it as the source of LSP intelligence. The Qt application owns UI timing, rendering, command routing, and failure presentation; the apython repository owns source translation, Python analysis, and position mapping.

## Rationale

- Reusing `python-lsp-server` gives V2 mature Python analysis without building a parser, type analyzer, import resolver, and refactor engine from scratch.
- Keeping the shim in `lughat-althuban` puts language semantics next to the runtime and transpiler that already define them.
- A stdio server boundary lets Lisan Studio test the Qt client with mocks while the apython repo tests mapping accuracy independently.
- Position mapping is the hard V2 language problem. Isolating it in the backend gives it focused tests and keeps UI code from encoding translation rules.

## Alternatives considered

1. **Build a custom apython-aware LSP server from scratch.** Rejected for V2 because it requires parser, scope, type, import, completion, rename, and diagnostics logic before the IDE gains useful language features.
2. **Ask pylsp to analyze `.apy` source directly.** Rejected because pylsp understands Python syntax and PEP 3131 identifiers, not apython's full Arabic syntax and translation rules.
3. **Hybrid parse-in-apython plus custom pylsp integration for each feature.** Rejected as too complex for the first public V2 path. It may become useful later for precision improvements, but the initial contract should be one shimmed LSP server.

## Consequences

- V2-C1 is implemented in `lughat-althuban`, not this repository.
- Lisan Studio must handle missing, crashing, slow, or incompatible `lughat-althuban-lsp` gracefully.
- The backend must publish enough version and capability metadata for the Qt client to detect unsupported language-server versions.
- Mapping tests in the apython repo must cover ASCII identifiers, Arabic identifiers, mixed bidi lines, multi-line ranges, edits that shift translated positions, and completion/hover requests.
- Public release notes must distinguish UI LSP features from backend limitations when a result is missing or approximate.

## Verification

The implementation slices gated by this ADR must prove:

- `lughat-althuban-lsp` starts on stdio and completes LSP initialize/initialized;
- `.apy` source translates to Python without losing a source map for every token range needed by completion and hover;
- completion and hover positions round-trip for ASCII-only, Arabic-only, mixed-direction, and multi-line fixtures;
- backend failures surface as recoverable Lisan Studio status messages, not editor crashes.
