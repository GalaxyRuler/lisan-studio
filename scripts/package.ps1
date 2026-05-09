param(
    [string]$ApythonRoot = "C:\Users\Admin\apython",
    [string]$PythonRoot = "C:\Users\Admin\AppData\Local\Programs\Python\Python313",
    [string]$Configuration = "Release",
    [string]$ProductVersion = "0.1.0",
    [switch]$SkipMsi
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$stage = Join-Path $repo "stage"
$artifacts = Join-Path $repo "artifacts"
$build = Join-Path $repo "build"
$bash = "C:\msys64\usr\bin\bash.exe"
$windeployqt = "C:\msys64\ucrt64\bin\windeployqt6.exe"
$wix = "C:\Program Files\WiX Toolset v7.0\bin\wix.exe"
$wxs = Join-Path $repo "packaging\wix\LisanStudio.wxs"
$qtLicenseRoot = "C:\msys64\ucrt64\share\licenses\qt6-base"

if (-not (Test-Path $bash)) { throw "MSYS2 bash not found: $bash" }
if (-not (Test-Path $windeployqt)) { throw "windeployqt6 not found: $windeployqt" }
if (-not (Test-Path $ApythonRoot)) { throw "ApythonRoot not found: $ApythonRoot" }
if (-not (Test-Path $PythonRoot)) { throw "PythonRoot not found: $PythonRoot" }
if (-not (Test-Path $qtLicenseRoot)) { throw "Qt license folder not found: $qtLicenseRoot" }

& (Join-Path $PSScriptRoot "validate.ps1")

if (Test-Path $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
if (Test-Path $artifacts) {
    New-Item -ItemType Directory -Force -Path $artifacts | Out-Null
} else {
    New-Item -ItemType Directory -Force -Path $artifacts | Out-Null
}
New-Item -ItemType Directory -Force -Path $stage | Out-Null

& $bash -lc @"
set -euo pipefail
export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH
cd /c/Users/Admin/arabic-code-studio-qt
cmake --install build --prefix stage
"@

& $windeployqt --release --no-translations (Join-Path $stage "LisanStudio.exe")

$runtimeRoot = Join-Path $stage "runtime\python"
Copy-Item -Path $PythonRoot -Destination $runtimeRoot -Recurse -Force
$python = Join-Path $runtimeRoot "python.exe"
if (-not (Test-Path $python)) {
    throw "Staged Python executable missing: $python"
}

$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"
$env:PYTHONNOUSERSITE = "1"

$sitePackages = Join-Path $runtimeRoot "Lib\site-packages"
if (Test-Path $sitePackages) {
    Get-ChildItem -LiteralPath $sitePackages -Force |
        Where-Object {
            $_.Name -notmatch '^(pip|setuptools|wheel|pkg_resources)(-|$)' -and
            $_.Name -ne "_distutils_hack" -and
            $_.Name -ne "distutils-precedence.pth"
        } |
        Remove-Item -Recurse -Force
}

& $python -m pip install --upgrade pip wheel setuptools

$wheelhouse = Join-Path $stage "runtime\wheelhouse"
New-Item -ItemType Directory -Force -Path $wheelhouse | Out-Null
& $python -m pip wheel --no-deps --wheel-dir $wheelhouse $ApythonRoot
& $python -m pip install --no-deps --no-index --find-links $wheelhouse lughat-althuban

$editableMarkers = Get-ChildItem -LiteralPath $sitePackages -Force -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -like "__editable__*" -or $_.Name -like "apython-*.dist-info" }
if ($editableMarkers) {
    throw "Staged runtime still contains editable/local apython markers."
}

& $python -c "import importlib.metadata as md, importlib.util; assert importlib.util.find_spec('arabicpython'); print('lughat-althuban', md.version('lughat-althuban'))"

Copy-Item -LiteralPath (Join-Path $repo "README.md") -Destination (Join-Path $stage "README.md") -Force
Copy-Item -LiteralPath (Join-Path $repo "docs\RELEASE_NOTES.md") -Destination (Join-Path $stage "RELEASE_NOTES.md") -Force
Copy-Item -LiteralPath (Join-Path $repo "docs\BETA_VALIDATION.md") -Destination (Join-Path $stage "BETA_VALIDATION.md") -Force
Copy-Item -LiteralPath (Join-Path $repo "licenses\LICENSES.md") -Destination (Join-Path $stage "LICENSES.md") -Force
Copy-Item -LiteralPath (Join-Path $repo "samples") -Destination (Join-Path $stage "samples") -Recurse -Force

$stageLicenses = Join-Path $stage "licenses"
New-Item -ItemType Directory -Force -Path $stageLicenses | Out-Null
Copy-Item -LiteralPath $qtLicenseRoot -Destination (Join-Path $stageLicenses "qt6-base") -Recurse -Force
Copy-Item -LiteralPath (Join-Path $PythonRoot "LICENSE.txt") -Destination (Join-Path $stageLicenses "Python-LICENSE.txt") -Force
Copy-Item -LiteralPath (Join-Path $ApythonRoot "LICENSE") -Destination (Join-Path $stageLicenses "lughat-althuban-LICENSE") -Force

if (-not $SkipMsi) {
    if (-not (Test-Path $wix)) {
        throw "WiX wix.exe not found: $wix"
    }
    & $wix build `
        -acceptEula wix7 `
        $wxs `
        -define "ProductVersion=$ProductVersion" `
        -define "SourceDir=$stage" `
        -out (Join-Path $artifacts "LisanStudio-0.1.0-beta.msi")
}

Get-ChildItem $artifacts -File -ErrorAction SilentlyContinue | Get-FileHash -Algorithm SHA256 |
    Format-Table -AutoSize
