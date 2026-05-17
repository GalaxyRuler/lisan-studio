[CmdletBinding()]
param(
    [string]$ArtifactRoot,
    [string]$OutputRoot,
    [string]$RunId,
    [switch]$Json
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
if (-not $ArtifactRoot) {
    $ArtifactRoot = Join-Path $repoRoot 'artifacts'
}
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $repoRoot 'artifacts\diagnostics'
}
if (-not $RunId) {
    $RunId = (Get-Date).ToUniversalTime().ToString('yyyyMMdd-HHmmss')
}

$bundleRoot = Join-Path $OutputRoot $RunId
$logsRoot = Join-Path $bundleRoot 'logs'
$manifestPath = Join-Path $bundleRoot 'diagnostics-manifest.json'
$environmentPath = Join-Path $bundleRoot 'environment.txt'
$gitStatusPath = Join-Path $bundleRoot 'git-status.txt'
$bundlePath = Join-Path $OutputRoot ("lisan-studio-diagnostics-{0}.zip" -f $RunId)

function Test-SecretLikePath {
    param([Parameter(Mandatory = $true)][string]$CandidatePath)

    $leafName = Split-Path -Leaf $CandidatePath
    return $leafName -match '(^\.env($|\.)|secret|token|credential|password|private|\.pem$|\.pfx$|\.key$|\.cer$|\.crt$)'
}

function Get-RelativeChildPath {
    param(
        [Parameter(Mandatory = $true)][string]$RootPath,
        [Parameter(Mandatory = $true)][string]$ChildPath
    )

    $rootFullPath = [System.IO.Path]::GetFullPath($RootPath).TrimEnd('\', '/')
    $childFullPath = [System.IO.Path]::GetFullPath($ChildPath)
    $rootWithSeparator = $rootFullPath + [System.IO.Path]::DirectorySeparatorChar
    if ($childFullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $childFullPath.Substring($rootWithSeparator.Length)
    }
    return Split-Path -Leaf $ChildPath
}

function Invoke-GitText {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    try {
        $outputText = & git @Arguments 2>&1
        if ($LASTEXITCODE -ne 0) {
            return "git $($Arguments -join ' ') failed with exit code $LASTEXITCODE`n$outputText"
        }
        return ($outputText -join "`n")
    } catch {
        return "git unavailable: $($_.Exception.Message)"
    }
}

New-Item -ItemType Directory -Force -Path $bundleRoot, $logsRoot, $OutputRoot | Out-Null

$environmentLines = @(
    'Lisan Studio Diagnostics Bundle',
    ('GeneratedUtc: {0}' -f (Get-Date).ToUniversalTime().ToString('o')),
    ('RepoRoot: {0}' -f $repoRoot),
    ('PowerShellVersion: {0}' -f $PSVersionTable.PSVersion.ToString()),
    ('OS: {0}' -f [System.Environment]::OSVersion.VersionString),
    ('MachineName: {0}' -f [System.Environment]::MachineName),
    'Environment variables are intentionally not captured.'
)
$environmentLines | Set-Content -LiteralPath $environmentPath -Encoding UTF8

$gitLines = @(
    '## git rev-parse --abbrev-ref HEAD',
    (Invoke-GitText -Arguments @('-C', $repoRoot, 'rev-parse', '--abbrev-ref', 'HEAD')),
    '',
    '## git rev-parse HEAD',
    (Invoke-GitText -Arguments @('-C', $repoRoot, 'rev-parse', 'HEAD')),
    '',
    '## git status --short --branch',
    (Invoke-GitText -Arguments @('-C', $repoRoot, 'status', '--short', '--branch'))
)
$gitLines | Set-Content -LiteralPath $gitStatusPath -Encoding UTF8

$safeExtensions = @('.log', '.txt', '.json', '.md')
$includedFiles = New-Object System.Collections.Generic.List[string]
$skippedFiles = New-Object System.Collections.Generic.List[string]
$maxBytes = 1024 * 1024

if (Test-Path -LiteralPath $ArtifactRoot) {
    $artifactRootFullPath = [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $ArtifactRoot).Path)
    $artifactFiles = Get-ChildItem -LiteralPath $artifactRootFullPath -Recurse -File -Force
    foreach ($artifactFile in $artifactFiles) {
        $relativePath = Get-RelativeChildPath -RootPath $artifactRootFullPath -ChildPath $artifactFile.FullName
        if (Test-SecretLikePath -CandidatePath $artifactFile.FullName) {
            $skippedFiles.Add($relativePath)
            continue
        }
        if ($safeExtensions -notcontains $artifactFile.Extension.ToLowerInvariant()) {
            $skippedFiles.Add($relativePath)
            continue
        }
        if ($artifactFile.Length -gt $maxBytes) {
            $skippedFiles.Add($relativePath)
            continue
        }

        $destinationPath = Join-Path $logsRoot $relativePath
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destinationPath) | Out-Null
        Copy-Item -LiteralPath $artifactFile.FullName -Destination $destinationPath -Force
        $includedFiles.Add($relativePath)
    }
}

$manifest = [ordered]@{
    ok = $true
    runId = $RunId
    generatedUtc = (Get-Date).ToUniversalTime().ToString('o')
    repoRoot = "$repoRoot"
    artifactRoot = [System.IO.Path]::GetFullPath($ArtifactRoot)
    bundlePath = $bundlePath
    secretExclusionEnabled = $true
    includedFiles = @($includedFiles)
    skippedFiles = @($skippedFiles)
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding UTF8

if (Test-Path -LiteralPath $bundlePath) {
    Remove-Item -LiteralPath $bundlePath -Force
}
Compress-Archive -Path (Join-Path $bundleRoot '*') -DestinationPath $bundlePath -Force

$result = [ordered]@{
    ok = $true
    runId = $RunId
    bundlePath = $bundlePath
    manifestPath = $manifestPath
    includedFileCount = $includedFiles.Count
    skippedFileCount = $skippedFiles.Count
}

if ($Json) {
    $result | ConvertTo-Json -Depth 6
} else {
    Write-Host ("Diagnostics bundle: {0}" -f $bundlePath)
    Write-Host ("Manifest: {0}" -f $manifestPath)
}
