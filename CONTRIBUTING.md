# Contributing to Lisan Studio

## Role-split execution model

Lisan Studio uses a three-role model for all feature work:

| Role | Who | Does |
|---|---|---|
| **Strategy / audit** | Claude (this session) | Plans slices, writes Codex prompts, audits commits, drafts ADRs, cuts releases |
| **Implementation** | Codex Desktop | Implements slices from prompt specs; no scope expansion |
| **Operator** | GalaxyRuler | Pastes prompts into Codex, approves diffs, commits, reports SHA back |

Codex implements exactly what the prompt says. If scope is unclear, halt and ask Claude — do not expand. If a pre-check fails, halt and report — do not proceed.

---

## Branch conventions

| Pattern | Use |
|---|---|
| `codex/<milestone>-<feature>` | Feature/milestone branch (e.g. `codex/v3-git-workspace`) |
| `codex/adr-<number>-<slug>` | ADR-only docs branch |
| `main` | Stable; only receives squash-merges from reviewed branches |

Squash-merge label format: `<type>(<scope>): <description>` — e.g. `feat(v3): Git source-control UI and multi-root workspace support`.

---

## Slice lifecycle

```
planned → prompt-drafted → dispatched → landed → released
□          ◔               ◑            ◕         ●
```

1. **Planned** — slice defined in roadmap and vault tracking table.
2. **Prompt-drafted** — Claude writes the Codex dispatch prompt.
3. **Dispatched** — operator pastes prompt into Codex Desktop; Codex begins.
4. **Landed** — operator commits Codex output; reports SHA to Claude; Claude audits diff.
5. **Released** — release-cut slice runs `package.ps1`, cuts GHA run, publishes GitHub release.

---

## Dispatch prompt structure

Every Codex prompt follows this template:

```
Scope guard
-----------
Slice goal: <one sentence>
Creates / modifies: <file list>
No <excluded scope>.

Pre-checks (HALT if any fails):
1. git status clean, on <branch>, HEAD = <sha>
2. <file> absent / present as expected
...

DO:
- Create/modify <file> with EXACTLY this content:
  <content block>
...

DO NOT:
- Do NOT touch <excluded dirs/files>
- Do NOT bump version unless specified

Verification:
1. <validation command> — expected result
2. git diff --name-only shows exactly: <file list>
...

Acceptance
----------
- <acceptance criterion 1>
- <acceptance criterion 2>

Squash-merge label: <type>(<scope>): <description>
```

---

## Pre-check protocol

Before every slice dispatch, verify:
1. `git status` is clean.
2. Active branch matches expected branch.
3. `git log --oneline -1` HEAD SHA matches expected SHA.

If any pre-check fails: **halt, report, do not proceed.**

---

## Audit protocol (Claude, post-commit)

After operator reports a commit SHA, Claude audits:
1. `git show <sha> --stat` — files changed match scope.
2. `git show <sha>` — diff is clean (no debug artifacts, no scope bleed).
3. Acceptance criteria from prompt are met.
4. No line-number citations in committed docs (cite by content pattern).

If audit fails: open a follow-up slice to fix. Do not amend.

---

## Halt conditions

Stop and report to Claude if:
- Pre-check fails.
- Codex output touches files outside the stated scope.
- A test that was passing before the slice now fails.
- `validate.ps1` exits non-zero.
- Ambiguity in the prompt requires judgment beyond the spec.

---

## ADR process

Architecture decisions live in `docs/adr/`. Numbering is sequential (`0001`, `0002`, …). Each ADR:

- States context, options considered, decision, and consequences.
- Is committed on `main` (docs-only slice, no src/ changes).
- Is referenced from `docs/ROADMAP-*.md` and the vault MOC.

Current ADR index:

| # | Title | Status |
|---|---|---|
| 0001–0009 | V1 / V1.5 decisions | Accepted |
| 0010 | MSI code signing | Accepted (option b: unsigned) |
| 0011 | lsp-framework submodule | Accepted |
| 0012 | pylsp translation shim | Accepted |
| 0013 | debugpy DAP | Accepted |
| 0014 | QTermWidget + ConPTY | Accepted |
| 0015 | Command ID style | Accepted |
| 0016 | Git backend (libgit2) | Accepted |

---

## Anti-patterns (learned from V1.5)

| Anti-pattern | Correct approach |
|---|---|
| Cite docs by line number | Cite by content pattern / heading |
| Reference HEAD SHA in committed docs | Use semantic references only |
| `git show HEAD --stat` for pre-commit verification | `git diff --stat` before commit |
| Cold-VM smoke flake → treat as real failure | Re-run once on warm VM; only act if second run also fails |
| `git add -A` without review | Stage specific files by name |
| Amend after pre-commit hook failure | Create new commit after fixing |

---

## Release cut protocol

1. All slices for the milestone must be in state `●` (released) or explicitly deferred.
2. Run `.\scripts\validate.ps1` — must pass all suites.
3. Run `.\scripts\package.ps1` — produces `artifacts\LisanStudio-<version>.msi`.
4. Trigger `gh workflow run msi-tests.yml -f scenario=full` on self-hosted runner `lisanstudio-qa`.
5. GHA run must pass both `msi-install` and `msi-upgrade` jobs.
6. Run `gh release create <tag> --target <sha> --title "..." --notes "..."`.
7. Update vault MOC tracking table and project board.

---

## Vault

Project planning lives in the command-center Obsidian vault at `C:\Users\Admin\command-center`.

| Path | Contains |
|---|---|
| `projects/active/lisan-studio.md` | Project board (source of truth for current state) |
| `plans/lisan-studio/` | V2 MOC + phase notes |
| `plans/lisan-studio/v3/` | V3 MOC + phase notes (G1–G6, MR) |

Vault wikilinks are bare-name and unique — folder moves never break them.

---

## Self-hosted CI runner

The `lisanstudio-qa` runner is a Hyper-V guest VM on the dev machine. If it goes offline:

1. Check VM is running in Hyper-V Manager.
2. Verify guest IP is on the same subnet as the host Default Switch (`192.168.16.x`).
3. Restart the runner task inside the VM if networking is healthy.

Cold-VM flakes on the first GHA run after a reboot are normal — re-run once.
