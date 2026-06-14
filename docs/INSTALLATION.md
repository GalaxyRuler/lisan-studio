# Lisan Studio Installation and Setup

This page covers public installation, local development setup, source builds,
packaging, validation, and troubleshooting.

## Install From A GitHub Release

1. Open <https://github.com/GalaxyRuler/lisan-studio/releases>.
2. Select the release you want to install.
3. Download the MSI asset, for example `LisanStudio-1.0.0-beta.msi`.
4. Run the MSI and follow the Windows Installer prompts.
5. Launch **Lisan Studio** from the Start Menu or desktop shortcut.

If the release has no MSI asset, that release is source-only. Build from source
or wait for the maintainer to attach a validated installer artifact.

The MSI installs per user under:

```text
%LOCALAPPDATA%\LisanStudio
```

The expected installed executable is:

```text
%LOCALAPPDATA%\LisanStudio\LisanStudio.exe
```

## First-Launch SmartScreen Warning

Lisan Studio installers are unsigned unless a release explicitly says
otherwise. On first launch, Windows may show a SmartScreen dialog saying that
Microsoft Defender SmartScreen prevented an unrecognized app from starting.

To proceed with an unsigned build:

1. Click **More info**.
2. Click **Run anyway**.

Managed Windows machines may block unsigned apps entirely. See
[ADR-0010](adr/0010-msi-code-signing.md) for the current code-signing decision
and revisit triggers.

## Uninstall

Use Windows Settings:

1. Open **Settings**.
2. Go to **Apps > Installed apps**.
3. Find **Lisan Studio**.
4. Choose **Uninstall**.

The MSI should remove the installed application payload and shortcuts. User
settings are not treated as MSI payload.

## Upgrade From Early ArabicCodeStudioQt Builds

If you installed an early build labeled `ArabicCodeStudioQt` or
`Arabic Code Studio Qt`, uninstall it manually before installing Lisan Studio.
Those early packages used a different product identity, so Windows Installer
does not treat them as the same app.

## Development Prerequisites

The default local build expects a Windows machine with:

- Windows 10 or Windows 11
- PowerShell 5.1 or PowerShell 7
- Git with submodule support
- MSYS2 UCRT64
- CMake 3.24 or newer
- Ninja
- GCC from MSYS2 UCRT64
- Qt 6 Widgets, Gui, Core, Test, and Concurrent

Install the common MSYS2 packages from an MSYS2 UCRT64 shell:

```bash
pacman -Syu
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-qt6-base
```

If `pacman -Syu` asks you to close and reopen the shell, do that first, then
run the package install command again.

The build scripts default to these tool paths:

```text
C:\msys64\usr\bin\bash.exe
C:\msys64\ucrt64\bin
```

If MSYS2 is installed somewhere else, pass the Bash path:

```powershell
.\scripts\build.ps1 -BashPath "D:\msys64\usr\bin\bash.exe"
```

## Build

From the repository root:

```powershell
git submodule update --init --recursive
.\scripts\build.ps1
```

The app binary is:

```text
build\LisanStudio.exe
```

## Test

Run the normal project validation gate:

```powershell
.\scripts\validate.ps1
```

This builds the app and runs the CTest suite in Qt offscreen mode.

## Packaging Prerequisites

Packaging adds these requirements beyond the normal build:

- WiX Toolset v7 with `wix.exe`
- A Python 3.13 runtime directory to bundle
- A local `lughat-althuban` source checkout with generated package metadata
- Qt license payloads from the MSYS2 Qt package

The packaging script accepts explicit paths:

```powershell
.\scripts\package.ps1 `
  -ProductVersion 1.0.0 `
  -ApythonRoot "<path-to-lughat-althuban>" `
  -PythonRoot "<path-to-python-3.13-runtime>" `
  -WixPath "<path-to-wix.exe>"
```

Or use environment variables:

```powershell
$env:LISAN_APYTHON_ROOT = "<path-to-lughat-althuban>"
$env:LISAN_PYTHON_ROOT = "<path-to-python-3.13-runtime>"
```

Output:

```text
artifacts\LisanStudio-<version>-beta.msi
stage\LisanStudio
```

To stage the application without creating the MSI:

```powershell
.\scripts\package.ps1 -SkipMsi
```

## Installed Smoke

After installing the MSI in an isolated Windows QA environment, validate the
installed application:

```powershell
.\scripts\installed-smoke.ps1
```

This checks the installed executable, bundled Python runtime, Arabic `.apy`
execution, UTF-8 output capture, command-line project/file launch, and runtime
packaging shape.

## MSI Smoke

MSI install, uninstall, and upgrade validation mutates the machine. Do not run
it on an active work desktop unless you explicitly intend to install or remove
the app there.

Install/uninstall smoke:

```powershell
.\scripts\msi-smoke.ps1
```

Upgrade smoke:

```powershell
.\scripts\msi-upgrade-smoke.ps1 `
  -EarlierMsiPath "<old-msi>" `
  -ReplacementMsiPath "<new-msi>" `
  -ExpectedEarlierVersion "<old-version>" `
  -ExpectedReplacementVersion "<new-version>" `
  -AllowMutation `
  -IUnderstandThisRunsMsiUpgrade
```

GitHub Actions workflow:

```powershell
gh workflow run msi-tests.yml -f scenario=install
gh workflow run msi-tests.yml -f scenario=upgrade
gh workflow run msi-tests.yml -f scenario=full
```

The workflow lives at `.github\workflows\msi-tests.yml` and expects a
self-hosted Windows runner with the Qt, WiX, Python, and packaging toolchain.

## Release Evidence

Generate a release evidence bundle in an isolated Windows QA environment:

```powershell
.\scripts\release-evidence.ps1 `
  -ProductVersion 1.0.0 `
  -ReleaseLabel "1.0.0-beta" `
  -ApythonRoot "<path-to-lughat-althuban>" `
  -PythonRoot "<path-to-python-3.13-runtime>"
```

Expected evidence:

```text
artifacts\release\<release-label>\VALIDATION_LOG.md
artifacts\release\<release-label>\CHECKSUMS-SHA256.txt
artifacts\release\<release-label>\KNOWN_ISSUES.md
artifacts\release\<release-label>\screenshots\main-window.png
```

## Manual QA Packet

Generate a manual QA packet from existing evidence:

```powershell
.\scripts\beta-manual-check.ps1 -ReleaseLabel "1.0.0-beta"
```

The script writes:

```text
artifacts\beta-manual-check\<run-id>\manual-beta-qa.docx
artifacts\beta-manual-check\<run-id>\manual-beta-qa.md
artifacts\beta-manual-check\<run-id>\manual-beta-qa.json
```

The Word document is the reviewer-facing checklist. The Markdown and JSON files
are traceability artifacts.

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

Treat this as a packaging failure. The MSI should use the bundled runtime under
`%LOCALAPPDATA%\LisanStudio\runtime\python`.

### Arabic output is mojibake

Treat this as a release blocker. `scripts\installed-smoke.ps1` should capture
and decode Arabic output as UTF-8.
