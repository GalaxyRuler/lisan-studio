[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$scriptPath = Join-Path $repoRoot 'scripts\collect-diagnostics.ps1'

if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "Missing diagnostics bundle script: $scriptPath"
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lisan-empty-trust-audit-bundle-test-" + [System.Guid]::NewGuid().ToString('N'))
$workspaceRoot = Join-Path $tempRoot 'workspace'
$artifactRoot = Join-Path $tempRoot 'artifacts'
$outputRoot = Join-Path $tempRoot 'diagnostics'

try {
    New-Item -ItemType Directory -Force -Path $workspaceRoot, $artifactRoot | Out-Null

    $result = & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath `
        -ArtifactRoot $artifactRoot `
        -OutputRoot $outputRoot `
        -RunId 'trust-audit-empty' `
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
    if ($auditText -notmatch 'No workspace trust events recorded\.') {
        throw 'Diagnostics bundle should include the empty trust audit message.'
    }
} finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}

"Test-DiagnosticsBundleReportsEmptyTrustAudit.ps1 passed"
