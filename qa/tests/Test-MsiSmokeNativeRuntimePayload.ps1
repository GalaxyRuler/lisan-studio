[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$msiSmokeScript = Join-Path $repoRoot 'scripts\msi-smoke.ps1'

if (-not (Test-Path -LiteralPath $msiSmokeScript)) {
    throw "Missing MSI smoke script: $msiSmokeScript"
}

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($msiSmokeScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "msi-smoke.ps1 parse failed: $($parseErrors[0].Message)"
}

$source = Get-Content -Raw -LiteralPath $msiSmokeScript

foreach ($requiredToken in @(
        'libgcc_s_seh-1.dll',
        'libstdc++-6.dll',
        'libwinpthread-1.dll',
        'libicuin78.dll',
        'libicuuc78.dll',
        'libicudt78.dll',
        'Installed payload file missing'
    )) {
    if ($source -notmatch [regex]::Escape($requiredToken)) {
        throw "msi-smoke.ps1 should require native runtime payload token: $requiredToken"
    }
}

"Test-MsiSmokeNativeRuntimePayload.ps1 passed"
