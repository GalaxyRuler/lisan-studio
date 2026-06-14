# Lisan Studio V2 Roadmap

> Status: planning, composed 2026-05-27 against HEAD `fc8787a` (v0.2.0-beta, V1.5 milestone closed).
> V1.5 archived at [docs/ROADMAP-V1.5-archive.md](ROADMAP-V1.5-archive.md).
> Release cadence: themes A and B per-slice; themes C, D, E, F per-theme cut. Each theme's final slice cuts a new `v0.x.0-beta` release.

## Mission

V2 crosses Lisan Studio from "good multi-cursor text editor" to "real IDE." The flagship is LSP — completion, hover, go-to-definition, references, rename, semantic highlighting — backed by an apython-aware translation shim around pylsp. The debugger and terminal complete the IDE essentials. Multi-cursor V2 extensions (IME, Ctrl+Z/Y, Tab indent) finish what the V1.5 keystone deliberately deferred.

## Scope

**V2 IS:**
- Multi-cursor extensions: IME composition dispatch, Ctrl+Z/Y across cursors, Tab indent across cursors
- Editor polish: incremental autosave for untitled drafts, MainWindow truncation-test seam, command-id style ADR + migration
- LSP foundation: lsp-framework client, apython translation shim, document sync, completion, hover
- LSP advanced: go-to-definition, find-references, rename, semantic highlighting, outline, workspace symbols
- Debugger: debugpy DAP client, breakpoints, step, locals, watch, call stack
- Terminal: Lisan-owned Windows terminal backend, text-backed terminal UI, multi-shell picker

**V2 IS NOT:**
- Git source-control UI (V3)
- Multi-root workspaces (V3)
- Profiles, extensions, remote SSH, AI side panel (V3+)
- Cross-platform port — Windows-x64 only through V2
- ARM64 Windows
- Direct Arabic-source LSP analysis without a translation shim (V3+; V2 uses pylsp wrapping)

## Architecture decisions

Four ADRs precede the implementation slices in their respective themes. Each ADR is a docs-only slice and must land before its theme's implementation slices dispatch.

### ADR-0011 — LSP transport library

| Option | Pros | Cons |
|---|---|---|
| **`lsp-framework` via diegoiast/lsp-client-demo-qt** ✓ | Working Qt 6 reference exists; MIT-licensed; isolated dependency; small surface | Third-party; vendor or submodule |
| Qt's `qtlanguageserver` module | Official Qt module | **No public API** per Qt wiki — internal to Qt Creator only |
| Roll-your-own with QJsonRpcProtocol + QProcess | Full control | LSP spec is large; reinventing the wheel |
| jcon-cpp + manual LSP layer | Mature JSON-RPC | Still requires hand-coded LSP message types |

**Decision: lsp-framework.** Submodule under `third_party/lsp-framework`. Reference integration: <https://github.com/diegoiast/lsp-client-demo-qt>.

### ADR-0012 — Apython LSP backend

| Option | Mechanism | Tradeoff |
|---|---|---|
| **Translation shim around pylsp** ✓ | Receive Arabic source → transpile to Python → query python-lsp-server → translate positions/identifiers back | Reuses pylsp's mature analysis; position mapping is the hard part |
| Custom apython-aware LSP from scratch | Tokenize/parse/analyze Arabic source directly | Hard — parser, type inference, scope analysis from zero |
| Hybrid: parse-in-apython, analyze-in-pylsp-via-translation | Combine | Most code; most complex contract |

**Decision: translation shim.** Lives in the apython repo (`lughat-althuban`), not Lisan Studio. PEP 3131 lets pylsp accept non-ASCII identifiers natively, but apython has keyword translation + operator overloading on top — shim handles the full surface.

### ADR-0013 — Debugger backend

| Option | Tradeoff |
|---|---|
| **debugpy + custom Qt DAP client** ✓ | debugpy is universal (PyCharm 2026.1 adopted as default); client work is moderate; reuse JSON-RPC primitives from theme C |
| debugpy + tomlin7/debug-adapter-client (Python) + IPC | Extra hop; less code; less performant |
| Embed pdb directly | Tightest integration; no DAP benefit (no future debugger swaps) |

