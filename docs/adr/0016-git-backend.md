# ADR-0016 — Git backend

Status: Accepted
Date: 2026-06-05
Deciders: GalaxyRuler

## Context

V3 adds Git source-control UI to Lisan Studio (status panel, diff viewer,
stage/commit, branch management, history log, blame gutter). Every feature
requires querying and mutating Git repository state. Three backend options
exist for a Qt 6 / C++17 application on Windows.

## Options considered

### Option A — libgit2 (chosen)

libgit2 is a C89 library implementing the Git core methods. It runs
in-process with no QProcess overhead, exposes a stable C API usable from
C++ directly, and is the backing library for VS Code's git extension
(via nodegit), GitHub Desktop, and Kate's git-blame plugin.

Integration: CMake `FetchContent_Declare` pulling libgit2 from
https://github.com/libgit2/libgit2 at a pinned tag. Static link. Adds
~1 MB stripped to the MSI payload.

### Option B — git plumbing subprocess

Spawn `git` via `QProcess`, use low-level plumbing commands (`cat-file`,
`diff-index`, `ls-files`, `rev-parse`, etc.). Plumbing output format is
documented and stable across git versions. Zero new dependency.

### Option C — git porcelain subprocess

Same as B but using porcelain (`status`, `log`, `diff`). Output is
locale-dependent and designed for human consumption — fragile to parse.

## Decision

**Option A — libgit2.**

Rationale:
- Status-bar refresh and project-tree decorations poll on every file-save.
  QProcess spawn overhead (~50 ms per cold spawn on Windows) is
  unacceptable at that frequency. libgit2 in-process calls complete in
  microseconds.
- Hunk-level staging (G4-a) maps directly to libgit2's index API. The
  subprocess equivalent requires `git add -p` interactive mode, which is
  not scriptable without a pty.
- libgit2 handles Arabic filenames correctly regardless of locale.
  Subprocess plumbing requires `core.quotePath=false` and explicit UTF-8
  locale setup on the runner.

Fallback: if CMake FetchContent integration proves problematic within the
first G2-a slice, pivot to **Option B (git plumbing subprocess)**. Option
C (porcelain) is never the fallback.

## Consequences

- `CMakeLists.txt` gains a `FetchContent_Declare(libgit2 ...)` block and
  links `acs_core` against `libgit2::libgit2`.
- MSI payload grows by ~1 MB (libgit2 stripped static lib merged into the
  executable via CMake).
- CI: `libgit2` headers are available to all test binaries that include
  `src/GitRepository.h`.
- `src/GitRepository.{h,cpp}` is the only production file that calls
  libgit2 directly. All other source files access Git state through
  `GitRepository`'s public API.
- Test binaries that need a real git repo initialize one via
  `QProcess`-spawned `git init` + `git commit` - this is acceptable in
  tests because it runs once per test fixture, not per save event.

## References

- libgit2 docs: https://libgit2.org
- CMake FetchContent: https://cmake.org/cmake/help/latest/module/FetchContent.html
- Kate git-blame (reference integration): https://invent.kde.org/utilities/kate/-/tree/master/addons/git-blame
- Arabic filename handling: `git config core.quotePath false`
