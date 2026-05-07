$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$bash = "C:\msys64\usr\bin\bash.exe"

if (-not (Test-Path $bash)) {
    throw "MSYS2 bash not found: $bash"
}

& $bash -lc @"
set -euo pipefail
export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH
cd /c/Users/Admin/arabic-code-studio-qt
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
"@

