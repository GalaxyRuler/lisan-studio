# Lisan Studio V3 Roadmap

> Status: planning, composed 2026-06-05 against HEAD `7185cd1` (v0.5.0-beta, V2 milestone closed).
> V2 archived at [docs/ROADMAP-V2.md](ROADMAP-V2.md). Execution plan (when created): `docs/V3-EXECUTION-PLAN.md`.
> Vault implementation plan: `plans/lisan-studio/v3/00-lisan-studio-V3-MOC.md`.
> Release cadence: per-phase. Each phase's final slice cuts a new release. V3 closes at `v1.0.0-rc1`.

## Mission

V3 adds Git source-control UI and multi-root workspace support. Lisan Studio
becomes a self-contained Arabic-first IDE — no external Git client required.

## Scope

**V3 IS:**
- Git status panel, diff viewer, stage/unstage hunks
- Commit workflow: message, commit, push, pull, fetch
- Branch management: create, switch, merge, delete
- History log panel + blame gutter in EditorSurface
- Multi-root workspaces (monorepo / multi-module support)

**V3 IS NOT:**
- Interactive 3-way merge editor (V4)
- GitHub / GitLab PR review UI (V4)
- Git LFS support (V4)
- Extension / plugin system (V4)
- Cross-platform port (not planned)
- ARM64 Windows (not planned)

## Architecture decision

**ADR-0016** (docs/adr/0016-git-backend.md): libgit2 via CMake FetchContent.
Fallback: git plumbing subprocess. Porcelain is never the fallback.

## Phase structure

```
G1 ADR-0016 (docs-only, gates G2+)
   └──> G2 Repository state model (GitRepository class, decorations, status bar)
           ├──> G3 Status panel + diff viewer ──> v0.6.0-beta
           │       └──> G4 Commit workflow (stage, message, push/pull) ──> v0.7.0-beta
           │               └──> G6 History + blame gutter ──> v0.8.0-beta ──> v1.0.0-rc1
           └──> G5 Branch management ──> v0.7.0-beta (parallel with G3→G4)
MR Multi-root workspaces (independent) ──> v0.9.0-beta ──> v1.0.0-rc1
```

## Slice list

| Slice | Title | Depends | Target |
|---|---|---|---|
| G1 | ADR-0016 Git backend | none | docs-only |
| G2-a | GitRepository state model | G1 | — |
| G2-b | Project-tree dirty decorations | G2-a | — |
| G2-c | Status-bar branch + dirty indicator | G2-a | — |
| G3-a | Git status panel | G2-a | — |
| G3-b | Inline diff viewer | G3-a | — |
| G3-c | v0.6.0-beta release cut | G3-b | v0.6.0-beta |
| G4-a | Stage / unstage hunks | G3-b | — |
| G4-b | Commit panel + commit action | G4-a | — |
| G4-c | Push / Pull / Fetch | G4-b | — |
| G4-d | v0.7.0-beta release cut | G4-c + G5-b | v0.7.0-beta |
| G5-a | Branch picker (switch + create) | G2-a | — |
| G5-b | Merge + delete branch | G5-a | — |
| G6-a | History log panel | G4-b | — |
| G6-b | Blame gutter in EditorSurface | G2-a | — |
| G6-c | Diff at historical commit | G6-a + G6-b | — |
| G6-d | v0.8.0-beta release cut | G6-c | v0.8.0-beta |
| MR-a | Multi-root workspace model | none | — |
| MR-b | UI: open/remove additional root | MR-a | — |
| MR-c | v0.9.0-beta release cut | MR-b | v0.9.0-beta |
| RC | v1.0.0-rc1 milestone close | G6-d + MR-c | v1.0.0-rc1 |

## Risk register

| Risk | Mitigation |
|---|---|
| libgit2 CMake bundling too heavy | Fall back to git-plumbing subprocess (ADR-0016 fallback path) |
| Arabic filenames corrupt in git ops | Test `core.quotePath=false` + UTF-8 locale; plumbing more reliable than porcelain |
| Conflict resolution UI scope creep | V3 shows conflict markers read-only; interactive 3-way merge is V4 |
| Multi-root project-tree performance | Lazy-load status per root; 5-second TTL cache |

## V3 completion criteria

V3 closes when all slices G1–RC and MR-a–MR-c are done OR explicitly
deferred. Milestone close cuts `v1.0.0-rc1`.

---
*Composed 2026-06-05. Vault implementation plan at `plans/lisan-studio/v3/`.*