**Decision: debugpy + custom Qt DAP client.**

### ADR-0014 — Terminal widget

| Option | Tradeoff |
|---|---|
| **Lisan-owned terminal backend + text-backed UI** ✓ | Validated in the current Windows/Qt toolchain; keeps backend and renderer replaceable |
| Contour embedded | Modern; better bidi; bigger dependency footprint |
| QTermWidget + ConPTY | Established Qt widget and native Windows terminal direction; command input/rendering still needs proof in this toolchain |

**Decision: Lisan-owned terminal backend + text-backed UI for V2.** Keep QTermWidget, ConPTY, and Contour as upgrade candidates after Windows input/rendering proof.

## Phase structure & dependency graph

```
Phase 0 — Architecture decisions (ADR-0011..0014)
   │
   ├──> Phase A — Multi-cursor V2 extensions (3 slices)  ── independent
   │
   ├──> Phase B — Editor polish foundations (3 slices)   ── independent
   │
   └──> Phase C — LSP foundation (4 slices)
           │
           └──> Phase D — LSP advanced (3 slices)
                   │
                   └──> Phase E — Debugger (4 slices)
                           │
                           └──> Phase F — Terminal (3 slices)
```

Phases A and B are parallel-safe and independent of C/D/E/F. Phases C → D → E → F are sequential because each builds on the previous: D needs C's LSP client; E reuses C's JSON-RPC primitives; F is the last because of its architectural surface size, not because it depends on E.

Recommended user-facing sequencing: A → B → C → D → E → F. A user pressing for LSP first can move C earlier and defer A/B; the dependency graph permits it.

## Phase A — Multi-cursor V2 extensions

Finishes what the V1.5 keystone deliberately deferred. Each slice extends the existing slice-9 `EditorSurface::secondaryCursors` machinery without altering core invariants (atomic edit blocks, cap enforcement, descending-position dispatch).

### V2-A1 — Multi-cursor Ctrl+Z/Y dispatch

**Status:** planned
**Target release:** v0.2.1-beta (per-slice) or end-of-phase-A bundle
**Dependencies:** V1.5 slice 9 multi-cursor model (shipped)
**Blocks:** nothing in V2

**Change.** Extend `keyPressEvent` to handle Ctrl+Z / Ctrl+Y when secondary cursors exist. The slice-9 `beginEditBlock`/`endEditBlock` pair already groups each multi-cursor edit at the QTextDocument level; the undo stack should already roll all cursors in lockstep. This slice VERIFIES that and adds regression tests pinning the invariant.

**Acceptance.**
1. New test `multiCursorUndoRevertsAllCursorEditsInOneStep`: type 3 X's via 3 cursors → 1 undo removes all 3 → 1 redo restores all 3.
2. New test `multiCursorUndoRedoStormPreservesPositionsAndDirtyState`: 50 iterations of multi-cursor edit + undo + redo with assertion that all cursor positions and selections match the pre-edit state at each boundary.
3. `validate.ps1` still 10/10. Existing 11 perf budgets unchanged.

**Files.** `src/EditorSurface.cpp` (~30 lines), `tests/TestEditorSurface.cpp` (+2 cases).

### V2-A2 — Multi-cursor Tab indent dispatch

**Status:** planned
**Target release:** v0.2.1-beta
**Dependencies:** V2-A1 (shared keyPressEvent area; sequence reduces conflict risk)
**Blocks:** nothing

**Change.** Tab and Shift+Tab apply smart indent/dedent across all cursors. Each cursor's line is indented independently; the operation is one atomic edit block (single undo step).

**Acceptance.**
1. New test `multiCursorTabIndentsEachCursorLineByOneIndent`.
2. New test `multiCursorShiftTabDedentsEachCursorLineByOneIndent`.
3. New test `multiCursorIndentIsSingleUndoStep`.
4. `validate.ps1` still 10/10.

**Files.** `src/EditorSurface.cpp` (~50 lines), `tests/TestEditorSurface.cpp` (+3 cases).

