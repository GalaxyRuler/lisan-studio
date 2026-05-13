param(
    [string]$ReleaseLabel = "0.1.0-beta",
    [string]$ReleaseDir = "",
    [string]$MsiPath = "",
    [string]$OutputRoot = "",
    [string]$RunId = "",
    [switch]$Json
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($ReleaseDir)) {
    $ReleaseDir = Join-Path $repo "artifacts\release\$ReleaseLabel"
}
if ([string]::IsNullOrWhiteSpace($MsiPath)) {
    $MsiPath = Join-Path $repo "artifacts\LisanStudio-$ReleaseLabel.msi"
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $repo "artifacts\beta-manual-check"
}
if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = Get-Date -Format "yyyyMMddTHHmmss"
}

$outputDirectory = Join-Path $OutputRoot $RunId
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

function New-ArtifactStatus {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Purpose
    )

    $item = Get-Item -LiteralPath $Path -ErrorAction SilentlyContinue
    [PSCustomObject]@{
        name = $Name
        path = $Path
        purpose = $Purpose
        exists = [bool]$item
        length = if ($item) { $item.Length } else { $null }
    }
}

$artifactSpecs = @(
    @{ Name = "MSI"; Path = $MsiPath; Purpose = "Private beta installer package" },
    @{ Name = "Validation log"; Path = (Join-Path $ReleaseDir "VALIDATION_LOG.md"); Purpose = "Automated validation evidence" },
    @{ Name = "Checksums"; Path = (Join-Path $ReleaseDir "CHECKSUMS-SHA256.txt"); Purpose = "Release artifact hashes" },
    @{ Name = "Known issues"; Path = (Join-Path $ReleaseDir "KNOWN_ISSUES.md"); Purpose = "Private beta limitations and blockers" },
    @{ Name = "Main window screenshot"; Path = (Join-Path $ReleaseDir "screenshots\main-window.png"); Purpose = "Installed app visual evidence" },
    @{ Name = "Package log"; Path = (Join-Path $ReleaseDir "logs\package.log"); Purpose = "Packaging command log" },
    @{ Name = "MSI smoke log"; Path = (Join-Path $ReleaseDir "logs\msi-smoke-keep-installed.log"); Purpose = "Installed MSI smoke log" }
)
$artifacts = $artifactSpecs | ForEach-Object {
    New-ArtifactStatus -Name $_.Name -Path $_.Path -Purpose $_.Purpose
}

$checklistItems = @(
    "Start Menu launch opens Lisan Studio",
    "Desktop shortcut launch opens Lisan Studio",
    "Open the installed sample project and verify the RTL shell",
    'Open a real `.apy` folder from the project sidebar',
    "Arabic mixed-direction edit, save, close, and reopen preserves text",
    "Cursor, selection, copy, paste, undo, redo, backspace, and delete behave around Arabic text",
    "Project search opens a clicked file and line",
    "Problems panel reports an inserted hidden BiDi control",
    'Run current `.apy` shows readable UTF-8 Arabic output',
    "Output panel shows command, working directory, exit code, elapsed time, and cancel behavior",
    "Settings opens, runtime diagnostics are readable, and editor font changes persist after restart",
    "Uninstall removes app payload and shortcuts",
    "Reinstall from the same beta MSI succeeds without manual PATH or Python setup"
)
$checklist = $checklistItems | ForEach-Object {
    [PSCustomObject]@{
        item = $_
        status = "NotRecorded"
        notes = ""
    }
}

$manualQaStatus = "NotStarted"
$generatedAt = Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"
$markdownPath = Join-Path $outputDirectory "manual-beta-qa.md"
$jsonPath = Join-Path $outputDirectory "manual-beta-qa.json"

$artifactLines = $artifacts | ForEach-Object {
    $state = if ($_.exists) { "present" } else { "missing" }
    '- [{0}] {1}: `{2}` - {3}' -f $state, $_.name, $_.path, $_.purpose
}
$checklistLines = @("| Item | Result | Notes |", "| --- | --- | --- |")
$checklistLines += $checklist | ForEach-Object { "| $($_.item) | $($_.status) | $($_.notes) |" }

$markdownLines = @(
    "# Lisan Studio $ReleaseLabel Manual Beta QA",
    "",
    "Generated: $generatedAt",
    "Repository: $repo",
    "Manual QA Status: $manualQaStatus",
    "",
    "This report is an installed-app checklist for a human beta pass. It does not launch the app, install or uninstall MSI packages, run GUI automation, mutate Hyper-V, or use active WHITEDRAGON for validation.",
    "",
    "## Existing Release Evidence",
    "",
    $artifactLines,
    "",
    "## Manual Installed-App Checklist",
    "",
    $checklistLines,
    "",
    "## Completion Rule",
    "",
    "Manual QA is not complete until every applicable checklist item is marked and any failures are recorded with artifact paths or screenshots."
)
$markdownLines | Set-Content -LiteralPath $markdownPath -Encoding UTF8

$result = [PSCustomObject]@{
    ok = $true
    generatedAt = $generatedAt
    releaseLabel = $ReleaseLabel
    releaseDir = $ReleaseDir
    msiPath = $MsiPath
    outputDirectory = $outputDirectory
    markdownPath = $markdownPath
    jsonPath = $jsonPath
    manualQaStatus = $manualQaStatus
    artifacts = $artifacts
    checklist = $checklist
}
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonPath -Encoding UTF8

if ($Json) {
    $result | ConvertTo-Json -Depth 6
} else {
    $result
}
