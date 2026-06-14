# Lisan Studio

Native Windows IDE for Arabic-first `.apy` / Arabic Python development.
Built with C++17 and Qt 6. No Electron, browser shell, or JVM.

**Current source release:** [v1.0.0-rc1](https://github.com/GalaxyRuler/lisan-studio/releases/tag/v1.0.0-rc1)

> Public installer note: use the MSI attached to a GitHub Release when one is present. If a release tag has no MSI asset, build from source or ask the maintainer to publish the installer artifact for that release.

## What Lisan Studio Includes

- Arabic-first Qt desktop shell with RTL workbench layout.
- `.apy`, `.py`, `.md`, and text-file editing with line numbers, tabs, save/reload, and recovery for dirty untitled drafts.
- `.apy` syntax highlighting, hidden Unicode BiDi control warnings, bracket matching, indentation guides, visible whitespace, and trim-on-save.
- Multi-cursor editing: `Alt+Click`, `Alt+Drag` column selection, `Ctrl+D`, `Ctrl+Shift+L`, and cursor-above/cursor-below commands.
- Find/replace with regex, wrap, and all-match highlighting.
- Project tree, multi-root workspaces, recent projects, and workspace settings.
- Runtime execution for `.apy` files through a packaged Python / `lughat-althuban` runtime.
- LSP client support for completions, hover, go-to-definition, references, rename, outline, workspace symbols, and semantic tokens.
- Debug adapter support through `debugpy`, including breakpoints, run control, variables, watches, and call stack panels.
- Integrated terminal tab with project trust checks.
- Git source-control UI: status, stage/unstage, unified diff, commit, push, pull, fetch, branch management, history, blame, and historical diffs.
- Diagnostics, known-issue, release-evidence, packaging, and MSI smoke scripts for maintainers.

## Install

### Option 1: Install a Release MSI

1. Open [GitHub Releases](https://github.com/GalaxyRuler/lisan-studio/releases).
2. Download the MSI asset for the release, for example `LisanStudio-1.0.0-beta.msi`.
3. Run the MSI and follow the Windows Installer prompts.
4. Launch **Lisan Studio** from the Start Menu or desktop shortcut.

The MSI installs per user under:

```text
%LOCALAPPDATA%\LisanStudio
```

The expected executable is:

```text
%LOCALAPPDATA%\LisanStudio\LisanStudio.exe
```

Lisan Studio installers are currently unsigned unless the release notes say otherwise. Windows SmartScreen may show a warning on first launch; see [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) and [ADR-0010](docs/adr/0010-msi-code-signing.md).

### Option 2: Build From Source

Clone with submodules:

```powershell
git clone --recurse-submodules https://github.com/GalaxyRuler/lisan-studio.git
cd lisan-studio
```

For an existing checkout:

```powershell
git submodule update --init --recursive
```

Install the development prerequisites listed in [docs/INSTALLATION.md](docs/INSTALLATION.md), then build:

```powershell
.\scripts\build.ps1
```

The built executable is:

```text
build\LisanStudio.exe
```

## First Use

1. Open Lisan Studio.
2. Open a folder that contains `.apy` files, or create a new `.apy` file.
3. Write Arabic Python code in the editor.
4. Use **Run Current File** to execute the current `.apy` file.
5. Use the bottom panel for output, problems, search results, terminal, and debug sessions.
6. Use the Source Control panel for Git status, diffs, staging, commits, branches, blame, and history.

Sample files live under:

```text
samples\torture-project
```

## Development

Normal validation:

```powershell
.\scripts\validate.ps1
```

This configures the Release build, builds the app and tests, then runs CTest with Qt in offscreen mode.

Package an MSI for release validation:

```powershell
.\scripts\package.ps1 -ProductVersion 1.0.0 -ApythonRoot "<path-to-lughat-althuban>" -PythonRoot "<path-to-python-3.13-runtime>"
```

Maintainers can set these environment variables instead of passing parameters:

```powershell
$env:LISAN_APYTHON_ROOT = "<path-to-lughat-althuban>"
$env:LISAN_PYTHON_ROOT = "<path-to-python-3.13-runtime>"
```

More setup, packaging, and troubleshooting detail: [docs/INSTALLATION.md](docs/INSTALLATION.md).

## Validation And Public Readiness

Public release checks are tracked in [docs/PUBLIC_RELEASE_READINESS.md](docs/PUBLIC_RELEASE_READINESS.md).

Local checks used before publishing source changes:

```powershell
.\scripts\validate.ps1
```

MSI install, uninstall, upgrade, screenshot, and installed-app validation should run only in an isolated Windows QA environment, not on an active work desktop. The GitHub workflow is:

```powershell
gh workflow run msi-tests.yml -f scenario=full
```

## Roadmap

| Milestone | Status | Release |
|---|---|---|
| V1.5 - Multi-cursor + MSI pipeline | shipped | v0.2.0-beta |
| V2 - LSP + debugger + terminal | shipped | v0.5.0-beta |
| V3 - Git UI + multi-root workspaces | shipped | v1.0.0-rc1 |
| V4 - 3-way merge, PR review, extensions | planned | - |

Roadmaps: [docs/ROADMAP-V2.md](docs/ROADMAP-V2.md), [docs/ROADMAP-V3.md](docs/ROADMAP-V3.md).

## Security

Do not commit `.env*`, private keys, certificates, tokens, signing material, or private runtime credentials. Report vulnerability concerns using [SECURITY.md](SECURITY.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup, branch guidance, validation expectations, and release rules.
