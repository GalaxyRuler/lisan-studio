[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$scriptPath = Join-Path $repoRoot 'scripts\release-evidence.ps1'

if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "Missing release evidence script: $scriptPath"
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lisan-release-trust-audit-test-" + [System.Guid]::NewGuid().ToString('N'))
$workspaceRoot = Join-Path $tempRoot 'workspace'
$metadataRoot = Join-Path $workspaceRoot '.lisan-workspace'
$tempAuditPath = Join-Path (Join-Path $tempRoot 'release') 'workspace-trust-audit.md'

try {
    New-Item -ItemType Directory -Force -Path $metadataRoot | Out-Null
    $auditPath = Join-Path $metadataRoot 'trust-audit.jsonl'
    @(
        ([ordered]@{ event = 'workspace.trust.granted'; projectRoot = $workspaceRoot; timestampUtc = '2026-05-19T01:02:03.000Z' } | ConvertTo-Json -Compress),
        ([ordered]@{ event = 'workspace.trust.revoked'; projectRoot = $workspaceRoot; timestampUtc = '2026-05-19T01:03:04.000Z' } | ConvertTo-Json -Compress)
    ) | Set-Content -LiteralPath $auditPath -Encoding UTF8

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
    if ($auditText -notmatch 'workspace\.trust\.granted') {
        throw 'Release evidence trust audit should include the granted trust event.'
    }
    if ($auditText -notmatch 'workspace\.trust\.revoked') {
        throw 'Release evidence trust audit should include the revoked trust event.'
    }
    if ($auditText -match 'No workspace trust events recorded\.') {
        throw 'Release evidence trust audit should not include the empty-state message when audit events exist.'
    }
} finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}

"Test-ReleaseEvidenceSurfacesTrustAudit.ps1 passed"
