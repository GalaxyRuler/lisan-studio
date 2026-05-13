[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$releaseEvidenceScript = Join-Path $repoRoot 'scripts\release-evidence.ps1'

if (-not (Test-Path -LiteralPath $releaseEvidenceScript)) {
    throw "Missing release evidence script: $releaseEvidenceScript"
}

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($releaseEvidenceScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "release-evidence.ps1 parse failed: $($parseErrors[0].Message)"
}

$source = Get-Content -Raw -LiteralPath $releaseEvidenceScript

foreach ($requiredToken in @(
        '## Handoff Summary',
        '## Isolation And Runner Boundary',
        '## Automated Validation Completed',
        '## Manual QA Required',
        '## Deferred Features',
        'LisanStudio-QA',
        'active WHITEDRAGON',
        'scripts\beta-manual-check.ps1',
        'Manual QA is not complete until the installed-app checklist is recorded.',
        'Git UI',
        'AI panel',
        'auto-update',
        'plugin system'
    )) {
    if ($source -notmatch [regex]::Escape($requiredToken)) {
        throw "release-evidence.ps1 should include beta handoff token: $requiredToken"
    }
}

if ($source -match 'Manual QA complete') {
    throw 'release-evidence.ps1 must not claim manual QA is complete.'
}

"Test-ReleaseEvidenceHandoffSections.ps1 passed"