### V2-A3 — Multi-cursor IME composition (HARD)

**Status:** planned
**Target release:** v0.2.1-beta (closes phase A and cuts the bundled release)
**Dependencies:** V2-A1, V2-A2 (keyPressEvent surface stabilized first)
**Blocks:** nothing in V2

**Change.** Lift the slice-9 `inputMethodEvent` rejection. Composition cluster (input method preedit + commit) dispatches across all cursors with bidi/composition-aware handling. Kate has open IME bugs in this area; expect this to be phase A's hardest slice. Acceptance focuses on Arabic IME behavior since that's the primary target audience.

**Acceptance.**
1. New test `arabicImeCompositionProducesIdenticalCommittedTextAtEveryCursor`: Arabic composition committed once produces the composed text at every cursor.
2. New test `imeCompositionWithSecondaryCursorsPreservesSingleUndoStep`.
3. `validate.ps1` still 10/10.
4. The slice-9 `multiCursorImeRejected` signal is removed from the API; the `inputMethodEvent` override changes from refuse to dispatch.

**Files.** `src/EditorSurface.{h,cpp}` (~80 lines), `tests/TestEditorSurface.cpp` (+2 cases).

**Risk note.** If after prototyping the composition cluster dispatch turns out infeasible on current Qt 6, ship a slice variant that keeps refusal as the default but exposes an opt-in mode (Settings → "Experimental: multi-cursor + IME"). Document in KNOWN_ISSUES.md.

## Phase B — Editor polish foundations

Independent of LSP/debugger/terminal. Ships anytime; can interleave with phase A.

### V2-B1 — Incremental autosave for untitled drafts

**Status:** planned
**Target release:** v0.2.2-beta (per-slice or bundled with B2/B3)
**Dependencies:** none (V1.5 slice 7 audit surfaced this; current state is dirty-clean transitions only)
**Blocks:** nothing

**Change.** Current `saveWorkbenchSession()` fires only on transitions (slice-7 audit finding). Add a 5-second debounce timer that fires `saveWorkbenchSession()` while any untitled buffer is dirty. On next launch, if drafts persisted from a non-graceful exit (detected by a write-on-shutdown sentinel cleared at launch and set at orderly shutdown), prompt user to recover or discard.

**Acceptance.**
1. New test `dirtyUntitledBufferAutosavesEveryFiveSecondsOfContinuousEditing`.
2. New test `gracefulShutdownClearsTheNonOrderlySentinel`.
3. New test `forceKillSimulationPersistsDraftsAndPromptsOnRelaunch`.
4. KNOWN_ISSUES.md bullet about transition-only persistence updated (now resolved or scoped down).
5. `validate.ps1` 10/10.

**Files.** `src/MainWindow.{h,cpp}` (~80 lines for the timer + sentinel + prompt), `src/SettingsStore.cpp` (~20 lines for sentinel store/load), `tests/TestUntitledDraftRecovery.cpp` (+3 cases — the existing slice-8 binary gains coverage).

### V2-B2 — MainWindow truncation-test seam

**Status:** planned
**Target release:** v0.2.2-beta
**Dependencies:** none
**Blocks:** nothing

**Change.** The V1.5 slice-6 `mainWindowSurfacesSearchTruncationInStatusBar` test creates 5001 files because MainWindow hard-binds to `SearchService::MaxScannedFiles`. Add a test-only seam: a `MainWindow::setSearchScanCapForTest(int)` setter, OR a constructor param defaulting to `SearchService::MaxScannedFiles`. Drop the fixture to 20 files.

**Acceptance.**
1. Test fixture drops from 5001 files to 20 files.
2. Test elapsed time drops by >10×.
3. `validate.ps1` 10/10.

**Files.** `src/MainWindow.{h,cpp}` (~10 lines), `tests/TestMainWindow.cpp` (fixture change).

### V2-B3 — Command-id style ADR + migration

**Status:** planned
**Target release:** v0.2.2-beta (closes phase B)
**Dependencies:** none
**Blocks:** any future LSP work that introduces commands (theme C and beyond) ideally happens after the style is locked

