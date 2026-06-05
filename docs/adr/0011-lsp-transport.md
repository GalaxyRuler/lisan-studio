# 0011. LSP transport library

## Status

Accepted (2026-06-05) — use `lsp-framework` as the Qt-side LSP transport and model layer for V2.

## Context

V2 turns Lisan Studio from a capable Arabic-aware text editor into a real IDE. The LSP phase needs document sync, request/response routing, notifications, diagnostics, completion, hover, go-to-definition, references, rename, semantic tokens, workspace symbols, and cancellation.

The editor is a Qt 6 Windows desktop application. LSP communication will start with a stdio child process for `lughat-althuban-lsp`, then may grow to additional launch modes later. The transport must therefore fit the current QProcess-based runtime model without taking ownership of editor UI decisions.

The project already avoids bespoke protocol implementations where a proven library can cover the core protocol surface. LSP has enough framing, JSON-RPC, lifecycle, cancellation, and partial-result behavior that rolling the whole client by hand would make V2 slower and harder to validate.

## Decision

Vendor `lsp-framework` under `third_party/lsp-framework` and use it as the Qt-side LSP protocol foundation. The initial integration will expose a Lisan-owned wrapper, `LspClient`, that owns process launch, document lifecycle, request dispatch, timeout handling, and conversion into editor-facing models.

The public Lisan Studio code should not let third-party LSP types leak into broad UI surfaces. Editor and panel code talk to Lisan-owned DTOs and signals. This keeps the option open to replace the transport if the dependency becomes unsuitable.

## Rationale

- A working Qt 6 reference exists through `diegoiast/lsp-client-demo-qt`, reducing integration uncertainty.
- `lsp-framework` keeps protocol mechanics out of `EditorSurface`, which should stay focused on editing and rendering behavior.
- A library-backed transport lets V2 focus test effort on Lisan-specific behavior: Arabic source mapping, UI timing, command routing, diagnostics, and recovery.
- A Lisan-owned wrapper still gives us control over process policy, logging, crash handling, timeout budgets, and user-visible failure states.

## Alternatives considered

1. **Roll a custom JSON-RPC/LSP client over QProcess.** Rejected for the first V2 implementation because it would require hand-coding protocol framing, request correlation, lifecycle, cancellation, and message types before any user-visible IDE feature ships.
2. **Use Qt's internal `qtlanguageserver` module.** Rejected because it is not a supported public application API; depending on Qt Creator internals would raise maintenance risk.
3. **Use a generic JSON-RPC library such as jcon-cpp plus hand-written LSP models.** Rejected because it solves only the JSON-RPC layer and still leaves most LSP-specific behavior to the application.

## Consequences

- V2-C2 adds the first third-party submodule to the repository. GitHub Actions checkout and local setup docs must handle submodules explicitly.
- `LspClient` tests should use mock LSP server fixtures. They must not depend on a live `lughat-althuban-lsp` binary for protocol handshake and document-sync coverage.
- All user-visible LSP features should be implemented through Lisan-owned models so `lsp-framework` remains replaceable.
- If the dependency cannot support required V2 behavior, the fallback is a Lisan-owned JSON-RPC client over QProcess. That fallback is expected to cost additional slices and should not be started until integration evidence shows the library cannot meet the contract.

## Verification

The implementation slices gated by this ADR must prove:

- initialize/initialized handshake succeeds against a mock server;
- didOpen, didChange, didSave, and didClose are emitted with the expected document URIs and versions;
- request/response correlation handles success, error, timeout, and server-exit paths;
- UI code receives Lisan-owned DTOs rather than depending directly on transport-library types.
