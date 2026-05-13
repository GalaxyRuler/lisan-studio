[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$scriptPath = Join-Path $repoRoot 'scripts\beta-manual-check.ps1'

if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "Missing beta manual check script: $scriptPath"
}

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "beta-manual-check.ps1 parse failed: $($parseErrors[0].Message)"
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lisan-beta-manual-check-test-" + [System.Guid]::NewGuid().ToString('N'))
$releaseDir = Join-Path $tempRoot 'release\0.1.0-beta'
$logsDir = Join-Path $releaseDir 'logs'
$screenshotsDir = Join-Path $releaseDir 'screenshots'
$outputRoot = Join-Path $tempRoot 'manual'
$msiPath = Join-Path $tempRoot 'artifacts\LisanStudio-0.1.0-beta.msi'

try {
    New-Item -ItemType Directory -Force -Path $logsDir, $screenshotsDir, (Split-Path -Parent $msiPath) | Out-Null
    Set-Content -LiteralPath $msiPath -Value 'fake-msi' -Encoding ASCII
    Set-Content -LiteralPath (Join-Path $releaseDir 'CHECKSUMS-SHA256.txt') -Value 'ABC  fake' -Encoding ASCII
    Set-Content -LiteralPath (Join-Path $releaseDir 'VALIDATION_LOG.md') -Value '# Validation' -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $releaseDir 'KNOWN_ISSUES.md') -Value '# Known Issues' -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $screenshotsDir 'main-window.png') -Value 'fake-png' -Encoding ASCII
    Set-Content -LiteralPath (Join-Path $logsDir 'package.log') -Value 'package ok' -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $logsDir 'msi-smoke-keep-installed.log') -Value 'smoke ok' -Encoding UTF8

    $result = & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath `
        -ReleaseLabel '0.1.0-beta' `
        -ReleaseDir $releaseDir `
        -MsiPath $msiPath `
        -OutputRoot $outputRoot `
        -RunId 'test-run' `
        -Json | ConvertFrom-Json

    if (-not $result.ok) {
        throw 'beta-manual-check.ps1 should return ok=true when it can write a report.'
    }
    if ($result.manualQaStatus -ne 'NotStarted') {
        throw "Manual QA status should start as NotStarted, got: $($result.manualQaStatus)"
    }
    if (-not (Test-Path -LiteralPath $result.markdownPath)) {
        throw "Missing generated markdown report: $($result.markdownPath)"
    }
    if (-not (Test-Path -LiteralPath $result.jsonPath)) {
        throw "Missing generated JSON report: $($result.jsonPath)"
    }
    if (-not $result.wordPath -or -not (Test-Path -LiteralPath $result.wordPath)) {
        throw "Missing generated Word report: $($result.wordPath)"
    }

    $markdown = Get-Content -Raw -LiteralPath $result.markdownPath
    foreach ($requiredText in @(
            '# Lisan Studio 0.1.0-beta Manual Beta QA',
            'Manual QA Status: NotStarted',
            '## Existing Release Evidence',
            '## Manual Installed-App Checklist',
            '| Item | Result | Notes |',
            '| --- | --- | --- |',
            ('- [present] MSI: `{0}` - Private beta installer package' -f $msiPath),
            '| Start Menu launch opens Lisan Studio | NotRecorded |  |',
            '| Desktop shortcut launch opens Lisan Studio | NotRecorded |  |',
            '| Arabic mixed-direction edit, save, close, and reopen preserves text | NotRecorded |  |',
            '| Run current `.apy` shows readable UTF-8 Arabic output | NotRecorded |  |',
            '| Uninstall removes app payload and shortcuts | NotRecorded |  |'
        )) {
        if ($markdown -notmatch [regex]::Escape($requiredText)) {
            throw "Manual QA markdown missing required text: $requiredText"
        }
    }
    if ($markdown -match '\$\(@\{') {
        throw 'Manual QA markdown should render plain artifact paths, not PowerShell object expressions.'
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $docxExtractRoot = Join-Path $tempRoot 'docx-extract'
    [System.IO.Compression.ZipFile]::ExtractToDirectory($result.wordPath, $docxExtractRoot)
    $documentXmlPath = Join-Path $docxExtractRoot 'word\document.xml'
    if (-not (Test-Path -LiteralPath $documentXmlPath)) {
        throw 'Generated Word report should contain word/document.xml.'
    }
    $documentXml = Get-Content -Raw -LiteralPath $documentXmlPath
    foreach ($requiredDocxText in @(
            'Lisan Studio 0.1.0-beta Manual Beta QA',
            'Manual QA Status',
            'Existing Release Evidence',
            'Manual Installed-App Checklist',
            'Start Menu launch opens Lisan Studio',
            'Run current `.apy` shows readable UTF-8 Arabic output',
            'NotRecorded',
            'Reviewer Notes'
        )) {
        if ($documentXml -notmatch [regex]::Escape($requiredDocxText)) {
            throw "Generated Word report missing required text: $requiredDocxText"
        }
    }
    if ($documentXml -notmatch '<w:tbl>') {
        throw 'Generated Word report should use Word tables for artifacts and checklist items.'
    }

    $json = Get-Content -Raw -LiteralPath $result.jsonPath | ConvertFrom-Json
    if ($json.artifacts.Count -lt 7) {
        throw 'Manual QA JSON should record the expected release artifacts.'
    }
    if ($json.checklist.Count -lt 13) {
        throw 'Manual QA JSON should record every installed-app checklist item.'
    }
    $unrecordedItems = @($json.checklist | Where-Object { $_.status -ne 'NotRecorded' -or -not $_.item })
    if ($unrecordedItems.Count -ne 0) {
        throw 'Manual QA JSON checklist items should start with item text and status=NotRecorded.'
    }
    $missing = @($json.artifacts | Where-Object { -not $_.exists })
    if ($missing.Count -ne 0) {
        throw "Expected all fake artifacts to be present, missing: $($missing.name -join ', ')"
    }
} finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}

"Test-BetaManualCheck.ps1 passed"
