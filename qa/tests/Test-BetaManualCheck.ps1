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
            '# Lisan Studio 0.1.0-beta Manual QA Review Packet',
            'Manual QA Status: NotStarted',
            '## 1. Automated Evidence Summary',
            '## 2. Reviewer Instructions',
            '## 3. Manual Installed-App QA Checklist',
            '## 4. Beta Blocker Watchlist',
            '## 5. Final Review Decision',
            '| # | Area | Test | Expected Result | Result | Reviewer Notes | Evidence / Screenshot |',
            '| --- | --- | --- | --- | --- | --- | --- |',
            ('- [present] MSI: `{0}` - Installer package' -f $msiPath),
            '| 1 | Launch | Start Menu launch opens Lisan Studio | App opens without crash and shows Lisan Studio branding | Not Recorded |  |  |',
            '| 2 | Launch | Desktop shortcut launch opens Lisan Studio | Shortcut targets the installed LisanStudio.exe, not any legacy app | Not Recorded |  |  |',
            '| 5 | Editing | Arabic mixed-direction edit, save, close, and reopen preserves text | Arabic, English, numbers, paths, and punctuation survive save/reopen without corruption | Not Recorded |  |  |',
            '| 9 | Runtime | Run current `.apy` shows readable UTF-8 Arabic output | Output panel shows Arabic text without mojibake | Not Recorded |  |  |',
            '| 12 | Installer | Uninstall removes app payload and shortcuts | Installed payload and shortcuts are removed; user settings are not treated as MSI payload | Not Recorded |  |  |',
            '| Arabic output mojibake | Arabic text appears as garbled characters in the output panel. |',
            '| Ready for release handoff | No blocking manual QA failures found |  |',
            '| Blocked | One or more beta blockers must be fixed before handoff |  |'
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
    $documentRelsPath = Join-Path $docxExtractRoot 'word\_rels\document.xml.rels'
    if (-not (Test-Path -LiteralPath $documentRelsPath)) {
        throw 'Generated Word report should contain word/_rels/document.xml.rels.'
    }
    $documentRels = Get-Content -Raw -LiteralPath $documentRelsPath
    if ($documentRels -notmatch 'officeDocument/2006/relationships/styles') {
        throw 'Generated Word report document relationships should link word/styles.xml.'
    }
    $documentXml = Get-Content -Raw -LiteralPath $documentXmlPath
    $stylesXmlPath = Join-Path $docxExtractRoot 'word\styles.xml'
    if (-not (Test-Path -LiteralPath $stylesXmlPath)) {
        throw 'Generated Word report should contain word/styles.xml.'
    }
    $stylesXml = Get-Content -Raw -LiteralPath $stylesXmlPath
    foreach ($requiredDocxText in @(
            'Lisan Studio 0.1.0-beta',
            'Manual QA Review Packet',
            'Product',
            'Manual Installed-App QA',
            'Target Environment',
            'Isolated Windows QA machine',
            'Validation Boundary',
            'Manual QA Status',
            'Reviewer',
            'Review Date',
            'Overall Decision',
            'Guest MSI Path',
            '1. Automated Evidence Summary',
            'Reviewer Instructions',
            'For each checklist item, record exactly one result: Pass, Fail, Blocked, or Not Applicable.',
            '3. Manual Installed-App QA Checklist',
            'Expected Result',
            'Start Menu launch opens Lisan Studio',
            'Run current `.apy` shows readable UTF-8 Arabic output',
            'Not Recorded',
            'Reviewer Notes',
            'Evidence / Screenshot',
            '4. Beta Blocker Watchlist',
            'Arabic output mojibake',
            '5. Final Review Decision',
            'Ready for release handoff',
            'Follow-up Issue Links or File Paths'
        )) {
        if ($documentXml -notmatch [regex]::Escape($requiredDocxText)) {
            throw "Generated Word report missing required text: $requiredDocxText"
        }
    }
    if ($documentXml -notmatch '<w:tbl>') {
        throw 'Generated Word report should use Word tables for artifacts and checklist items.'
    }
    foreach ($requiredProfessionalXml in @(
            '<w:pgSz w:w="15840" w:h="12240" w:orient="landscape"/>',
            '<w:tblGrid>',
            '<w:shd w:fill="1F4E79"',
            '<w:shd w:fill="EAF2F8"',
            '<w:tcMar>',
            '<w:spacing w:after="120"',
            'w:val="LisanTitle"',
            'w:val="LisanSubtitle"',
            'w:val="LisanHeading1"',
            'w:val="LisanInstruction"'
        )) {
        if ($documentXml -notmatch [regex]::Escape($requiredProfessionalXml) -and
            $stylesXml -notmatch [regex]::Escape($requiredProfessionalXml)) {
            throw "Generated Word report missing professional formatting XML: $requiredProfessionalXml"
        }
    }

    $json = Get-Content -Raw -LiteralPath $result.jsonPath | ConvertFrom-Json
    if ($json.artifacts.Count -lt 7) {
        throw 'Manual QA JSON should record the expected release artifacts.'
    }
    if ($json.checklist.Count -lt 13) {
        throw 'Manual QA JSON should record every installed-app checklist item.'
    }
    $unrecordedItems = @($json.checklist | Where-Object { $_.status -ne 'Not Recorded' -or -not $_.test -or -not $_.area -or -not $_.expectedResult })
    if ($unrecordedItems.Count -ne 0) {
        throw 'Manual QA JSON checklist items should start with area, test, expectedResult, and status=Not Recorded.'
    }
    if ($json.blockerWatchlist.Count -lt 10) {
        throw 'Manual QA JSON should record the standard beta blocker watchlist.'
    }
    $missing = @($json.artifacts | Where-Object { -not $_.exists })
    if ($missing.Count -ne 0) {
        throw "Expected all fake artifacts to be present, missing: $($missing.name -join ', ')"
    }
} finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}

"Test-BetaManualCheck.ps1 passed"
