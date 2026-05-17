[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$scriptPath = Join-Path $repoRoot 'scripts\collect-diagnostics.ps1'

if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "Missing diagnostics bundle script: $scriptPath"
}

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "collect-diagnostics.ps1 parse failed: $($parseErrors[0].Message)"
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lisan-diagnostics-test-" + [System.Guid]::NewGuid().ToString('N'))
$artifactRoot = Join-Path $tempRoot 'artifacts'
$releaseLogDir = Join-Path $artifactRoot 'release\logs'
$outputRoot = Join-Path $tempRoot 'diagnostics'

try {
    New-Item -ItemType Directory -Force -Path $releaseLogDir | Out-Null
    Set-Content -LiteralPath (Join-Path $releaseLogDir 'package.log') -Value 'package ok' -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $releaseLogDir 'release-evidence.result.json') -Value '{"ok":true}' -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $artifactRoot '.env') -Value 'TOKEN=secret' -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $artifactRoot 'api-token.txt') -Value 'secret' -Encoding UTF8

    $result = & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath `
        -ArtifactRoot $artifactRoot `
        -OutputRoot $outputRoot `
        -RunId 'test-run' `
        -Json | ConvertFrom-Json

    if (-not $result.ok) {
        throw 'collect-diagnostics.ps1 should return ok=true when it can write a bundle.'
    }
    if (-not (Test-Path -LiteralPath $result.bundlePath)) {
        throw "Missing diagnostics bundle zip: $($result.bundlePath)"
    }
    if (-not (Test-Path -LiteralPath $result.manifestPath)) {
        throw "Missing diagnostics manifest: $($result.manifestPath)"
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $extractRoot = Join-Path $tempRoot 'extract'
    [System.IO.Compression.ZipFile]::ExtractToDirectory($result.bundlePath, $extractRoot)

    foreach ($requiredFile in @(
            'diagnostics-manifest.json',
            'environment.txt',
            'git-status.txt',
            'logs\release\logs\package.log',
            'logs\release\logs\release-evidence.result.json'
        )) {
        $requiredPath = Join-Path $extractRoot $requiredFile
        if (-not (Test-Path -LiteralPath $requiredPath)) {
            throw "Diagnostics bundle missing required file: $requiredFile"
        }
    }

    foreach ($forbiddenFile in @(
            '.env',
            'api-token.txt'
        )) {
        $forbiddenMatches = @(Get-ChildItem -LiteralPath $extractRoot -Recurse -Force | Where-Object { $_.Name -eq $forbiddenFile })
        if ($forbiddenMatches.Count -ne 0) {
            throw "Diagnostics bundle must exclude secret-like file: $forbiddenFile"
        }
    }

    $manifest = Get-Content -Raw -LiteralPath $result.manifestPath | ConvertFrom-Json
    if (-not $manifest.secretExclusionEnabled) {
        throw 'Diagnostics manifest should record that secret-like files were excluded.'
    }
    if ($manifest.includedFiles.Count -lt 2) {
        throw 'Diagnostics manifest should list copied diagnostic files.'
    }
    if (-not (@($manifest.includedFiles) -match 'package\.log')) {
        throw 'Diagnostics manifest should include package.log.'
    }
} finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}

"Test-DiagnosticsBundle.ps1 passed"
