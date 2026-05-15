$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cmakePath = Join-Path $repoRoot 'CMakeLists.txt'
$cmake = Get-Content -LiteralPath $cmakePath -Raw

if ($cmake -notmatch '(?s)qt_add_executable\s*\(\s*LisanStudio\s+WIN32\b') {
    throw 'LisanStudio must be declared as qt_add_executable(LisanStudio WIN32 ...) so normal app launch does not open a console window.'
}

$exePath = Join-Path $repoRoot 'build\LisanStudio.exe'
if (Test-Path -LiteralPath $exePath) {
    $bytes = [System.IO.File]::ReadAllBytes($exePath)
    if ($bytes.Length -lt 0x100) {
        throw "Built executable is too small to inspect: $exePath"
    }

    $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
    $optionalHeaderOffset = $peOffset + 24
    $magic = [BitConverter]::ToUInt16($bytes, $optionalHeaderOffset)
    if ($magic -ne 0x10B -and $magic -ne 0x20B) {
        throw "Unexpected PE optional-header magic 0x$($magic.ToString('X')) in $exePath"
    }

    $subsystemOffset = $optionalHeaderOffset + 0x44
    $subsystem = [BitConverter]::ToUInt16($bytes, $subsystemOffset)

    if ($subsystem -ne 2) {
        throw "Expected Windows GUI subsystem 2 for $exePath, got subsystem $subsystem."
    }
}

'Windows GUI subsystem metadata check passed.'
