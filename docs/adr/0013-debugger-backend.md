# 0013. Debugger backend

## Status

Accepted (2026-06-05) - implement V2 debugger support with `debugpy`
and a Lisan-owned Qt Debug Adapter Protocol client.

## Context

Phase E turns Lisan Studio from an editor with language navigation into an
interactive IDE. Users need breakpoints, continue, step over, step into,
step out, locals, watch expressions, and call stack navigation for `.apy`
programs running through the bundled apython runtime.

The runtime is already owned by `lughat-althuban`, and Lisan Studio already
launches runtime processes through Qt `QProcess` boundaries. Debugging
should keep that ownership split: the apython runtime owns Python execution
and translation behavior; the Qt app owns workbench commands, panels,
breakpoint visuals, and user-facing state.

Python's Debug Adapter Protocol ecosystem is mature through `debugpy`.
Building a custom Python debugger would require tracing, frame management,
variable serialization, stepping semantics, and source mapping before users
see a reliable breakpoint.

## Decision

Use `debugpy` as the Python debug backend and add a Lisan-owned Qt DAP
client in this repository. The client starts from a stdio-oriented transport
for tests and process lifecycle consistency, then can add socket launch modes
only if debugpy integration evidence requires it.

The implementation will add `DapClient` and, if useful during E1, extract the
shared Content-Length JSON-RPC framing/request-correlation code currently in
`LspClient` into a small reusable transport. DAP response/event DTOs exposed
to MainWindow, DebugController, and bottom panels must be Lisan-owned types,
not raw third-party protocol objects.

Debug commands belong in the existing command registry. E2 will free F5 for
debug continue by moving ordinary run to Ctrl+F5, while F10/F11/Shift+F11
belong to step-over/step-into/step-out. Commands must be enabled according
to debug session state rather than always hijacking editor shortcuts.

## Rationale

- `debugpy` is the standard Python DAP backend and matches the bundled Python
  runtime model.
- A custom Qt DAP client keeps UI state, command enablement, and panel
  rendering under Lisan Studio ownership.
- Reusing or extracting the existing JSON-RPC framing reduces duplicate
  protocol code without making the LSP and debugger clients depend on each
  other's feature models.
- DAP is stateful and event-heavy, so the app needs explicit tests for
  initialize, launch, breakpoint, stopped, continued, stackTrace, scopes, and
  variables behavior before user-facing debugger controls can be trusted.

## Alternatives considered

1. **Build a custom Python tracing debugger.** Rejected for V2 because it
   duplicates a mature ecosystem and would likely underperform debugpy on
   stepping, frames, variables, and future Python compatibility.
2. **Embed a full IDE/debugger framework.** Rejected because it would pull
   broad workbench assumptions into a focused Qt app and make Arabic-first
   UI control harder.
3. **Shell out to a terminal-only debugger.** Rejected because Phase E's goal
   is workbench-native breakpoints, stepping, locals/watch, and call-stack
   navigation.
4. **Make the apython repository own the UI debugger.** Rejected because
   apython owns language/runtime semantics, while Lisan Studio owns the IDE
   surfaces and user workflow.

## Consequences

- Phase E depends on `debugpy` being present in the packaged runtime or
  installed into the bundled Python environment during packaging.
- Debugging a translated `.apy` file may require source-map support from
  apython. Until that support is proven, E2/E3 must report limitations
  honestly and avoid implying exact stepping where mapping is approximate.
- Debugger tests should use a mock DAP server for protocol handshake and
  state-machine coverage before any live debugpy smoke.
- GUI/MSI/installed-app debugger validation remains in the isolated
  `LisanStudio-QA` Homelab lane, not on active WHITEDRAGON.

## Verification

Implementation slices gated by this ADR must prove:

- initialize and launch requests round-trip against a mock DAP server;
- debugpy process launch failures surface as recoverable status/problem
  messages, not workbench crashes;
- breakpoints are serialized from editor line state into DAP requests;
- stopped, continued, stackTrace, scopes, and variables responses update
  Lisan-owned models before they touch UI widgets;
- command shortcuts and enabled states are tested through the command
  registry.
