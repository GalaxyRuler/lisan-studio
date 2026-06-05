# 0014. Integrated terminal backend

## Status

Accepted (2026-06-05) - implement V2 terminal support with a Windows
ConPTY backend behind Lisan-owned terminal UI surfaces.

## Context

Phase F closes V2 by replacing the placeholder terminal command with real
shell execution. The existing code already has a terminal profile model,
workspace-trust gate, terminal link parsing, and a bottom-panel terminal
surface. What is missing is the execution backend and the UI binding.

Terminal execution is higher risk than normal runtime actions because shells
are interactive, long-running, and capable of arbitrary workspace mutation.
The V1/V2 trust model must remain the gate: untrusted workspaces can show the
terminal panel and explanation, but must not spawn a shell.

Windows terminal integration should use the platform pseudo-console API
rather than a plain `QProcess` pipe, because many shells and console programs
depend on terminal semantics for prompts, control sequences, and interactive
input.

## Decision

Implement a Lisan-owned `ConPtyBackend` using Windows ConPTY APIs:
`CreatePseudoConsole`, anonymous pipes, `STARTUPINFOEX`, and an asynchronous
read pump that emits UTF-8/console output into Qt.

The V2 UI will bind this backend into the existing terminal bottom panel with
a shell picker for PowerShell and cmd. QTermWidget remains the preferred
long-term terminal renderer if it proves reliable on Windows, but V2 will not
block public-readiness on vendoring a Linux-first widget that cannot be
verified in the current Windows toolchain. The renderer boundary stays narrow
so QTermWidget or another terminal widget can replace the text surface later.

## Rationale

- ConPTY is the native Windows terminal boundary and supports real interactive
  shells better than ordinary process pipes.
- Keeping the backend Lisan-owned lets tests cover spawn, write, read, and
  exit behavior without depending on a GUI terminal widget.
- The existing trust and terminal profile model already provide the right
  permission boundary.
- A text-backed terminal panel is less featureful than QTermWidget, but it can
  be validated locally and packaged now. Public-use readiness is better served
  by a proven, trust-gated shell than by an unverified renderer dependency.

## Alternatives considered

1. **QProcess-only terminal.** Rejected because it does not provide terminal
   semantics for interactive console applications.
2. **QTermWidget-first integration.** Deferred for V2 because the Windows
   build and ConPTY wiring risk is higher than the backend itself.
3. **Embed an external terminal emulator.** Rejected for V2 because it would
   add installer, focus, and trust-model complexity.
4. **Keep terminal placeholder.** Rejected because V2 completion requires a
   real IDE terminal path.

## Consequences

- Phase F must add Windows-specific backend code guarded in CMake and tests.
- GUI terminal rendering in V2 is intentionally basic: command input, output,
  shell picker, and trust gate first; richer ANSI rendering can follow.
- Public distribution still requires installed-app terminal QA in an isolated
  runner, not on the active desktop.

## Verification

Implementation slices gated by this ADR must prove:

- spawning `cmd.exe` through ConPTY succeeds;
- writing `echo hello\r\n` produces `hello` in the terminal output buffer;
- multi-line input/output works;
- process exit propagates to Qt;
- the UI refuses to start terminals in untrusted workspaces;
- the trusted UI path uses explicit profile program/argument lists, not shell
  string construction.
