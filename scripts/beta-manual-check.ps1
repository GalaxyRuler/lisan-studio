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

function ConvertTo-DocxXmlText {
    param([AllowNull()][string]$Value)

    if ($null -eq $Value) {
        return ''
    }
    return [System.Security.SecurityElement]::Escape($Value)
}

function New-DocxParagraphXml {
    param(
        [string]$Text,
        [ValidateSet('Title', 'Heading1', 'Normal')]
        [string]$Style = 'Normal'
    )

    $styleXml = ''
    if ($Style -ne 'Normal') {
        $styleXml = '<w:pPr><w:pStyle w:val="' + $Style + '"/></w:pPr>'
    }
    return '<w:p>' + $styleXml + '<w:r><w:t xml:space="preserve">' + (ConvertTo-DocxXmlText $Text) + '</w:t></w:r></w:p>'
}

function New-DocxTableXml {
    param(
        [string[]]$Headers,
        [object[]]$Rows
    )

    $xml = New-Object System.Collections.Generic.List[string]
    [void]$xml.Add('<w:tbl><w:tblPr><w:tblStyle w:val="TableGrid"/><w:tblW w:w="0" w:type="auto"/></w:tblPr>')
    [void]$xml.Add('<w:tr>')
    foreach ($header in $Headers) {
        [void]$xml.Add('<w:tc><w:tcPr><w:tcW w:w="2400" w:type="dxa"/></w:tcPr><w:p><w:r><w:b/><w:t xml:space="preserve">' + (ConvertTo-DocxXmlText $header) + '</w:t></w:r></w:p></w:tc>')
    }
    [void]$xml.Add('</w:tr>')

    foreach ($row in $Rows) {
        [void]$xml.Add('<w:tr>')
        foreach ($cell in @($row)) {
            [void]$xml.Add('<w:tc><w:tcPr><w:tcW w:w="2400" w:type="dxa"/></w:tcPr><w:p><w:r><w:t xml:space="preserve">' + (ConvertTo-DocxXmlText ([string]$cell)) + '</w:t></w:r></w:p></w:tc>')
        }
        [void]$xml.Add('</w:tr>')
    }

    [void]$xml.Add('</w:tbl>')
    return ($xml -join '')
}