**Change.** 47 commands across three id-style tiers (V1: 42 + multi-cursor: 5). Multi-cursor commands followed dotted camelCase; not enforced repo-wide. Two artifacts:
- ADR `docs/adr/0015-command-id-style.md`: lock dotted camelCase (`category.action`) repo-wide.
- Migration in code: rename non-canonical ids. User shortcut JSON must migrate seamlessly via a `shortcuts/idMigrations` table keyed by old-id → new-id.

**Acceptance.**
1. ADR-0015 committed.
2. All 47 commands' ids match `^[a-z]+(\.[a-z][a-zA-Z0-9]*)+$`.
3. User shortcut JSON with an old id is migrated to the new id at launch with the user's binding preserved.
4. New test `legacyCommandIdInShortcutJsonMigratesToCanonicalForm`.
5. `validate.ps1` 10/10.

**Files.** `docs/adr/0015-command-id-style.md` (new), `src/MainWindow.cpp` (~50 lines of id renames + migration call), `src/SettingsStore.cpp` (~40 lines for migration table), `tests/TestMainWindow.cpp` (+1 case).

## Phase C — LSP foundation

The flagship V2 theme. ADR slice first.

### V2-C0 — ADR-0011 + ADR-0012 ship as one ADR slice

**Status:** planned
**Target release:** none (docs-only)
**Dependencies:** none
**Blocks:** V2-C1, V2-C2, V2-C3, V2-C4 (the rest of theme C)

**Change.** Commit `docs/adr/0011-lsp-transport.md` (lsp-framework) and `docs/adr/0012-apython-lsp-backend.md` (translation shim around pylsp). Both with full context/decision/consequences sections matching ADR-0010's format.

**Acceptance.** Two ADRs committed. Single PR.

**Files.** `docs/adr/0011-lsp-transport.md`, `docs/adr/0012-apython-lsp-backend.md`.

### V2-C1 — Apython translation-shim LSP server (Python side)

**Status:** planned
**Target release:** none (this slice lives in `lughat-althuban`, not Lisan Studio repo — coordinated commit pair)
**Dependencies:** V2-C0 (ADR-0012 lock)
**Blocks:** V2-C2 (the Qt client needs a backend to talk to)

**Change.** In the apython repo, wrap pylsp with a translation shim. Receive Arabic source, transpile to Python via the existing apython pipeline, query pylsp, translate positions and identifiers back. Lives in `lughat-althuban` (apython repo), not Lisan Studio.

**Acceptance.**
1. Standalone command `lughat-althuban-lsp` starts a pylsp wrapper on stdio.
2. Position mapping correct for: ASCII-only identifiers (degenerate case), pure Arabic identifiers, mixed-direction lines with embedded Arabic, multi-line completion requests.
3. Tests in the apython repo covering position translation.
4. Released as a new apython point version, pinned in Lisan Studio's `msi-tests.yml` cross-repo checkout.

**Files.** In `lughat-althuban` repo. Cross-repo coordination required.

### V2-C2 — LSP client integration in EditorSurface (Qt side)

**Status:** planned
**Target release:** none (no user-visible feature yet)
**Dependencies:** V2-C0 (ADR-0011), V2-C1 (working backend to talk to)
**Blocks:** V2-C3

**Change.** Vendor `lsp-framework` as a git submodule under `third_party/lsp-framework`. Add `src/LspClient.{h,cpp}` managing the QProcess-spawned `lughat-althuban-lsp` server, JSON-RPC pump, document sync (didOpen, didChange, didSave, didClose). Implement initialize/initialized handshake. No user-visible features yet — this is foundation only.

