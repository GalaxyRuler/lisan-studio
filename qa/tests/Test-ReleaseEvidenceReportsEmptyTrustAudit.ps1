[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$scriptPath = Join-Path $repoRoot 'scripts\release-evidence.ps1'

if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "Missing release evidence script: $scriptPath"
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lisan-release-empty-trust-audit-test-" + [System.Guid]::NewGuid().ToString('N'))
$workspaceRoot = Join-Path $tempRoot 'workspace'
$tempAuditPath = Join-Path (Join-Path $tempRoot 'release') 'workspace-trust-audit.md'

try {
    New-Item -ItemType Directory -Force -Path $workspaceRoot | Out-Null

    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $scriptOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath `
        -TrustAuditOnly `
        -WorkspaceRoot $workspaceRoot `
        -WorkspaceTrustAuditPath $tempAuditPath 2>&1
    $scriptExitCode = $LASTEXITCODE
    $stopwatch.Stop()

    if ($scriptExitCode -ne 0) {
        throw "release-evidence.ps1 -TrustAuditOnly failed with exit code $scriptExitCode`n$($scriptOutput -join "`n")"
    }
    if ($stopwatch.Elapsed.TotalSeconds -gt 10) {
        throw "release-evidence.ps1 -TrustAuditOnly should finish quickly; elapsed seconds: $($stopwatch.Elapsed.TotalSeconds)"
    }
    if (-not (Test-Path -LiteralPath $tempAuditPath)) {
        throw 'Release evidence trust-audit-only mode should write workspace-trust-audit.md.'
    }

    $auditText = Get-Content -Raw -LiteralPath $tempAuditPath
    if ($auditText -notmatch '## Workspace Trust Audit') {
        throw 'Release evidence trust audit should include the Workspace Trust Audit header.'
    }
    if ($auditText -notmatch 'No workspace trust events recorded\.') {
        throw 'Release evidence trust audit should include the empty trust audit message.'
    }
    if ($auditText -match 'workspace\.trust\.granted') {
        throw 'Release evidence empty trust audit should not include a granted trust event.'
    }
    if ($auditText -match 'workspace\.trust\.revoked') {
        throw 'Release evidence empty trust audit should not include a revoked trust event.'
    }
} finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}

"Test-ReleaseEvidenceReportsEmptyTrustAudit.ps1 passed"
