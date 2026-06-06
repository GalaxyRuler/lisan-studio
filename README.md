# Lisan Studio

Native Windows IDE for Arabic-first `.apy` / Arabic Python programming.
Built with C++17 and Qt 6. No Electron. No browser shell. No JVM.

**Current release: [v1.0.0-rc1](https://github.com/GalaxyRuler/lisan-studio/releases/tag/v1.0.0-rc1)**

---

## What it is

- Qt 6 / C++17 desktop application. Single process. Native Win32 UI.
- `QPlainTextEdit` editor core — RTL-native, Arabic-first, LTR islands for code/paths/output.
- `.apy` syntax highlighting, hidden BiDi control detection, multi-cursor editing.
- Bundled Python runtime + `lughat-althuban` for standalone packaging.
- WiX MSI installer (per-user, no elevation required).

## Features

### Editor
- Open / edit / save `.apy`, `.py`, `.md`, `.txt`
- Arabic-first RTL layout; LTR islands for code, paths, commands
- `.apy` syntax highlighting (ApyHighlighter)
- Hidden BiDi control detection with Problems panel warnings
- Multi-cursor editing: `Alt+Click`, `Alt+Drag` column selection, `Ctrl+D` next match, `Ctrl+Shift+L` all matches, `Alt+↑/↓` cursor above/below
- Bracket matching, indentation guides, visible whitespace toggle
- Trim trailing whitespace on save
- Find/replace with regex, wrap, all-matches highlight
- Untitled draft recovery on crash
- Line numbers, breakpoint gutter

### Language Server (LSP)
- LSP client (`LspClient`) wired to `pylsp` via Arabic→Python translation shim
- Completions popup, hover documentation (Markdown rendered), go-to-definition
- Semantic token highlighting

### Debugger
- DAP client (`DapClient`) backed by `debugpy`
- Breakpoints set from editor gutter
- Debug run with attach handshake

### Terminal
- Integrated terminal via `QTermWidget` + ConPTY
- Full PTY — interactive REPL, pip, git, etc.

### Git source control
- Status panel: changed files list with per-file stage/unstage
- Inline diff viewer (unified)
- Commit workflow: message panel, commit, push, pull, fetch
- Branch management: create, switch, merge, delete (GitBranchPicker)
- History log panel (commit list + metadata)
- Blame gutter in editor (per-line annotation)
- Historical commit diff viewer
- Backend: libgit2 via CMake FetchContent (ADR-0016); Arabic filenames handled correctly regardless of locale

### Workspace
- Project tree with file/folder open, right-click context menu
- Multi-root workspaces: add/remove additional project roots, per-root tree browsing, persisted root list
- Project-tree dirty/clean Git decorations
- Status bar: current branch + dirty indicator

### Shell
- Custom RTL top shell (no inherited QMenuBar/QToolBar)
- Tabbed bottom panel: Terminal, Output, Problems, Search Results, Debug
- RTL settings dialog: editor font, runtime diagnostics, recent projects
- Runtime diagnostics: bundled Python, lughat-althuban, run/lint/format

---

## Local build

Clone with submodules:

```powershell
git clone --recurse-submodules https://github.com/GalaxyRuler/lisan-studio
```

Existing checkout:

```powershell
git submodule update --init --recursive
```

Build (requires MSYS2 UCRT64 at `C:\msys64\ucrt64`):

```powershell
.\scripts\build.ps1
```

Full setup, dev environment, and troubleshooting: `docs\INSTALLATION.md`

---

## Tests

```powershell
.\scripts\validate.ps1
```

Runs all test suites:

| Suite | Covers |
|---|---|
| `acs_editor_tests` | EditorSurface core |
| `acs_editor_tabs_controller_tests` | Tab management |
| `acs_project_tree_controller_tests` | Project tree |
| `acs_command_registry_tests` | Command registry |
| `acs_workbench_state_tests` | Workbench state |
| `acs_project_runtime_tests` | Runtime execution |
| `acs_runtime_orchestrator_tests` | Orchestration |
| `acs_main_window_tests` | Main window integration |
| `acs_editor_torture_tests` | Multi-cursor stress |
| `acs_untitled_draft_recovery_tests` | Crash recovery |
| `acs_lsp_client_tests` | LSP client |
| `acs_git_repository_tests` | GitRepository state model |
| `acs_git_status_panel_tests` | Status panel |
| `acs_git_commit_workflow_tests` | Stage/commit/push |

### MSI install + upgrade testing

Runs on self-hosted GitHub Actions runner (`lisanstudio-qa` Hyper-V VM).

```powershell
gh workflow run msi-tests.yml -f scenario=install
gh workflow run msi-tests.yml -f scenario=upgrade
gh workflow run msi-tests.yml -f scenario=full
```

Evidence artifacts (MSI, install logs, registry diffs) upload to the GHA run.

---

## Packaging

```powershell
.\scripts\package.ps1 -ApythonRoot C:\Users\Admin\apython
```

Stages:
- `LisanStudio.exe` + Qt runtime (via `windeployqt6`)
- Python runtime under `runtime\python`
- `lughat-althuban` installed into staged runtime
- libgit2 (statically linked — no extra DLL)
- License and release docs, third-party licenses under `licenses\`
- Per-user install path: `%LOCALAPPDATA%\LisanStudio`
- Output: `artifacts\LisanStudio-1.0.0-beta.msi`

### Release evidence

```powershell
.\scripts\release-evidence.ps1
```

Runs packaging + MSI smoke, writes SHA256 checksums, validation log, known issues, workspace trust audit, and screenshot under `artifacts\release\`.

### Installed smoke

```powershell
.\scripts\installed-smoke.ps1   # validate installed exe + runtime
.\scripts\msi-smoke.ps1         # full silent install → smoke → uninstall
```

---

## Signing

Unsigned — SmartScreen will warn on first run. See `docs/adr/0010-msi-code-signing.md` for the decision record and user-facing guidance. Signing is deferred to a future milestone.

---

## Roadmap

| Milestone | Status | Release |
|---|---|---|
| V1.5 — Multi-cursor + MSI pipeline | ✅ shipped | v0.2.0-beta |
| V2 — LSP + debugger + terminal | ✅ shipped | v0.5.0-beta |
| V3 — Git UI + multi-root workspaces | ✅ shipped | v1.0.0-rc1 |
| V4 — 3-way merge, PR review, extensions | planned | — |

Full specs: `docs/ROADMAP-V2.md`, `docs/ROADMAP-V3.md`

---

## Contributing

See `CONTRIBUTING.md` for the role-split execution model, branch conventions, slice dispatch protocol, and ADR process.
