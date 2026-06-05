# Lisan Studio V2 Execution Plan

> Companion to [docs/ROADMAP-V2.md](ROADMAP-V2.md). The roadmap is the **spec**; this file is the **operational playbook** — how slices flow through the role-split execution model and where the live status lives. Updated whenever a slice changes state.

## Operational model

Lisan Studio executes via a three-role split:

| Role | Responsibility | Boundary |
|---|---|---|
| **Claude (strategy/audit)** | Plans, slice prompts, audits diffs, memory updates, release-publish operator commands | Does NOT implement code or run validate.ps1 |
| **Codex Desktop (implementer)** | Implements slices from pasted prompts, runs validate.ps1, commits | Does NOT push, tag, or publish |
| **Operator (user)** | Pastes prompts into Codex Desktop, confirms commits, approves go/no-go on dispatch & publish | Owns the keyboard for both Desktop apps |

Authoritative reference: `~/.claude/projects/C--/memory/feedback_role_split.md`.

Why this works: Codex Desktop has different sandbox/permissions than `mcp__codex__codex`, the user can see and intervene in Codex's work, and Claude can audit without conflict-of-interest. The V1.5 arc proved this across 15 commits with five mid-execution halts that all resolved cleanly.

## Slice lifecycle states

Every V2 slice transits through these states:

```
planned ── prompt-drafted ── dispatched ── landed ── released
   │            │                │           │          │
   │            │                │           │          └── tagged + MSI published in a v0.x.0-beta release
   │            │                │           └── committed on origin/main
   │            │                └── pasted into Codex Desktop; Codex working
   │            └── Claude has drafted the full Codex prompt; awaiting operator paste
   └── in the roadmap spec but no prompt yet
```

**State transitions update the tracking table below.** Each transition has a one-line memory effect:

- `planned → prompt-drafted`: Claude drafts the slice prompt in chat. No table update yet.
- `prompt-drafted → dispatched`: operator pastes the prompt. Update table cell `Status` to `dispatched`.
- `dispatched → landed`: Codex commits + operator reports back. Update table `Status` to `landed` and `Commit` to the SHA.
- `landed → released`: a release-cut slice ships this slice's code as part of a `v0.x.0-beta`. Update table `Status` to `released` and `Release` to the tag.

## Dispatch protocol

For each slice, the dispatch flow is:

