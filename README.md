# Lisan Studio

Lisan Studio is a clean-room native Windows desktop IDE for editing and running
Arabic-first `.apy` files. It is not based on the earlier WPF/Emacs-host
prototype, not a browser shell, and not an Electron app.

## Product Direction

- Native C++17 / Qt 6 desktop application.
- `QPlainTextEdit` is the first editor core candidate.
- RTL-native Arabic-first UI with LTR islands for code, paths, commands, and output.
- Lisan Studio branding, logo resources, and premium dark visual system.
- Bundled Python runtime plus `lughat-althuban` for private beta packaging.
- WiX MSI is the first installer target.

## Local Build

Run from PowerShell:

```powershell
.\scripts\build.ps1
```

The script uses MSYS2 UCRT64 Qt/CMake/Ninja:

```text
C:\msys64\ucrt64
```

## Tests

```powershell
.\scripts\validate.ps1
```

The current gate builds the Qt app and runs:

- `acs_editor_tests`
- `acs_project_runtime_tests`
- `acs_main_window_tests`

## Packaging

```powershell
.\scripts\package.ps1 -ApythonRoot C:\Users\Admin\apython
```

The packager stages:

- `LisanStudio.exe`
- Qt runtime files via `windeployqt6`
- Python runtime under `runtime\python`
- `lughat-althuban` installed into the staged runtime
- license and release docs
- per-user install path under `%LOCALAPPDATA%\LisanStudio`
- `LisanStudio-0.1.0-beta.msi` under `artifacts\`

## Installed Smoke

After installing the MSI, run:

```powershell
.\scripts\installed-smoke.ps1
```

This validates the installed executable, bundled Python runtime, Arabic `.apy`
execution, command-line project/file launch, and non-editable runtime packaging.

## Beta Boundaries

Included in v0.1.0 beta:

- edit/save/open `.apy`, `.py`, `.md`, `.txt`
- minimal `.apy` syntax highlighting
- hidden BiDi control detection
- project tree
- custom RTL Claude-design top shell with integrated menus, primary run action, and unified command/search field
- no inherited `QMenuBar` or `QToolBar` shell surface
- RTL editor tabs, project sidebar, bottom panel, and status bar
- tabbed bottom panel for terminal, output, problems, search results, and debug
- project search with clickable file/line result rows
- Problems panel for hidden BiDi warnings and runtime failures
- run current `.apy`
- structured runtime output with exit code, elapsed time, and cancel action
- real RTL settings dialog for editor font, runtime diagnostics, and recent projects
- runtime diagnostics for bundled Python, `lughat-althuban`, run, lint, and format availability
- MSI packaging

Deferred:

- Git UI
- AI panel
- public distribution
- auto-update
- plugin system
- Emacs/JetBrains/Electron/web shells