**Acceptance.**
1. New `acs_lsp_client_tests` binary (test #11 in ctest).
2. Tests verify initialize handshake, didOpen, didChange round-trips.
3. Mock LSP server fixture for tests so we don't depend on a live `lughat-althuban-lsp` binary.
4. `validate.ps1` 11/11.

**Files.** `src/LspClient.{h,cpp}` (~400 lines), `src/MainWindow.cpp` (wire up — ~30 lines), `tests/TestLspClient.cpp` (new, ~300 lines), `CMakeLists.txt` (new binary + submodule), `.gitmodules` (new — first submodule in this repo).

### V2-C3 — Completion + hover (first user-visible LSP features)

**Status:** planned
**Target release:** v0.3.0-beta (closes phase C; first LSP-driven feature on screen)
**Dependencies:** V2-C2
**Blocks:** V2-D1 (D themes build on the established LSP→UI binding pattern)

**Change.** Wire LSP textDocument/completion and textDocument/hover into EditorSurface. Completion popup with debounce. Hover tooltip on identifier dwell. Establishes the LSP→UI binding pattern subsequent slices follow.

**Acceptance.**
1. New tests for completion popup model, hover-tooltip surface.
2. Manual QA: open `.apy` file, type an identifier, see completion; hover an identifier, see signature.
3. Performance: completion popup ≤ 200 ms p95. Hover ≤ 100 ms p95.
4. `validate.ps1` 11/11.
5. **v0.3.0-beta cut** — first minor-version release after v0.2.x.

**Files.** `src/EditorSurface.{h,cpp}` (~250 lines for completion model + hover widget), `tests/TestEditorSurface.cpp` (+5 cases). Release-cut slice follows in a separate commit (same pattern as V1.5 slice 7 / 10 / C3).

## Phase D — LSP advanced

### V2-D1 — Go-to-definition + find-references

**Status:** planned
**Target release:** v0.3.1-beta
**Dependencies:** V2-C3 (the LSP→UI binding pattern in place)
**Blocks:** V2-D2

**Change.** Wire textDocument/definition and textDocument/references. Ctrl+Click on identifier → go-to-definition. Shift+F12 → find references with a results panel (reuse the BottomPanelController surface).

**Acceptance.**
1. Tests for go-to-definition request/response round-trip.
2. References panel populated correctly.
3. Manual QA: Ctrl+Click identifier jumps to definition; Shift+F12 lists references.
4. `validate.ps1` 11/11.

**Files.** `src/EditorSurface.{h,cpp}`, `src/MainWindow.cpp` (Shift+F12 binding), new ReferencesPanel.

### V2-D2 — Rename refactor

**Status:** planned
**Target release:** v0.3.1-beta or v0.3.2-beta
**Dependencies:** V2-D1
**Blocks:** V2-D3

**Change.** Wire textDocument/rename. F2 on identifier opens inline rename dialog. Multi-edit refactor applies across all matches with natural synergy to the V1.5 multi-cursor model (every rename location becomes a synchronized cursor for preview).

**Acceptance.**
1. F2 → rename dialog → all occurrences updated.
2. Multi-file rename works (preview panel shows all files affected).
3. Atomic rollback if any file write fails.
4. `validate.ps1` 11/11.

### V2-D3 — Semantic highlighting + outline + workspace symbols

**Status:** planned
**Target release:** v0.3.2-beta (closes phase D)
**Dependencies:** V2-D2
**Blocks:** V2-E0

**Change.** textDocument/semanticTokens for syntax highlighting that replaces the current ApyHighlighter for known identifiers. textDocument/documentSymbol for outline panel. workspace/symbol for "Go to symbol in workspace" (Ctrl+T).

**Acceptance.**
1. Semantic tokens layered on top of ApyHighlighter (LSP wins; ApyHighlighter is the fallback).
2. Outline panel renders document symbols.
3. Ctrl+T workspace-symbol picker functional.
4. `validate.ps1` 11/11.
5. **v0.3.2-beta cut** closing phase D.

## Phase E — Debugger

### V2-E0 — ADR-0013

**Status:** planned
**Target release:** none (docs)
**Dependencies:** none
**Blocks:** V2-E1

**Change.** Commit `docs/adr/0013-debugger-backend.md` locking debugpy + custom Qt DAP client.

### V2-E1 — DAP client transport + initialize + launch

**Status:** planned
**Target release:** none (foundation, no user-visible)
**Dependencies:** V2-E0, V2-C2 (reuse JSON-RPC primitives from LSP client)
**Blocks:** V2-E2

**Change.** Add `src/DapClient.{h,cpp}` reusing the LspClient's JSON-RPC primitives (refactor LspClient.cpp's protocol layer into a shared `src/JsonRpcTransport.{h,cpp}` if not already). Spawn debugpy as a child of the apython runtime; verify initialize + launch handshake.