1. **Pre-dispatch sample.** Before drafting the prompt, Claude reads the actual files the slice will touch. Cite by content pattern (`Select-String "<unique-substring>"`), not bare line numbers — line numbers shift and produced five halt-causing defects across V1.5.
2. **Audit the chain.** For every slice that extends V1.5 machinery (multi-cursor, perf budgets, etc.), grep all inbound and outbound call sites of the affected symbols. Don't draft an audit-gate prompt from a one-direction sample (this caused the V1.5 slice-8 halt — Claude's audit findings were factually wrong).
3. **Draft the prompt** following the proven structure (see "Sample Codex prompt template" below).
4. **Surface to operator.** Output the prompt in chat, wrapped in a fenced code block, with explicit "Paste into Lisan Studio Codex" cue above.
5. **Wait for commit report.** Operator pastes, Codex commits, operator reports `Committed <sha> with subject <subject>`.
6. **Audit the diff.** Claude reads the actual diff, verifies against the slice's acceptance criteria. Never trust commit-body prose alone.
7. **Push.** Claude runs `git push origin main` (proven safe in V1.5 — it's a fast-forward of already-committed work). For tag + release publish, ask operator explicitly.
8. **Update tracking table.** Slice state advances to `landed` (or `released` if it's a release-cut slice).
9. **Produce the next prompt.** Per V1.5 standing rule: do not wait to be asked. End of audit and start of next prompt belong in the same turn.

## Audit checklist template

For each landed slice, Claude verifies:

- [ ] **File scope:** `git show <sha> --stat` confirms only the files listed in the prompt's DO section changed.
- [ ] **Line-count claims:** if the commit body cites src/ line counts, verify with `git show <sha>:<file> | wc -l`. Codex's commit-body absolute line counts were wrong throughout V1.5 slices 6-9; resolved at slice 9, could recur.
- [ ] **Scope creep:** any file touched OUTSIDE the prompt's DO section is investigated. Acceptable scope expansions (e.g., V1.5 slice 9 touched `TestMainWindow.cpp` because command-inventory tests are exact-match) are documented in the commit body.
- [ ] **Perf budgets:** if the slice touches editor code, ALL prior perf budgets must still pass. Re-grep the torture log for any `PERF metric=... elapsed=...` lines and verify each is within its budget.
- [ ] **Test plumbing:** new test binaries appear in ctest output. New test cases appear in `<binary>.exe -functions` output.
- [ ] **Squash-merge label:** commit subject matches the prompt's squash-merge label.
- [ ] **Acceptance criteria:** each acceptance bullet from the slice's spec is verifiable in the diff or via a follow-up grep/test. Don't accept "should work" — verify each.

## Halt protocols

### When Codex halts mid-slice

Codex's halts in V1.5 were always correct — five halts, zero false-positives. The pattern: Codex finds the prompt's pre-check assertion doesn't match reality, stops without making changes, reports what it found. **Treat every Codex halt as a real finding.** Read the actual files Codex cites, verify, then either:

- **Prompt was wrong:** Claude re-issues a corrected prompt (most common — 5/5 V1.5 halts).
- **Reality drifted:** Update memory and the slice spec to match.
- **Genuine disagreement:** Ask the operator to arbitrate.

### When Claude must halt mid-audit

If during audit Claude discovers the slice violates an invariant Codex's tests didn't catch (e.g., a perf budget regression, a missing tracked file, a memory leak), halt the slice. Options:

- **Repairable in a follow-up commit:** Accept the slice, draft a follow-up fix prompt.
- **Architecturally broken:** Recommend revert + redraft.

Either way: never silently push a slice with known defects.

## Live tracking table

This is the source of truth for "where are we in V2." Update on every state transition.

**Legend:** `□` planned · `◔` prompt-drafted · `◑` dispatched · `◕` landed · `●` released

### Phase A — Multi-cursor V2 extensions

| Slice | Title | Depends on | Status | Commit | Release | Notes |
|---|---|---|---|---|---|---|
| V2-A1 | Multi-cursor Ctrl+Z/Y dispatch | V1.5 slice 9 | ◕ | e2addc1 | — | Landed with V2-A2 in one Codex implementation commit |
| V2-A2 | Multi-cursor Tab indent dispatch | V2-A1 | ◕ | e2addc1 | — | Landed with V2-A1 in one Codex implementation commit |
| V2-A3 | Multi-cursor IME composition | V2-A1, V2-A2 | ◕ | b2edec9 | — | Committed IME text dispatch landed; live preedit preview remains documented limitation |
| — | **v0.2.1-beta cut** (phase A close) | V2-A3 | ◕ | d7328f1 | — | In-repo release prep landed; push/GHA/tag/publish still pending |

### Phase B — Editor polish foundations

| Slice | Title | Depends on | Status | Commit | Release | Notes |
|---|---|---|---|---|---|---|
| V2-B1 | Incremental autosave for untitled drafts | none | ◕ | da8fd43 | — | Dirty untitled drafts autosave every 5s and interrupted sessions prompt recovery |
| V2-B2 | MainWindow truncation-test seam | none | ◕ | 790f832 | — | Search truncation fixture now uses a 20-file cap seam |
| V2-B3 | Command-id style ADR + migration | none | ◕ | e9715ac | — | ADR-0015 landed; command IDs now use dotted camelCase with shortcut JSON migration |
| — | **v0.2.2-beta cut** (phase B close) | V2-B1..3 | ◕ | 5d89795 | — | In-repo release prep landed; push/GHA/tag/publish still pending |

### Phase C — LSP foundation

| Slice | Title | Depends on | Status | Commit | Release | Notes |
|---|---|---|---|---|---|---|
| V2-C0 | ADR-0011 + ADR-0012 | none | ◕ | 374f558 | — | ADR-0011 LSP transport and ADR-0012 apython backend accepted |
| V2-C1 | Apython translation-shim LSP server | V2-C0 | ◕ | lughat-althuban:6f1e4de | — | `lughat-althuban-lsp` entrypoint and stdio LSP contract landed in apython worktree |
| V2-C2 | LSP client integration in EditorSurface | V2-C0, V2-C1 | ◕ | ec0c044 | — | lsp-framework submodule + Qt LspClient landed; validate.ps1 now passes 11/11 |
| V2-C3 | Completion + hover | V2-C2 | ◕ | d16e9c3 | — | Completion popup + hover tooltip UI binding landed; validate.ps1 passes 11/11 |
| — | **v0.3.0-beta cut** (phase C close) | V2-C3 | ◕ | 9dbc414 | — | In-repo release prep landed; MSI artifact built locally; push/GHA/tag/publish/signing still pending |

### Phase D — LSP advanced

| Slice | Title | Depends on | Status | Commit | Release | Notes |
|---|---|---|---|---|---|---|
| V2-D1 | Go-to-definition + find-references | V2-C3 | ◕ | 91d488b | — | LSP definition/references requests, Ctrl+Click/F12 navigation, and References panel landed; validate.ps1 passes 11/11 |
| V2-D2 | Rename refactor | V2-D1 | ◕ | 89b2f38 | — | F2 rename, LSP workspace-edit parsing, multi-file preview, and validated rollback path landed; validate.ps1 passes 11/11 |
| V2-D3 | Semantic highlighting + outline + workspace symbols | V2-D2 | ◕ | 30874a9 | — | Semantic token overlay, document outline panel, Ctrl+T workspace-symbol picker, and tests landed; validate.ps1 passes 11/11 |
| — | **v0.3.2-beta cut** (phase D close) | V2-D3 | ◕ | 5cde635 | v0.3.2-beta | In-repo release prep landed; MSI artifact built locally (`DB5E395793CE474F55F9DFEEAC501ADB51380D68ED793C387D8BFDBACD1C46B7`); signing/GHA/tag/publish/manual QA still pending |

### Phase E — Debugger

| Slice | Title | Depends on | Status | Commit | Release | Notes |
|---|---|---|---|---|---|---|
| V2-E0 | ADR-0013 | none | ◕ | 99c49da | — | debugpy + Lisan-owned Qt DAP client accepted |
| V2-E1 | DAP client transport + initialize + launch | V2-E0, V2-C2 | ◕ | c628664 | — | Shared Content-Length JSON framing extracted; DapClient initialize/launch round-trip tests landed; validate.ps1 passes 12/12 |
| V2-E2 | Breakpoints + step + continue | V2-E1 | ◕ | fc0fcdd | — | Editor breakpoints, F5/F10/F11 run control, live debugpy adapter launch, staged-runtime debugpy copy/import check, and staged DAP smoke passed; validate.ps1 passes 12/12 |
| V2-E3 | Locals + watch + call stack | V2-E2 | ◕ | 6b778eb | v0.4.1-beta | Debug inspector tabs for variables/watch/call stack; DapClient stackTrace/scopes/variables/evaluate; validate.ps1 passes 12/12; staged debugpy inspection smoke passed |
| — | **v0.4.0-beta cut** + **v0.4.1-beta cut** | V2-E2 / V2-E3 | ◕ / ◕ | aed2764 / 5556e4f | v0.4.0-beta / v0.4.1-beta | E2/E3 release prep landed. Local MSI artifacts built: 0.4.0 `27D6991A4B161F56C96749B1C9CDDCC3579D4324BF04F19FC237CA8464CAC0A5` (69,165,576 bytes), 0.4.1 `C9EFEE7FF6FD1F42DC060C54820D479D01F087983DB71C8CE3D0BCB6E8C18EFB` (69,202,440 bytes); signing/GHA/tag/publish/manual QA still pending |

### Phase F — Terminal

| Slice | Title | Depends on | Status | Commit | Release | Notes |
|---|---|---|---|---|---|---|
| V2-F0 | ADR-0014 | none | ◕ | b4b2eab | — | Lisan-owned terminal backend + UI accepted; raw ConPTY/QTermWidget deferred until Windows input/rendering proof is safe |
| V2-F1 | Windows terminal backend | V2-F0 | ◕ | 990c9b2 | — | `TerminalBackend` process-backed shell execution landed; cmd spawn/write/read/exit tests pass; validate.ps1 passes 13/13 |
| V2-F2 | Terminal UI integration + multi-shell picker | V2-F1 | □ | — | — | Text-backed terminal panel first; ConPTY/QTermWidget remain upgrade paths |
| — | **v0.5.0-beta cut** (V2 milestone close) | V2-F2 | □ | — | — | Final V2 release; archive ROADMAP-V2 |

## Sample Codex prompt template (V2-A1 worked example)

Future sessions can copy this skeleton when drafting V2 slice prompts.

```text
Scope guard
-----------
Slice goal: [one sentence].

NON-GOALS (explicit deferrals):
- [item]
- [item]

Pre-checks (HALT if any fails):
1. git status clean, on origin/main, HEAD = <expected sha>.
2. Select-String "<unique anchor 1>" <file> returns N hits.
3. Select-String "<unique anchor 2>" <file> returns N hits.
   (Cite by content pattern, NEVER by bare line number — line numbers
   shift between read and write.)

DO (Phase 1 — [header/data model]):
- src/<file>: [exact change with code or grep target]
- [...]

DO (Phase 2 — [implementation]):
- [...]

DO (Phase 3 — [tests]):
Add N test cases in tests/<file>:
1. <testName1>: [behavior + assertions]
2. <testName2>: [...]

DO NOT:
- Do NOT modify <files outside scope>.
- Do NOT bump the WiX product version.
- Do NOT cut a release in this slice.

Verification:
1. .\scripts\validate.ps1 — N/N ctest binaries pass.
2. git diff --check — clean.
3. git diff --stat (PRE-commit) OR git show HEAD --stat (POST-commit)
   confirms only:
   - <file1>
   - <file2>
4. <perf budget greps if applicable>
5. <line-count verification via git show HEAD:<file> | wc -l>

Acceptance
----------
- [criterion 1]
- [criterion 2]

Squash-merge label: feat(component): <imperative title>

After commit: operator pushes via `git push origin main`. [No GHA |
GHA dispatch] needed in this slice.
```

## Release-cut protocol

Distilled from V1.5's three successful release cuts (v0.1.2-beta, v0.1.3-beta, v0.2.0-beta). Each `v0.x.0-beta` cut is its own slice with this exact structure:

### 1. Code change (Codex)
- `packaging/wix/LisanStudio.wxs`: bump `<?define ProductVersion = "X.Y.Z" ?>` preprocessor default.
- `.github/workflows/msi-tests.yml`:
  - `msi-install` job: bump `-ProductVersion` arg and `LisanStudio-X.Y.Z-beta.msi` filename in install step + upload-artifact path.
  - `msi-upgrade` job: bump `-ProductVersion`, `-ExpectedReplacementVersion`, and replacement-MSI filename. **Baseline pull stays at v0.1.0-beta** — never change the persistent cross-version anchor.
- `docs/RELEASE_NOTES.md`: prepend new `# vX.Y.Z-beta (date)` section above the previous release's heading. Include user-visible features, known limitations.
- `docs/KNOWN_ISSUES.md`: update bullets where features have shipped (e.g., "X deferred" → "X shipped in vX.Y.Z-beta, limitations Y/Z remain").
- Commit. Squash label: `chore(release): cut vX.Y.Z-beta`.

### 2. Push + dispatch (Claude)
```powershell
git -C "C:\Users\Admin\arabic-code-studio-qt" push origin main
gh workflow run msi-tests.yml --repo GalaxyRuler/lisan-studio --ref main --field scenario=full
gh run watch <id> --repo GalaxyRuler/lisan-studio --exit-status
```

### 3. Verify + flake-check
If first run fails on a cold VM (the msi-install smoke timing-race seen in v0.2.0-beta's first attempt), **re-run once** before alarming. Cold-VM flake on a freshly-reset baseline is a known pattern.

### 4. Tag + publish (Claude, with operator approval)
```powershell
git -C "C:\Users\Admin\arabic-code-studio-qt" tag -a vX.Y.Z-beta -m "Lisan Studio vX.Y.Z-beta"
git -C "C:\Users\Admin\arabic-code-studio-qt" push origin vX.Y.Z-beta

$evDir = Join-Path $env:TEMP "lisan-vX.Y.Z-evidence"
gh run download <runId> --repo GalaxyRuler/lisan-studio --name "msi-upgrade-evidence-<runId>" --dir $evDir
$msi = Join-Path $evDir 'artifacts\LisanStudio-X.Y.Z-beta.msi'
$sha = (Get-FileHash -Algorithm SHA256 -LiteralPath $msi).Hash.ToLower()

gh release create vX.Y.Z-beta --repo GalaxyRuler/lisan-studio --prerelease `
  --title "vX.Y.Z-beta" `
  --notes "<inline notes — extract from RELEASE_NOTES.md vX.Y.Z section>" `
  $msi
```

### 5. Memory pass (Claude)
- Update `project_lisan_studio.md` current-state section with tag, commit SHA, MSI SHA256, GHA run ID, publish timestamp.
- Update `MEMORY.md` index entry.
- Update this file's tracking table: mark all slices included in this release with state `released`.

## Cross-session continuity

A future Lisan Studio session (this one or another) onboards by reading:

1. `~/.claude/projects/C--/memory/MEMORY.md` — index entry shows current release.
2. `~/.claude/projects/C--/memory/project_lisan_studio.md` — current state, commit ledger, key facts.
3. `~/.claude/projects/C--/memory/feedback_role_split.md` — operating model.
4. `docs/ROADMAP-V2.md` — the spec.
5. `docs/V2-EXECUTION-PLAN.md` — this file, the live status.
6. `git log --oneline origin/main..HEAD` and `git log --oneline -20` — current state.
7. `gh release list --repo GalaxyRuler/lisan-studio --limit 5` — published releases.

If `MEMORY.md` and the tracking table here disagree about a slice's state, **the tracking table wins** (it's the source of truth; memory is a denormalized cache).

## V1.5 anti-patterns

Six patterns that caused halts or near-misses in V1.5. Each one is a "do this instead" rule for V2.

### 1. Cite by content pattern, not bare line number
**Wrong:** "the deferred comment at `tests/TestEditorTorture.cpp:23`"
**Right:** "the deferred comment matching `Select-String 'V1.5: multi-cursor overlay torture' tests\TestEditorTorture.cpp`"
**Why:** lines shift between Claude's read and Claude's prompt-draft. Five halts in V1.5 traced to off-by-one line numbers.

### 2. Don't bake a commit's own SHA into committed roadmap text
**Wrong:** roadmap row says "landed `<short-sha>`" where Codex substitutes the slice's own SHA.
**Right:** "landed (see git log)" — uniform across all slices.
**Why:** a commit changing the roadmap to contain its own SHA changes that SHA. Chicken-and-egg.

### 3. Read actual files before alarming on prose summaries
**Wrong:** "slice 4 default emit isn't integrated" (based on stale memory note)
**Right:** grep `release-evidence.ps1` first; discover the default emit IS at line 301; correct the memory note instead of dispatching a corrective slice.
**Why:** Claude's own memory drifts. Always verify against the source before declaring a defect.

### 4. Verify line counts with `git show HEAD:<file> | wc -l`
**Wrong:** trust Codex's commit-body "src/MainWindow.cpp: 2986 → 2986" claim.
**Right:** run `git show HEAD:src/MainWindow.cpp | wc -l` and `(Get-Content src/MainWindow.cpp | Measure-Object -Line).Lines`. Compare.
**Why:** Codex's absolute line counts were wrong throughout V1.5 slices 6-9 (claimed 2986, actual 3322). Resolved at slice 9 but could recur.

### 5. Cold-VM smoke flake — re-run once before alarming
**Wrong:** v0.2.0-beta first attempt failed; Claude considered drafting a smoke-window fix.
**Right:** re-run the workflow on the warm VM; the second run passed cleanly with no code change. The first failure was a near-miss timing race during cold-VM startup after baseline reset.
**Why:** transient VM-cold flakes are real on self-hosted runners post-baseline-reset. Don't pattern-match every red run to a code regression.

### 6. Pre-commit verification uses `git diff --stat`, not `git show HEAD --stat`
**Wrong:** "git show HEAD --stat confirms only docs/ROADMAP-V2.md changed" — shows the LAST COMMIT, not the staged work.
**Right:** `git diff --stat` (or `git diff --name-only`) for pre-commit verification; `git show <sha> --stat` for post-commit verification.
**Why:** caught by operator during V2 slice 1 dispatch. Pre-commit vs post-commit verification commands are different; prompts must specify which phase the verification belongs to.

## How to update this file

- **Slice state change:** edit the relevant tracking-table row.
- **New slice added to V2:** add a row in the relevant phase's table. Update ROADMAP-V2.md in parallel.
- **Slice deferred:** strike-through the row OR move to a "deferred" section at the bottom (don't delete — keep the history).
- **New anti-pattern surfaces in V2:** append to the anti-patterns section with the same "Wrong / Right / Why" structure.
- **V2 closes:** archive this file to `docs/V2-EXECUTION-PLAN-archive.md` and create `docs/V3-EXECUTION-PLAN.md`.

---

*Composed 2026-05-27 by Claude (strategy/audit) as the operational companion to docs/ROADMAP-V2.md. Distilled from the V1.5 arc — 15 commits, 3 releases, 5 halts, all caught and resolved cleanly under this same role-split model.*
