[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$scriptPath = Join-Path $repoRoot 'scripts\collect-diagnostics.ps1'

if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "Missing diagnostics bundle script: $scriptPath"
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lisan-trust-audit-bundle-test-" + [System.Guid]::NewGuid().ToString('N'))
$workspaceRoot = Join-Path $tempRoot 'workspace'
$metadataRoot = Join-Path $workspaceRoot '.lisan-workspace'
$artifactRoot = Join-Path $tempRoot 'artifacts'
$outputRoot = Join-Path $tempRoot 'diagnostics'

try {
    New-Item -ItemType Directory -Force -Path $metadataRoot, $artifactRoot | Out-Null
    $auditPath = Join-Path $metadataRoot 'trust-audit.jsonl'
    @(
        ([ordered]@{ event = 'workspace.trust.granted'; projectRoot = $workspaceRoot; timestampUtc = '2026-05-19T01:02:03.000Z' } | ConvertTo-Json -Compress),
        ([ordered]@{ event = 'workspace.trust.revoked'; projectRoot = $workspaceRoot; timestampUtc = '2026-05-19T01:03:04.000Z' } | ConvertTo-Json -Compress)
    ) | Set-Content -LiteralPath $auditPath -Encoding UTF8

    $result = & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath `
        -ArtifactRoot $artifactRoot `
        -OutputRoot $outputRoot `
        -RunId 'trust-audit-present' `
        -WorkspaceRoot $workspaceRoot `
        -Json | ConvertFrom-Json

    if (-not $result.ok) {
        throw 'collect-diagnostics.ps1 should return ok=true when it can write a bundle.'
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $extractRoot = Join-Path $tempRoot 'extract'
    [System.IO.Compression.ZipFile]::ExtractToDirectory($result.bundlePath, $extractRoot)
    $auditBundlePath = Join-Path $extractRoot 'workspace-trust-audit.md'
    if (-not (Test-Path -LiteralPath $auditBundlePath)) {
        throw 'Diagnostics bundle should include workspace-trust-audit.md.'
    }

    $auditText = Get-Content -Raw -LiteralPath $auditBundlePath
    if ($auditText -notmatch '## Workspace Trust Audit') {
        throw 'Diagnostics bundle should include the Workspace Trust Audit header.'
    }
    if ($auditText -notmatch 'workspace\.trust\.granted') {
        throw 'Diagnostics bundle should include the granted trust event.'
    }
    if ($auditText -notmatch 'workspace\.trust\.revoked') {
        throw 'Diagnostics bundle should include the revoked trust event.'
    }
    if ($auditText -match 'No workspace trust events recorded\.') {
        throw 'Diagnostics bundle should not include the empty-state message when audit events exist.'
    }
} finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}

"Test-DiagnosticsBundleSurfacesTrustAudit.ps1 passed"
