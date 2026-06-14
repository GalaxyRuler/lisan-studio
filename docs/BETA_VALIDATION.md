# Lisan Studio Validation

This file describes the validation gates for installed Windows builds. It is
packaged into release artifacts so users and reviewers can see what a release is
expected to prove.

## Local Source Gate

Run from the repository root:

```powershell
.\scripts\validate.ps1
```

This configures the Release build, builds the app and tests, and runs CTest with
Qt in offscreen mode.

## Package Gate

Build a release candidate MSI:

```powershell
.\scripts\package.ps1 `
  -ProductVersion 1.0.0 `
  -ApythonRoot "<path-to-lughat-althuban>" `
  -PythonRoot "<path-to-python-3.13-runtime>"
```

The packaging gate verifies:

- `LisanStudio.exe` is staged.
- Qt runtime files are deployed.
- Python runtime is bundled under `runtime\python`.
- `lughat-althuban` and `debugpy` are copied into the staged runtime.
- Editable/local runtime markers are absent.
- License payloads are present.
- The MSI and signing-status evidence are written under `artifacts\`.

## Installed-App Gate

Run only in an isolated Windows QA environment or on a machine where installing
and uninstalling Lisan Studio is intended:

```powershell
.\scripts\installed-smoke.ps1
```

The installed smoke script verifies:

- the installed executable exists
- the installed executable is `LisanStudio.exe` under `%LOCALAPPDATA%\LisanStudio`
- the bundled Python runtime exists
- `lughat-althuban` runs a mixed Arabic/English `.apy` file
- Arabic output is decoded as UTF-8
- the app can launch with a project path
- the app can launch with a file path
- the bundled runtime has no editable local source markers

## MSI Gate

Run only in an isolated Windows QA environment or on a machine where MSI
mutation is intended:

```powershell
.\scripts\msi-smoke.ps1
```

The MSI smoke script verifies:

- silent MSI install
- installed payload under `%LOCALAPPDATA%\LisanStudio`
- Start Menu and Desktop shortcuts
- packaged README, release notes, validation notes, and license files
- Qt, Python, and `lughat-althuban` license payloads
- installed runtime smoke through `scripts\installed-smoke.ps1`
- silent MSI uninstall removes the app executable and shortcuts

Upgrade validation:

```powershell
.\scripts\msi-upgrade-smoke.ps1 `
  -EarlierMsiPath "<old-msi>" `
  -ReplacementMsiPath "<new-msi>" `
  -ExpectedEarlierVersion "<old-version>" `
  -ExpectedReplacementVersion "<new-version>" `
  -AllowMutation `
  -IUnderstandThisRunsMsiUpgrade
```

## GitHub Actions Gate

Maintainers can run the full MSI gate through a self-hosted Windows runner:

```powershell
gh workflow run msi-tests.yml -f scenario=full
```

The workflow lives at `.github\workflows\msi-tests.yml` and uploads MSI,
install, and upgrade evidence artifacts.

## Manual QA Gate

Use the installed app for these checks:

- launch from the Start Menu
- launch from the Desktop shortcut
- open `samples\torture-project`
- verify the shell, project sidebar, editor tabs, bottom panel, and status bar are RTL
- open, edit, save, close, and reopen a mixed Arabic/English file
- verify cursor movement across Arabic identifiers, English names, numbers,
  operators, and Windows paths
- verify selection, copy, paste, undo, redo, backspace, and delete near Arabic text
- search the project and open a result from the search-results tab
- insert a hidden BiDi control into a scratch file and confirm the Problems panel reports it
- run the current `.apy` file and confirm stdout/stderr, exit code, elapsed time,
  and cancel behavior are readable
- open Settings, verify runtime diagnostics, change the editor font setting, and reopen the app
- uninstall and confirm app payload files and shortcuts are removed
- reinstall without manual PATH or Python setup

Generate a manual QA packet from existing evidence:

```powershell
.\scripts\beta-manual-check.ps1 -ReleaseLabel "1.0.0-beta"
```

## Release Blockers

- cursor or selection corruption
- Arabic output mojibake
- save/open data loss
- installer requiring PATH or system Python setup
- MSI install/uninstall smoke failure
- missing Qt, Python, or `lughat-althuban` license payloads
- hidden BiDi controls inserted by the editor
- visible placeholder UI
- left-to-right shell regression in the top command bar, project/sidebar, tabs,
  status bar, or bottom panel
- crash on open, save, run, launch, or close
