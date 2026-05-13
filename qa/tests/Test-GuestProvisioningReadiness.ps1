[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$provisioningScript = Join-Path $repoRoot 'qa\vm\Install-LisanStudioQaGuest.ps1'

if (-not (Test-Path -LiteralPath $provisioningScript)) {
    throw "Missing guest provisioning script: $provisioningScript"
}

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($provisioningScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "Install-LisanStudioQaGuest.ps1 parse failed: $($parseErrors[0].Message)"
}

$source = Get-Content -Raw -LiteralPath $provisioningScript

if ($source -match '\$toolingOk\s*=\s*\(@\(\$installFailures\)\.Count\s+-eq\s+0\)\s+-and\s+\(@\(\$missingRequired\)\.Count\s+-eq\s+0\)') {
    throw 'Guest readiness should not fail only because an install command failed after required tools are already present.'
}

if ($source -notmatch '\$toolingOk\s*=\s*\(@\(\$missingRequired\)\.Count\s+-eq\s+0\)') {
    throw 'Guest readiness should be based on required tool discovery.'
}

if ($source -notmatch 'readinessWarnings\s*=\s*@\(\$readinessWarnings\)') {
    throw 'Install failures should remain visible as readiness warnings.'
}

"Test-GuestProvisioningReadiness.ps1 passed"