**Acceptance.** Tests for initialize/launch round-trip. `validate.ps1` 12/12.

### V2-E2 — Breakpoints + step + continue

**Status:** planned
**Target release:** v0.4.0-beta
**Dependencies:** V2-E1
**Blocks:** V2-E3

**Change.** Breakpoint margin in line-number area (click to toggle). Toolbar: Step Over (F10), Step Into (F11), Step Out (Shift+F11), Continue (F5 — currently bound to Run; rebind Run to Ctrl+F5 to free F5 for debug). New DebugController coordinating between EditorSurface (breakpoint visuals) and DapClient (protocol).

**Acceptance.**
1. Set breakpoint → run → debugger pauses → resume.
2. Step Over/Into/Out work on the bundled apython runtime.
3. F5 / F10 / F11 bindings active only when debugging.
4. `validate.ps1` 12/12.
5. **v0.4.0-beta cut.**

### V2-E3 — Locals + watch + call stack

**Status:** planned
**Target release:** v0.4.1-beta (closes phase E)
**Dependencies:** V2-E2
**Blocks:** V2-F0

**Change.** Variable inspection panel (new Variables widget in BottomPanelController infrastructure). Locals scope, watch expressions (user adds via right-click → Add to Watch), call-stack panel with frame switching.

**Acceptance.**
1. Locals panel populates at breakpoint.
2. Add-to-watch works.
3. Call stack frame switching updates the editor cursor to the matching source line.
4. `validate.ps1` 12/12.
5. **v0.4.1-beta cut** closing phase E.

## Phase F — Terminal

Highest architectural risk. Pushed last unless reprioritized.

### V2-F0 — ADR-0014

**Status:** planned
**Target release:** none (docs)
**Dependencies:** none
**Blocks:** V2-F1

**Change.** Commit `docs/adr/0014-terminal-widget.md` locking a Lisan-owned terminal backend, trust gate, and deferred ConPTY/QTermWidget upgrade path.

### V2-F1 — Windows terminal backend

**Status:** planned
**Target release:** none (foundation)
**Dependencies:** V2-F0
**Blocks:** V2-F2

**Change.** Validated process-backed terminal execution in `src/TerminalBackend.{h,cpp}`. Spawn cmd.exe / PowerShell with explicit program/argument lists, write stdin, read merged output, and emit process exit. Raw ConPTY remains deferred until its stdin path is reproducible in the Windows/Qt toolchain.

**Acceptance.**
1. Test that spawning cmd.exe and writing "echo hello\r\n" produces "hello" in the backend output buffer.
2. Test multi-line input/output.
3. Test process exit propagates as a signal.
4. `validate.ps1` 13/13 (new `acs_terminal_backend_tests` binary).

### V2-F2 — Terminal UI integration + multi-shell picker

**Status:** planned
**Target release:** v0.5.0-beta (closes phase F and V2)
**Dependencies:** V2-F1
**Blocks:** nothing — V2 closes

**Change.** Wire `TerminalBackend` into the existing terminal panel. UI: shell-picker dropdown in the terminal panel (cmd / PowerShell / WSL bash when available, configured per-workspace). QTermWidget/ConPTY remains an upgrade path, not a V2 blocker.

**Acceptance.**
1. Manual QA: open terminal panel; pick PowerShell; type `Get-Process`; see output.
2. Shell-picker persists choice per workspace.
3. Bidi/Arabic text in terminal output renders acceptably in the text-backed terminal surface.
4. Replaces the placeholder `terminal.openPowerShell` command from V1.
5. **v0.5.0-beta cut** closes V2.

## Cross-cutting policy

