# Lisan Studio V1.5 Roadmap
> Status: planning, composed 2026-05-23 against HEAD `4a8ee89` (v0.1.1-beta).
> Release cadence: per-slice. Each slice ships independently; code/MSI changes cut a new `v0.1.x-beta` release, docs/CI-only slices land via push without a release.
## Scope
V1.5 is **editor maturity plus operator-experience polish on the V1.1 workbench**. The keystone is multi-cursor + column selection. Everything else is small, time-bounded, or carrying-forward backlog from the V1 cut.
V1.5 is NOT:
- Real terminal execution. (Deferred to V2: requires ANSI rendering, ConPTY integration, multi-shell choice, and possibly a different terminal widget. The architectural surface is V2-scale; the half-step is ugly UX.)
- Any LSP-driven feature (completion, hover, go-to-def, references, rename, semantic highlighting, outline). All V2.
- Debugger, test discovery, Test Explorer. All V2.
- Git source-control UI. V3.
- Multi-root workspaces, profiles, extensions, remote SSH, AI side panel. V3+.
- Cross-platform (Linux/macOS) or ARM64 Windows support. Lisan Studio stays Windows-x64 through V1.5.
- Command-id style migration. The current 3-style mix is harmless until LSP integration in V2 forces a canonical id format; renaming preemptively breaks users' persisted shortcut bindings without a forcing reason.
## Slices in sequence
### Slice 1 — GHA Node.js 24 migration (NOT MSI release; CI-only)
**Urgency:** time-bounded. GitHub Actions defaults runners to Node.js 24 on **2026-06-02**. After that date, the current workflow still works (an opt-out exists) but the deprecation tail starts.
**Change.** `.github/workflows/msi-tests.yml` and `hello.yml`: bump `actions/checkout@v4` and `actions/upload-artifact@v4` to whichever current major supports Node 24, OR set `FORCE_JAVASCRIPT_ACTIONS_TO_NODE24=true` at the workflow level. Verify both workflows still pass.
**Acceptance.** Workflow runs without the Node-20 deprecation warning in the run log.
### Slice 2 — WiX cross-product-name upgrade limitation (NOT MSI release; docs-only)
**Reality check.** Probably only the maintainer has a legacy `ArabicCodeStudioQt` install in the wild. No code change; document the limitation.
**Change.** Add a one-section paragraph to `docs/INSTALLATION.md` AND to the v0.1.1-beta release's `KNOWN_ISSUES.md` describing: "If you have an older `ArabicCodeStudioQt` installation from the early private beta (pre-rename), it will NOT be removed when you install Lisan Studio. Uninstall it manually via Programs and Features before installing Lisan Studio v0.1.1-beta or later."
**Acceptance.** One docs commit, both files updated, no code change.
### Slice 3 — release-evidence.ps1 stale wording cleanup (NOT MSI release)
**Change.** `scripts/release-evidence.ps1` lines 268-270 (per Phase-1 inspection during the V1 cut) still describe the deleted Hyper-V harness pattern. Update wording to describe the GHA-based test pipeline (`msi-tests.yml`, self-hosted runner, `workflow_dispatch`).
**Acceptance.** `git grep -n "Hyper-V\|orchestrator" scripts/release-evidence.ps1` shows no references to the deleted harness. Script still parses cleanly.
### Slice 4 — Trust audit mirrored into release-evidence bundle (NOT MSI release)
**Change.** Mirror the `Get-WorkspaceTrustAuditLines` helper from `scripts/collect-diagnostics.ps1` into `scripts/release-evidence.ps1`. Add a `## Workspace Trust Audit` section to the release-evidence bundle output. Add `qa/tests/Test-ReleaseEvidenceSurfacesTrustAudit.ps1` mirroring the existing diagnostics-bundle PS1 test.
**Policy decision baked into the slice:** all `workspace.*` audit events go in BOTH the diagnostics bundle AND the release-evidence bundle. The two bundles diverge only on non-audit content. Future audit events (e.g., a `workspace.terminal.launched` event if/when terminal ships) automatically appear in both.
**Acceptance.** Both PS1 tests pass (existing diagnostics one + new release-evidence one). The release-evidence bundle for any future release page includes the trust audit section.
### Slice 5 — Code signing investigation (NOT MSI release; investigation)
**Change.** Investigate the current state of MSI signing. Read what `artifacts/SIGNING_STATUS.txt` currently says (already an asset in the v0.1.0-beta and v0.1.1-beta releases). If signing isn't done, document the SmartScreen impact ("Windows protected your PC" warning on first launch for unsigned installs from the internet) and either:
1. Ship signing now via Authenticode (requires a code-signing certificate purchase, ~$200-500/year, plus signing step in `scripts/package.ps1` or as a GHA workflow step after `package.ps1`).
2. Document the SmartScreen experience explicitly in `KNOWN_ISSUES.md` and accept it for V1.5.
Produce `docs/adr/0011-msi-code-signing.md` with the decision and reasoning.
**Acceptance.** ADR committed. If signing is chosen, follow-on slice ships the signing pipeline.
### Slice 6 — Search/replace 500-file cap raise (MSI release; v0.1.2-beta) — landed cff51d2
**Change.** Raise `MaxScannedFiles` in `src/SearchService.cpp:14` and `src/ProjectReplaceService.cpp:16` from 500 to a higher fixed number (recommend: 5000), OR remove the cap entirely and rely on `MaxFileBytes` per-file plus the existing per-call `limit` parameter as the only bounds. The streaming-search option from the earlier roadmap draft is over-scoped for V1.5; defer to V2 if file counts exceed 5000 in practice.
Add a regression test confirming a 1000-file project surfaces results from all 1000 files (or up to 5000 per the new cap).
**Acceptance.** Existing tests pass. New regression test pins the new cap. The "results truncated" status notice still fires correctly when the new cap IS hit (test that case too with a 6000-file fixture if cap is 5000, or skip if cap is removed).
### Slice 7 — Crash-recovery verification for dirty untitled buffers (likely investigation; possible fix slice) — landed 2334f0a
**Change.** V1 ships session restore + "persist untitled session drafts" (commit `d089406`). Verify what happens when the app is killed ungracefully with a dirty untitled buffer open (kill via Task Manager, force-shutdown, BSOD simulation). If the dirty untitled content survives the kill, document the invariant and add a test. If it doesn't, ship a fix: debounced auto-save of dirty buffers (titled and untitled) to `%LOCALAPPDATA%\LisanStudio\recovery\` on every contentsChanged with a 5-second debounce; on next launch, prompt the user to recover or discard.
Decide as part of the investigation phase whether a fix is needed.
**Acceptance.** Either: a test asserts dirty untitled buffer survives ungraceful exit (no fix needed); or a fix lands plus the test. Recovery directory location decided and documented.
### Slice 8 — Performance baseline measurement (NOT MSI release; investigation)
**Change.** Measure the current editor's real-world performance on a representative `.apy` workload:
- Open a 10k-line, 50k-line, and 100k-line file. Wall-clock to first paint, to fully scrollable.
- Cursor movement latency (key-down to caret-rendered).
- Find/replace storm latency on each file size.
- Memory footprint at idle and under torture.
Produce `docs/performance-baseline-2026.md` with the numbers. This data informs whether multi-cursor's performance budget is realistic and where the editor is fast/slow today.
**Acceptance.** Baseline document committed. No code change. Numbers are reproducible (script committed alongside the doc, or instructions on how to reproduce).
### Slice 9 — Multi-cursor and column selection (MSI release; v0.1.x-beta)
**This is the V1.5 keystone.** Architecturally largest. Has a hard exit ramp.
**Step 0 (gate, FIRST):** implement the deferred `multiCursorOverlayTorture` test in `tests/TestEditorTorture.cpp` (the deferral comment is at line 23). The test creates 10 active QTextCursor decorations on a 100k-line mixed-direction file, performs 200 inserts spread across the cursors, and asserts:
- Wall-clock for the 200 inserts ≤1 second.
- Memory footprint doesn't grow by more than 100MB during the test.
- Cursor positions remain logically correct after each insert.
If Step 0 fails or shows >50ms per 10-cursor operation, **abort the slice and move multi-cursor to V2 alongside the LSP/semantic-highlighting work**. The "switch editor widget" fallback from the earlier roadmap draft is not a V1.5 option — it's a multi-month rewrite that invalidates the V1 controller decompositions. The only V1.5 fallbacks are: ship column-select-only (no add-cursor), or defer the whole feature.
**Step 1+ (if Step 0 passes):** Implement multi-cursor.
1. `MultiCursorModel` owning a primary `QTextCursor` plus a list of secondaries. Owned by `EditorSurface`.
2. Key bindings: `Alt+Click` add cursor, `Alt+Shift+Click` column-select between cursors, `Esc` collapse to primary, `Ctrl+D` add next match, `Ctrl+Shift+L` add cursors at all find matches.
3. Column-select mode: `Alt+Shift+drag` produces a rectangular selection rendered as cursors-per-line at the same column.
4. Type/paste/delete operations splice through all cursors in order with a single undo group.
5. Find/replace highlights extend to all cursors' selections.
6. Visible-whitespace, indent-guide, line-number-area painters get a sanity pass with multiple visible cursors.
7. If MainWindow.cpp grows during the slice (the new code probably lives on EditorSurface), state the before/after line counts. The slice budget for MainWindow.cpp growth is +0; everything goes on EditorSurface or a new MultiCursorController.
**Acceptance.**
- `multiCursorOverlayTorture` passes with the budgets above.
- All existing torture tests still pass.
- A real Arabic mixed-direction multi-cursor test (cursors at logical positions in mixed RTL/LTR text) passes.
- `Ctrl+D` add-next-match integrates with the existing `EditorFindService`.
- The deferral comment at `tests/TestEditorTorture.cpp:23` is removed and replaced with the real test.
## Cross-cutting policy
- **Slice ordering rationale.** Time-bounded (Slice 1) and tiny docs items (Slices 2-4) first because they ship in days. Investigation slices (5, 7, 8) before the keystone because they produce data the keystone needs. Multi-cursor last because it's the biggest risk; landing it last means V1.5 has shipped something even if multi-cursor falls.
- **Per-slice release cadence.** Each MSI-touching slice cuts a new `v0.1.x-beta` release. Docs/CI-only slices land via push without a release. Cumulative V1.5 release count is ~3 MSI releases (Slice 6, possibly Slice 7, Slice 9).
- **V1.5 completion criteria.** V1.5 milestone closes when: Slices 1-8 are done OR explicitly deferred, AND Slice 9 either lands (multi-cursor in) or is documented as moved to V2 (multi-cursor deferred). The milestone close cuts a final release (v0.2.0-beta) if multi-cursor landed; otherwise the last v0.1.x-beta is the milestone-close release.
## Risk register
| Risk | Detection | Mitigation |
|---|---|---|
| Multi-cursor torture test fails | Step 0 of Slice 9 | Documented exit ramp: defer to V2 |
| Code-signing certificate cost not approved | Slice 5 investigation outcome | Document SmartScreen experience in KNOWN_ISSUES; ship unsigned |
| Crash-recovery fix is more invasive than expected | Slice 7 investigation outcome | Scope the fix to "untitled only" if titled recovery already works |
| GHA Node 24 migration breaks the workflow | Slice 1 verification | Roll back the action versions; cut a follow-up after the actions stabilize |
| Performance baseline reveals editor is slow on 100k-line files | Slice 8 outcome | Inform Slice 9 budgets; possibly inform a V2 perf-pass milestone |
| User finds the per-slice release cadence noisy | Feedback during V1.5 | Switch to a single v0.2.0-beta cut at milestone close |
## Non-goals (explicitly NOT V1.5)
| Feature | Why deferred |
|---|---|
| Real terminal execution | ANSI rendering + ConPTY + shell choice + encoding handling = V2-scale architectural surface; the half-step is ugly UX |
| LSP boundary | Different architectural surface; V2 |
| Completion, hover, signature help | LSP-driven; V2 |
| Go-to-def, find references, rename | LSP-driven; V2 |
| Semantic highlighting | LSP-driven; V2 |
| Outline, workspace symbols | LSP-driven; V2 |
| Debugger | New process model; V2 |
| Test discovery / Test Explorer | Project-model expansion; V2 |
| Command-id style ADR + migration | Defer until LSP forces canonical id format; V2 |
| Git source-control UI | V3 |
| Multi-root workspaces | V3 |
| Profiles, extensions, remote SSH, AI side panel | V3+ |
| Linux / macOS port | Not planned in any milestone; Lisan Studio is Windows-x64 |
| ARM64 Windows | Not planned in any milestone |
## How to use this roadmap
- **Picking work:** start at Slice 1 (the time-bounded one). Each slice is independently committable.
- **Updating this doc:** when a slice lands, mark it done with the tag (or "merged via PUSH" for non-release slices) and the SHA. When V1.5 closes, retitle to `docs/ROADMAP-V1.5-archive.md` and create the V2 roadmap.
- **Auditing against current state:** cross-reference slice goals against the V1.1-beta KNOWN_ISSUES.md, the v0.1.1-beta release page, and the current `MainWindow.cpp` line count.
---
*Composed 2026-05-23 by Claude (strategy/audit) after a grill pass on the first draft. Slice prompts will be drafted per-slice when work begins.*
