param(
    [string]$BashPath = "C:\msys64\usr\bin\bash.exe"
)

$ErrorActionPreference = "Stop"

function Convert-ToMsysPath {
    param([string]$WindowsPath)

    $resolved = (Resolve-Path -LiteralPath $WindowsPath).Path
    $drive = $resolved.Substring(0, 1).ToLowerInvariant()
    $rest = $resolved.Substring(2).Replace('\', '/')
    return "/$drive$rest"
}

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$bash = $BashPath
$rootUnix = Convert-ToMsysPath -WindowsPath $root

if (-not (Test-Path $bash)) {
    throw "MSYS2 bash not found: $bash"
}

& $bash -lc @"
set -euo pipefail
export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH
cd "$rootUnix"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
"@
