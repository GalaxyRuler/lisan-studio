# Lisan Studio Installation and Setup

This page covers installing the private beta, setting up a local development
environment, and running the project-owned packaging and validation workflows.

## Private Beta Install

Use the MSI from the private beta handoff package:

```text
LisanStudio-0.1.0-beta.msi
```

For the completed 0.1.0-beta handoff, the reviewer package was assembled as:

```text
LisanStudio-0.1.0-beta-handoff-20260515T184105.zip
```

Inside that package, start with:

```text
README.txt
manual-qa\manual-beta-qa.docx
00-INSTALLER-CLICK-HERE\LisanStudio-0.1.0-beta.msi
```

Install steps for a beta reviewer:

1. Extract the handoff zip to a local folder.
2. Open `manual-qa\manual-beta-qa.docx` and read the current decision/status.
3. Double-click `00-INSTALLER-CLICK-HERE\LisanStudio-0.1.0-beta.msi`.
4. Follow the Windows Installer prompts.
5. Launch Lisan Studio from the Start Menu or Desktop shortcut.
6. Use the manual QA checklist if this is a validation pass.

The MSI installs per user under:

```text
%LOCALAPPDATA%\LisanStudio
```

The expected installed executable is:

```text
%LOCALAPPDATA%\LisanStudio\LisanStudio.exe
```

## Uninstall

Use Windows Settings:

1. Open **Settings**.
2. Go to **Apps > Installed apps**.
3. Find **Lisan Studio**.
4. Choose **Uninstall**.

The MSI is expected to remove the installed application payload and shortcuts.
User-level Qt settings are not treated as MSI payload.

## Reinstall or Upgrade the Same Beta

The 0.1.0-beta MSI supports same-version beta reinstall/replace during private
QA. If a previous beta install exists, uninstall it first when doing manual QA,
then reinstall from the handoff MSI.

For automated MSI validation, use the project-owned smoke script in an isolated
runner or approved VM, not on the active desktop:

```powershell
.\scripts\msi-smoke.ps1
```

## Runtime Included in the MSI

The private beta MSI is designed to include the runtime pieces needed to run
Arabic `.apy` files without asking the reviewer to configure Python manually:

- `LisanStudio.exe`
- Qt runtime files from `windeployqt6`
- bundled Python runtime under `runtime\python`
- `lughat-althuban` package files
- license payloads for Qt, Python, and `lughat-althuban`
- release notes and validation notes

If the installed app requires a system Python install, manual `PATH` edits, or a
local editable `apython` checkout, treat that as a beta blocker.

## Development Prerequisites

The default local build expects a Windows machine with MSYS2 UCRT64 tooling:

```text
C:\msys64\usr\bin\bash.exe
C:\msys64\ucrt64\bin
```

Required development tools:

- Windows 11 or Windows 10
- PowerShell 5.1 or PowerShell 7
- MSYS2 UCRT64
- CMake 3.24 or newer
- Ninja
- GCC from MSYS2 UCRT64
- Qt 6 Widgets, Gui, Core, Test, and Concurrent

The build script assumes MSYS2 at `C:\msys64`:

```powershell
.\scripts\build.ps1
```

If MSYS2 is installed somewhere else, pass the Bash path:

```powershell
.\scripts\build.ps1 -BashPath "D:\msys64\usr\bin\bash.exe"
```

## Suggested MSYS2 Package Setup

Open an MSYS2 UCRT64 shell and install the expected toolchain packages:

```bash
pacman -Syu
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-qt6-base
```

If `pacman -Syu` asks you to close and reopen the shell, do that first, then run
the package install command again.

## Build

From the repository root:

```powershell
.\scripts\build.ps1
```

This configures and builds:

```text
build\LisanStudio.exe
build\acs_editor_tests.exe
build\acs_project_runtime_tests.exe
build\acs_main_window_tests.exe
```

## Test

Run the normal project validation gate:

```powershell
.\scripts\validate.ps1
```

This builds the app and runs:

- `acs_editor_tests`
- `acs_project_runtime_tests`
- `acs_main_window_tests`

GUI-sensitive validation should run in an interactive desktop session. Do not
weaken GUI-sensitive tests to make a non-interactive runner pass.

## Packaging Prerequisites

Packaging adds these requirements beyond the normal build:

- WiX Toolset v7 with `wix.exe` available at:

  ```text
  C:\Program Files\WiX Toolset v7.0\bin\wix.exe
  ```

- Python 3.13 runtime source at:

  ```text
  C:\Users\Admin\AppData\Local\Programs\Python\Python313
  ```

- `apython` / `lughat-althuban` source checkout at:

  ```text
  C:\Users\Admin\apython
  ```

- Qt license payloads under:

  ```text
  C:\msys64\ucrt64\share\licenses\qt6-base
  ```

