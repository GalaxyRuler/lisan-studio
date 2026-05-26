# Lisan Studio V2 Roadmap

> Status: planning, composed 2026-05-26 against HEAD `fd831e0`
> (post-V1.5 close). V1.5 archived at `docs/ROADMAP-V1.5-archive.md`.
> Release cadence: per-theme. Each theme is a multi-slice arc; the
> final slice in a theme cuts a new `v0.x.0-beta` release covering
> that theme's user-visible changes.

## Scope

V2 is **language intelligence + debugger + terminal + multi-cursor
completion**. The IDE crosses from "good text editor with multi-cursor"
into "real IDE" via LSP. Each theme is independently committable and
ships its own release cut.

V2 is NOT:
- Git source-control UI. V3.
- Multi-root workspaces, profiles, extensions, remote SSH, AI side
  panel. V3+.
- Cross-platform (Linux/macOS) or ARM64 Windows support. Windows-x64
  only through V2.
- Apython source analysis from scratch — V2 LSP uses a translation
  shim around `python-lsp-server` (pylsp). Direct Arabic-source
  analysis is V3+.

## Architecture decisions to lock before V2 slices dispatch

Four ADRs precede the implementation slices. Each is a docs-only
slice in this roadmap.

| ADR | Decision | Recommendation |
|---|---|---|
| ADR-0011 | LSP transport library | `lsp-framework` via the diegoiast/lsp-client-demo-qt reference. Qt's `qtlanguageserver` is internal-only (no public API per the Qt wiki). |
| ADR-0012 | Apython LSP backend | Translation shim around pylsp. Lives in the apython repo (`lughat-althuban`). PEP 3131 lets pylsp handle non-ASCII identifiers natively, but apython has more than identifier translation (keyword translation, operator overloading) — translation shim handles the full surface. |
| ADR-0013 | Debugger backend | `debugpy` + custom Qt DAP client. Reuses the JSON-RPC transport primitives from theme C. PyCharm 2026.1 adopted debugpy as default. |
| ADR-0014 | Terminal widget | `QTermWidget` + Windows ConPTY adapter. Defer Contour to V3 unless QTermWidget proves insufficient on Windows bidi. |

## Themes

### Theme A — Multi-cursor V2 extensions (3 slices)

Finishes what the V1.5 keystone deliberately deferred. Each slice
extends the existing slice-9 `EditorSurface::secondaryCursors`
machinery without altering its core invariants (atomic edit blocks,
cap enforcement, descending-position dispatch).

- **V2-A1 — Multi-cursor + Ctrl+Z/Y dispatch.** Wrap undo/redo so a
  single Ctrl+Z reverses the most recent multi-cursor edit across all
  cursors. The existing `beginEditBlock`/`endEditBlock` pair from
  slice 9 already groups the edit; this slice verifies the undo stack
  rolls all cursors in lockstep and adds a regression test.
- **V2-A2 — Multi-cursor + Tab indent dispatch.** Tab and Shift+Tab
  apply smart indent/dedent across all cursors. Each cursor's line is
  indented independently; the operation is one atomic edit block.
- **V2-A3 — Multi-cursor + IME composition.** Lift the slice-9 IME
  rejection. Composition cluster (input method preedit string +
  commit) dispatches across all cursors with bidi/composition-aware
  handling. Hardest theme A slice; Kate has open bugs in this area
  (kate-editor.org/post/2022/2022-03-10-ktexteditor-multicursor).
  Acceptance: Arabic IME composition typed once produces the composed
  text at every cursor.

### Theme B — Editor polish foundations (3 slices)

Independent of LSP/debugger/terminal — ships anytime.

- **V2-B1 — Incremental autosave for untitled drafts.** Surfaced by
  V1.5 slice 7 audit. Current persistence on dirty/clean transitions
  only. Design: 5-second debounce timer firing
  `saveWorkbenchSession()` while any untitled buffer is dirty.
  Recovery prompt on next launch if drafts persisted from a
  non-graceful exit.
- **V2-B2 — MainWindow truncation-test seam.** The slice-6
  `mainWindowSurfacesSearchTruncationInStatusBar` test currently
  creates 5001 files because MainWindow hard-binds to
  `SearchService::MaxScannedFiles`. Add a test-only seam (e.g.,
  `MainWindow::setSearchScanCapForTest`) and drop the fixture to ~20
  files.
- **V2-B3 — Command-id style ADR + migration.** 47 commands across
  three id-style tiers. Pick one (recommend dotted camelCase —
  already used by multi-cursor commands). Migration must persist
  user shortcut JSON across renames.

### Theme C — LSP foundation (4 slices)

The flagship V2 theme.

- **V2-C1 — ADR-0011 + ADR-0012.** Lock the LSP transport library
  (`lsp-framework`) and the apython backend strategy (translation
  shim around pylsp). Docs-only.
- **V2-C2 — Apython translation-shim LSP server (Python side).**
  Lives in `lughat-althuban`, not Lisan Studio. Wraps pylsp:
  translates Arabic source to standard Python for analysis,
  translates positions and identifiers back. Position mapping is the
  hard part.