### Release cadence
- **Phases A and B** ship per-slice or per-phase at user discretion. Recommended: per-slice for B (small, independent); per-phase bundle for A (multi-cursor extensions feel like one feature).
- **Phases C, D, E, F** ship per-theme. Each phase's final slice cuts a new minor version (v0.3.0, v0.3.2, v0.4.1, v0.5.0).
- Each `v0.x.0-beta` cut follows the V1.5-proven pattern: WiX preprocessor bump → GHA workflow bump → release notes prepend → KNOWN_ISSUES update → commit → push → trigger full-scenario GHA → tag → publish.

### ADR-first within themes
- Themes C, E, F each have an ADR slice (V2-C0, V2-E0, V2-F0) that must land before any other slice in that theme dispatches.
- ADR slices have no release target; they're docs.

### V2 completion criteria
V2 closes when phases A-F are done OR explicitly deferred. The milestone close cuts `v1.0.0-rc1` if all phases ship; otherwise the last `v0.x.0-beta` is the milestone-close release and the deferred phases carry to V3.

### Tracking convention
V2 is historical. Use the release notes and Git history for landed/released
state; keep this roadmap focused on product scope rather than live execution
status.

## Risk register

| Risk | Detection | Mitigation |
|---|---|---|
| lsp-framework integration harder than the demo suggests | V2-C2 dispatch | Fall back to rolling JSON-RPC over QProcess directly. Cost: ~2 extra slices. |
| Apython translation-shim position mapping has unsolvable edge cases | V2-C1 testing | Iterate; document known-bad inputs in KNOWN_ISSUES.md; defer to V3 if blocking. |
| debugpy DAP version compatibility | V2-E1 dispatch | Pin a known-good debugpy version in apython's bundled runtime. |
| QTermWidget Windows bidi rendering is poor for Arabic terminal output | V2-F2 testing | Switch to Contour. ~3 extra slices. |
| Multi-cursor IME composition unsolvable on current Qt 6 | V2-A3 prototyping | Document as open Qt platform limitation; ship single-cursor IME with degraded multi-cursor behavior; or opt-in mode. |
| First submodule (lsp-framework) breaks GHA cross-repo checkout flow | V2-C2 GHA run | Use `submodules: recursive` on actions/checkout@v4; ensure runner has SSH/HTTPS access. Cost: ~1 extra debug cycle. |
| Cold-VM smoke flake recurs (like the v0.2.0-beta first-attempt) | Any release-cut GHA run | Re-run once; if persistent, bump `--smoke-exit-ms 1500` → 3000 in `scripts/installed-smoke.ps1`. |

## Non-goals (explicitly NOT V2)

| Feature | Why deferred |
|---|---|
| Direct Arabic-source LSP analysis (no translation shim) | Requires apython parser and type inference from scratch; V3+ |
| Git source-control UI | V3 |
| Multi-root workspaces | V3 |
| Profiles, extensions, remote SSH, AI side panel | V3+ |
| Cross-platform port | Not planned in any milestone |
| ARM64 Windows | Not planned in any milestone |
| WiX ProductVersion single-source consolidation | Could be added to phase B; tracked as candidate but not committed |

## How to read this roadmap

- **Picking work:** Start with Phase A (smallest surface, finishes V1.5 multi-cursor) OR Phase C ADR (largest upside; gates LSP work). Phases A/B/C ADR can run in parallel across sessions.
- **Sequencing within a phase:** Slices are numbered with their dependency order. Don't dispatch V2-A2 before V2-A1 lands; don't dispatch V2-E1 before V2-E0 + V2-C2 land.
- **Status tracking:** V2 status is now historical. Use release notes and Git history for landed/released state.
- **Updating this roadmap:** Scope refinements, deferrals, or additions can edit this file.
- **At V2 close:** retitle this file to `docs/ROADMAP-V2-archive.md` and create `docs/ROADMAP-V3.md`.

---

*Composed 2026-05-27 after a research pass on Qt 6 LSP libraries, debugpy, ConPTY widgets, and Kate multi-cursor reference implementations during V1.5 close.*