function New-ManualQaWordDocument {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$ReleaseLabel,
        [Parameter(Mandatory = $true)][string]$GeneratedAt,
        [Parameter(Mandatory = $true)][string]$Repository,
        [Parameter(Mandatory = $true)][string]$ManualQaStatus,
        [Parameter(Mandatory = $true)][object[]]$Artifacts,
        [Parameter(Mandatory = $true)][object[]]$Checklist
    )

    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $tempDocxRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lisan-beta-docx-" + [System.Guid]::NewGuid().ToString('N'))
    $wordDir = Join-Path $tempDocxRoot 'word'
    $relsDir = Join-Path $tempDocxRoot '_rels'
    $docPropsDir = Join-Path $tempDocxRoot 'docProps'
    New-Item -ItemType Directory -Force -Path $wordDir, $relsDir, $docPropsDir | Out-Null

    try {
        @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>
</Types>
'@ | Set-Content -LiteralPath (Join-Path $tempDocxRoot '[Content_Types].xml') -Encoding UTF8

        @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>
</Relationships>
'@ | Set-Content -LiteralPath (Join-Path $relsDir '.rels') -Encoding UTF8

        @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>
  <w:style w:type="paragraph" w:styleId="Title"><w:name w:val="Title"/><w:basedOn w:val="Normal"/><w:pPr><w:jc w:val="center"/></w:pPr><w:rPr><w:b/><w:sz w:val="32"/></w:rPr></w:style>
  <w:style w:type="paragraph" w:styleId="Heading1"><w:name w:val="heading 1"/><w:basedOn w:val="Normal"/><w:rPr><w:b/><w:sz w:val="24"/></w:rPr></w:style>
  <w:style w:type="table" w:styleId="TableGrid"><w:name w:val="Table Grid"/><w:tblPr><w:tblBorders><w:top w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:left w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:bottom w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:right w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:insideH w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:insideV w:val="single" w:sz="4" w:space="0" w:color="auto"/></w:tblBorders></w:tblPr></w:style>
</w:styles>
'@ | Set-Content -LiteralPath (Join-Path $wordDir 'styles.xml') -Encoding UTF8

        $artifactRows = @($Artifacts | ForEach-Object {
            $state = if ($_.exists) { 'Present' } else { 'Missing' }
            @($_.name, $state, $_.path, $_.purpose)
        })
        $checklistRows = @($Checklist | ForEach-Object {
            @($_.item, $_.status, $_.notes)
        })
        $body = @(
            (New-DocxParagraphXml -Text "Lisan Studio $ReleaseLabel Manual Beta QA" -Style Title),
            (New-DocxParagraphXml -Text "Generated: $GeneratedAt"),
            (New-DocxParagraphXml -Text "Repository: $Repository"),
            (New-DocxParagraphXml -Text "Manual QA Status: $ManualQaStatus"),
            (New-DocxParagraphXml -Text "This Word document is for the human installed-app beta pass. It does not launch the app, install or uninstall MSI packages, run GUI automation, mutate Hyper-V, or use active WHITEDRAGON for validation."),
            (New-DocxParagraphXml -Text 'Existing Release Evidence' -Style Heading1),
            (New-DocxTableXml -Headers @('Artifact', 'State', 'Path', 'Purpose') -Rows $artifactRows),
            (New-DocxParagraphXml -Text 'Manual Installed-App Checklist' -Style Heading1),
            (New-DocxTableXml -Headers @('Item', 'Result', 'Reviewer Notes') -Rows $checklistRows),
            (New-DocxParagraphXml -Text 'Completion Rule' -Style Heading1),
            (New-DocxParagraphXml -Text 'Manual QA is not complete until every applicable checklist item is marked Pass, Fail, or Blocked and any failures are recorded with artifact paths or screenshots.')
        ) -join ''
        $documentXml = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>' + $body + '<w:sectPr><w:pgSz w:w="12240" w:h="15840"/><w:pgMar w:top="720" w:right="720" w:bottom="720" w:left="720" w:header="720" w:footer="720" w:gutter="0"/></w:sectPr></w:body></w:document>'
        $documentXml | Set-Content -LiteralPath (Join-Path $wordDir 'document.xml') -Encoding UTF8

        $created = (Get-Date).ToUniversalTime().ToString('s') + 'Z'
        ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?><cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><dc:title>Lisan Studio Manual Beta QA</dc:title><dc:creator>Codex</dc:creator><dcterms:created xsi:type="dcterms:W3CDTF">' + $created + '</dcterms:created></cp:coreProperties>') |
            Set-Content -LiteralPath (Join-Path $docPropsDir 'core.xml') -Encoding UTF8

        if (Test-Path -LiteralPath $Path) {
            Remove-Item -LiteralPath $Path -Force
        }
        [System.IO.Compression.ZipFile]::CreateFromDirectory($tempDocxRoot, $Path)
    } finally {
        Remove-Item -LiteralPath $tempDocxRoot -Recurse -Force -ErrorAction SilentlyContinue
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
$wordPath = Join-Path $outputDirectory "manual-beta-qa.docx"

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
New-ManualQaWordDocument `
    -Path $wordPath `
    -ReleaseLabel $ReleaseLabel `
    -GeneratedAt $generatedAt `
    -Repository $repo `
    -ManualQaStatus $manualQaStatus `
    -Artifacts $artifacts `
    -Checklist $checklist

$result = [PSCustomObject]@{
    ok = $true
    generatedAt = $generatedAt
    releaseLabel = $ReleaseLabel
    releaseDir = $ReleaseDir
    msiPath = $MsiPath
    outputDirectory = $outputDirectory
    markdownPath = $markdownPath
    wordPath = $wordPath
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