- **V2-C3 — LSP client integration (Qt side).** Vendor or submodule
  `lsp-framework`. Add an `LspClient` class managing the
  QProcess-spawned server, JSON-RPC pump, document sync. Initial
  handshake (initialize/initialized/didOpen). No user-visible
  features yet.
- **V2-C4 — Completion + hover.** First user-visible LSP features.
  Completion popup in the editor, hover tooltip on identifiers.
  Establishes the LSP→UI binding pattern subsequent slices follow.
  Cuts `v0.3.0-beta`.

### Theme D — LSP advanced (3 slices)

- **V2-D1 — Go-to-definition + find-references.** Navigation.
- **V2-D2 — Rename refactor.** Multi-edit refactor — natural
  synergy with the slice-9 multi-cursor model. The killer feature
  on top of the keystone.
- **V2-D3 — Semantic highlighting + outline + workspace symbols.**
  Diminishing user-visible returns but rounds out the LSP feature
  set. Cuts `v0.4.0-beta`.

### Theme E — Debugger (4 slices)

- **V2-E1 — ADR-0013.** debugpy + custom Qt DAP client architecture
  lock. Docs-only.
- **V2-E2 — DAP transport + initialize + launch.** Spawn debugpy
  server, attach to child apython process, verify the initialize
  handshake.
- **V2-E3 — Breakpoints + step + continue.** First user-visible
  debugger features. UI: breakpoint margin in the line-number area,
  step toolbar.
- **V2-E4 — Locals + watch + call stack.** Variable inspection.
  New Variables panel widget in the bottom-panel infrastructure
  (EditorTabsController/BottomPanelController). Cuts `v0.5.0-beta`.

### Theme F — Terminal (3 slices)

Highest architectural risk — pushed last in V2 unless reprioritized.

- **V2-F1 — ADR-0014.** QTermWidget + ConPTY architecture lock.
  Docs-only.
- **V2-F2 — ConPTY backend.** Win32 ConPTY API integration in C++;
  spawn cmd.exe/PowerShell/WSL bash as backing process; pty I/O
  pump.
- **V2-F3 — QTermWidget integration + multi-shell picker.** Render
  PTY output via QTermWidget; UI to pick shell. Replaces the
  `terminal.openPowerShell` placeholder that's lived in the command
  palette since V1. Cuts `v0.6.0-beta` and closes V2.

## Cross-cutting policy

- **Per-theme release cadence.** Each theme's final slice cuts a new
  minor-version `v0.x.0-beta` release. Within a theme, intermediate
  slices land via push without a release.
- **V2 completion criteria.** V2 closes when themes A-F are done OR
  explicitly deferred. The milestone close cuts `v1.0.0-rc1` if all
  themes ship; otherwise the last `v0.x.0-beta` is the milestone-
  close release and the deferred themes carry to V3.
- **ADR-first within each theme.** Themes C, E, F each have an ADR
  slice that must land before implementation slices in that theme
  dispatch.

## Risk register

| Risk | Detection | Mitigation |
|---|---|---|
| lsp-framework integration is harder than the demo suggests | V2-C3 dispatch | Fall back to rolling JSON-RPC over QProcess directly using existing Qt primitives. Cost: ~2 extra slices. |
| Apython translation shim position mapping has edge cases | V2-C2 testing | Iterate; document known-bad inputs in KNOWN_ISSUES.md; defer to V3 if blocking. |
| debugpy DAP version compat with current debugpy releases | V2-E2 dispatch | Pin a known-good debugpy version in apython's bundled runtime. |
| QTermWidget Windows bidi rendering is poor for Arabic terminal output | V2-F3 testing | Switch to Contour. ~3 extra slices to integrate. |
| Multi-cursor IME composition is unsolvable on current Qt 6 | V2-A3 prototyping | Document as an open Qt platform limitation; ship single-cursor IME with degraded multi-cursor behavior. |

## Non-goals (explicitly NOT V2)

| Feature | Why deferred |
|---|---|
| Direct Arabic-source LSP analysis (no translation shim) | Requires apython parser and type inference from scratch; V3+ |
| Git source-control UI | V3 |
| Multi-root workspaces | V3 |
| Profiles, extensions, remote SSH, AI side panel | V3+ |
| Cross-platform port | Not planned in any milestone |
| ARM64 Windows | Not planned in any milestone |

## How to use this roadmap

- **Picking work:** start with Theme A (multi-cursor completion;
  smallest surface) OR Theme C ADR (largest upside; needs prep).
- **Sequencing:** themes are independent. Recommended order is
  A → B → C → D → E → F, but a user pressing for LSP can move C
  earlier.
- **Updating this doc:** when a slice lands, mark it done with
  "landed (see git log)". When a theme closes, retitle the theme
  heading to include "(closed)". When V2 closes, retitle this file
  to `docs/ROADMAP-V2-archive.md` and create `docs/ROADMAP-V3.md`.

---

*Composed 2026-05-26 by Claude (strategy/audit) after a grilled
research pass on Qt 6 LSP libraries, debugpy, ConPTY widgets, and
Kate multi-cursor reference implementations. Slice prompts will be
drafted per-slice when work begins.*