These paths can be overridden with script parameters when needed.

## Package the MSI

From the repository root:

```powershell
.\scripts\package.ps1
```

Common override:

```powershell
.\scripts\package.ps1 -ApythonRoot "C:\Users\Admin\apython"
```

Output:

```text
artifacts\LisanStudio-0.1.0-beta.msi
stage\LisanStudio
```

To stage the application without creating the MSI:

```powershell
.\scripts\package.ps1 -SkipMsi
```

## Installed Smoke

After installing the MSI, validate the installed application:

```powershell
.\scripts\installed-smoke.ps1
```

This checks the installed executable, bundled Python runtime, Arabic `.apy`
execution, UTF-8 output capture, command-line project/file launch, and runtime
packaging shape.

## MSI Smoke

To validate install and uninstall behavior:

```powershell
.\scripts\msi-smoke.ps1
```

To leave the app installed after the smoke pass:

```powershell
.\scripts\msi-smoke.ps1 -KeepInstalled
```

Do not run MSI install/uninstall validation on an active work desktop unless
that is explicitly approved for the current run.

## MSI Upgrade Smoke

Upgrade validation is also MSI-mutating and belongs in `LisanStudio-QA`, not on
active `WHITEDRAGON`. The project-owned scripts cover:

- same-version private beta replace leaving exactly one Windows Apps uninstall
  entry;
- older-version to replacement-version upgrade;
- optional downgrade refusal that keeps the replacement install active.

The scenario wrappers are:

```powershell
.\qa\vm\Test-MsiSameVersionReplaceLeavesRegistryClean.ps1
.\qa\vm\Test-MsiUpgradeReplacesEarlierVersion.ps1
```

Both wrappers call `scripts\msi-upgrade-smoke.ps1` and require explicit
mutation switches from the approved VM lane.

## Release Evidence

Generate the private beta evidence bundle:

```powershell
.\scripts\release-evidence.ps1
```

Expected evidence:

```text
artifacts\release\0.1.0-beta\VALIDATION_LOG.md
artifacts\release\0.1.0-beta\CHECKSUMS-SHA256.txt
artifacts\release\0.1.0-beta\KNOWN_ISSUES.md
artifacts\release\0.1.0-beta\screenshots\main-window.png
```

## Manual QA Packet

Generate a manual QA packet from existing evidence:

```powershell
.\scripts\beta-manual-check.ps1
```

The script writes:

```text
artifacts\beta-manual-check\<run-id>\manual-beta-qa.docx
artifacts\beta-manual-check\<run-id>\manual-beta-qa.md
artifacts\beta-manual-check\<run-id>\manual-beta-qa.json
```

Use the Word document as the reviewer-facing checklist. The Markdown and JSON
files are traceability artifacts.

## Homelab and VM Validation

The project is integrated with the central Codex Homelab runner metadata:

```text
.codex\homelab-runner.json
qa\homelab\
qa\vm\
```

For MSI, GUI, installed-app, screenshot, or release-evidence validation, use the
project VM route:

```text
LisanStudio-QA
```

Do not run GUI automation, MSI install/uninstall, installed-app validation, or
destructive validation on active `WHITEDRAGON`.

Safe local work includes:

- reading metadata
- parsing JSON and PowerShell
- small project-local QA tests
- generating docs from already-produced evidence

Unsafe local work without explicit approval includes:

- installing or uninstalling the MSI on active `WHITEDRAGON`
- running GUI automation on active `WHITEDRAGON`
- mutating Hyper-V outside the Homelab approval lane
- deleting VMs or runner artifacts

## Troubleshooting

### `C:\msys64\usr\bin\bash.exe` not found

Install MSYS2 or pass the correct Bash path:

```powershell
.\scripts\build.ps1 -BashPath "D:\msys64\usr\bin\bash.exe"
```

### CMake cannot find Qt 6

Confirm the UCRT64 Qt package is installed and that the build script is using
the UCRT64 path first:

```text
/ucrt64/bin
```

### `windeployqt6.exe` not found

Confirm Qt tools exist at:

```text
C:\msys64\ucrt64\bin\windeployqt6.exe
```

Or pass:

```powershell
.\scripts\package.ps1 -WindeployQtPath "D:\msys64\ucrt64\bin\windeployqt6.exe"
```

### `wix.exe` not found

Install WiX Toolset v7 or pass the path:

```powershell
.\scripts\package.ps1 -WixPath "C:\Program Files\WiX Toolset v7.0\bin\wix.exe"
```

### Installed app asks for system Python

Treat this as a packaging failure. The beta MSI should use the bundled runtime
under `%LOCALAPPDATA%\LisanStudio\runtime\python`.

### Arabic output is mojibake

Treat this as a beta blocker. `scripts\installed-smoke.ps1` should capture and
decode Arabic output as UTF-8.
