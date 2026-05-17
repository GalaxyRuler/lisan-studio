[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$packageScript = Join-Path $repoRoot 'scripts\package.ps1'
$releaseEvidenceScript = Join-Path $repoRoot 'scripts\release-evidence.ps1'

foreach ($script in @($packageScript, $releaseEvidenceScript)) {
    if (-not (Test-Path -LiteralPath $script)) {
        throw "Missing script: $script"
    }

    $tokens = $null
    $parseErrors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile($script, [ref]$tokens, [ref]$parseErrors)
    if ($parseErrors -and $parseErrors.Count -gt 0) {
        throw "$script parse failed: $($parseErrors[0].Message)"
    }
}

$packageSource = Get-Content -Raw -LiteralPath $packageScript
$releaseSource = Get-Content -Raw -LiteralPath $releaseEvidenceScript

foreach ($requiredToken in @(
        'function Write-SigningStatus',
        'SIGNING_STATUS.txt',
        'Get-AuthenticodeSignature',
        'AuthenticodeStatus',
        'Public distribution requires a valid Authenticode signature.'
    )) {
    if ($packageSource -notmatch [regex]::Escape($requiredToken)) {
        throw "package.ps1 should record signing evidence token: $requiredToken"
    }
}

foreach ($requiredToken in @(
        'SIGNING_STATUS.txt',
        '$signingStatusPath',
        'Signing status',
        'SigningStatus'
    )) {
    if ($releaseSource -notmatch [regex]::Escape($requiredToken)) {
        throw "release-evidence.ps1 should include signing evidence token: $requiredToken"
    }
}

"Test-DigitalSignaturePolicy.ps1 passed"
