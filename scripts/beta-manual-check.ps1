param(
    [string]$ReleaseLabel = "0.1.0-beta",
    [string]$ReleaseDir = "",
    [string]$MsiPath = "",
    [string]$GuestMsiPath = "",
    [string]$OutputRoot = "",
    [string]$RunId = "",
    [string]$TemplatePath = "",
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
if ([string]::IsNullOrWhiteSpace($GuestMsiPath)) {
    $GuestMsiPath = "C:\CodexRunner\work\arabic-code-studio-qt\artifacts\LisanStudio-$ReleaseLabel.msi"
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $repo "artifacts\beta-manual-check"
}
if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = Get-Date -Format "yyyyMMddTHHmmss"
}
if ([string]::IsNullOrWhiteSpace($TemplatePath)) {
    $homelabRoot = if ($env:CODEX_HOMELAB_ROOT) {
        $env:CODEX_HOMELAB_ROOT
    } else {
        'C:\Users\Admin\Documents\Codex\Homelab\codex-isolated-test-runners'
    }
    $candidateTemplatePath = Join-Path $homelabRoot 'docs\templates\manual-qa-review-packet-template.docx'
    if (Test-Path -LiteralPath $candidateTemplatePath) {
        $TemplatePath = $candidateTemplatePath
    }
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
        [ValidateSet('Title', 'Subtitle', 'Heading1', 'Instruction', 'Normal')]
        [string]$Style = 'Normal'
    )

    $styleXml = ''
    if ($Style -ne 'Normal') {
        $styleId = switch ($Style) {
            'Title' { 'LisanTitle' }
            'Subtitle' { 'LisanSubtitle' }
            'Heading1' { 'LisanHeading1' }
            'Instruction' { 'LisanInstruction' }
        }
        $styleXml = '<w:pPr><w:pStyle w:val="' + $styleId + '"/><w:spacing w:after="120"/></w:pPr>'
    }
    return '<w:p>' + $styleXml + '<w:r><w:t xml:space="preserve">' + (ConvertTo-DocxXmlText $Text) + '</w:t></w:r></w:p>'
}

function New-DocxTableXml {
    param(
        [string[]]$Headers,
        [object[]]$Rows,
        [int[]]$ColumnWidths = @()
    )

    if ($ColumnWidths.Count -ne $Headers.Count) {
        $ColumnWidths = @(1..$Headers.Count | ForEach-Object { 2400 })
    }
    $normalizedRows = New-Object System.Collections.Generic.List[object[]]
    $pendingCells = New-Object System.Collections.Generic.List[object]
    foreach ($row in $Rows) {
        if ($row -is [array]) {
            if ($pendingCells.Count -gt 0) {
                while ($pendingCells.Count -lt $Headers.Count) {
                    [void]$pendingCells.Add('')
                }
                [void]$normalizedRows.Add([object[]]$pendingCells.ToArray())
                $pendingCells.Clear()
            }
            [void]$normalizedRows.Add([object[]]$row)
        } else {
            [void]$pendingCells.Add($row)
            if ($pendingCells.Count -eq $Headers.Count) {
                [void]$normalizedRows.Add([object[]]$pendingCells.ToArray())
                $pendingCells.Clear()
            }
        }
    }
    if ($pendingCells.Count -gt 0) {
        while ($pendingCells.Count -lt $Headers.Count) {
            [void]$pendingCells.Add('')
        }
        [void]$normalizedRows.Add([object[]]$pendingCells.ToArray())
    }

    $xml = New-Object System.Collections.Generic.List[string]
    $tableWidth = ($ColumnWidths | Measure-Object -Sum).Sum
    [void]$xml.Add('<w:tbl><w:tblPr><w:tblStyle w:val="TableGrid"/><w:tblW w:w="' + $tableWidth + '" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="80" w:type="dxa"/><w:left w:w="80" w:type="dxa"/><w:bottom w:w="80" w:type="dxa"/><w:right w:w="80" w:type="dxa"/></w:tblCellMar></w:tblPr>')
    [void]$xml.Add('<w:tblGrid>')
    foreach ($width in $ColumnWidths) {
        [void]$xml.Add('<w:gridCol w:w="' + $width + '"/>')
    }
    [void]$xml.Add('</w:tblGrid>')
    [void]$xml.Add('<w:tr>')
    for ($index = 0; $index -lt $Headers.Count; ++$index) {
        $header = $Headers[$index]
        $width = $ColumnWidths[$index]
        [void]$xml.Add('<w:tc><w:tcPr><w:tcW w:w="' + $width + '" w:type="dxa"/><w:shd w:fill="1F4E79" w:val="clear"/><w:tcMar><w:top w:w="90" w:type="dxa"/><w:left w:w="90" w:type="dxa"/><w:bottom w:w="90" w:type="dxa"/><w:right w:w="90" w:type="dxa"/></w:tcMar></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t xml:space="preserve">' + (ConvertTo-DocxXmlText $header) + '</w:t></w:r></w:p></w:tc>')
    }
    [void]$xml.Add('</w:tr>')

    $rowIndex = 0
    foreach ($row in $normalizedRows) {
        $fill = if (($rowIndex % 2) -eq 0) { 'FFFFFF' } else { 'EAF2F8' }
        [void]$xml.Add('<w:tr>')
        $cells = @($row)
        for ($index = 0; $index -lt $Headers.Count; ++$index) {
            $cell = if ($index -lt $cells.Count) { [string]$cells[$index] } else { '' }
            $width = $ColumnWidths[$index]
            [void]$xml.Add('<w:tc><w:tcPr><w:tcW w:w="' + $width + '" w:type="dxa"/><w:shd w:fill="' + $fill + '" w:val="clear"/><w:tcMar><w:top w:w="90" w:type="dxa"/><w:left w:w="90" w:type="dxa"/><w:bottom w:w="90" w:type="dxa"/><w:right w:w="90" w:type="dxa"/></w:tcMar></w:tcPr><w:p><w:r><w:t xml:space="preserve">' + (ConvertTo-DocxXmlText $cell) + '</w:t></w:r></w:p></w:tc>')
        }
        [void]$xml.Add('</w:tr>')
        ++$rowIndex
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
        [Parameter(Mandatory = $true)][string]$GuestMsiPath,
        [Parameter(Mandatory = $true)][object[]]$Artifacts,
        [Parameter(Mandatory = $true)][object[]]$Checklist,
        [Parameter(Mandatory = $true)][object[]]$BlockerWatchlist,
        [string]$TemplatePath
    )

    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $tempDocxRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lisan-beta-docx-" + [System.Guid]::NewGuid().ToString('N'))
    $wordDir = Join-Path $tempDocxRoot 'word'
    $wordRelsDir = Join-Path $wordDir '_rels'
    $relsDir = Join-Path $tempDocxRoot '_rels'
    $docPropsDir = Join-Path $tempDocxRoot 'docProps'

    try {
        $resolvedTemplatePath = $null
        if (-not [string]::IsNullOrWhiteSpace($TemplatePath) -and (Test-Path -LiteralPath $TemplatePath)) {
            $resolvedTemplatePath = (Resolve-Path -LiteralPath $TemplatePath).Path
        }
        if ($resolvedTemplatePath) {
            [System.IO.Compression.ZipFile]::ExtractToDirectory($resolvedTemplatePath, $tempDocxRoot)
        }

        New-Item -ItemType Directory -Force -Path $wordDir, $wordRelsDir, $relsDir, $docPropsDir | Out-Null

        if (-not $resolvedTemplatePath) {
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
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>
'@ | Set-Content -LiteralPath (Join-Path $wordRelsDir 'document.xml.rels') -Encoding UTF8
        }

        $lisanStylesXml = @'
  <w:style w:type="paragraph" w:styleId="LisanTitle"><w:name w:val="Lisan Title"/><w:basedOn w:val="Normal"/><w:pPr><w:jc w:val="center"/><w:shd w:fill="1F4E79" w:val="clear"/><w:spacing w:before="120" w:after="120"/></w:pPr><w:rPr><w:b/><w:color w:val="FFFFFF"/><w:sz w:val="36"/></w:rPr></w:style>
  <w:style w:type="paragraph" w:styleId="LisanSubtitle"><w:name w:val="Lisan Subtitle"/><w:basedOn w:val="Normal"/><w:pPr><w:jc w:val="center"/><w:spacing w:after="180"/></w:pPr><w:rPr><w:color w:val="365F91"/><w:sz w:val="22"/></w:rPr></w:style>
  <w:style w:type="paragraph" w:styleId="LisanHeading1"><w:name w:val="Lisan Heading 1"/><w:basedOn w:val="Normal"/><w:pPr><w:spacing w:before="220" w:after="100"/></w:pPr><w:rPr><w:b/><w:color w:val="1F4E79"/><w:sz w:val="26"/></w:rPr></w:style>
  <w:style w:type="paragraph" w:styleId="LisanInstruction"><w:name w:val="Lisan Instruction"/><w:basedOn w:val="Normal"/><w:pPr><w:shd w:fill="EAF2F8" w:val="clear"/><w:spacing w:before="80" w:after="120"/></w:pPr><w:rPr><w:color w:val="1F4E79"/><w:sz w:val="21"/></w:rPr></w:style>
  <w:style w:type="table" w:styleId="TableGrid"><w:name w:val="Table Grid"/><w:tblPr><w:tblBorders><w:top w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:left w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:bottom w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:right w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:insideH w:val="single" w:sz="4" w:space="0" w:color="auto"/><w:insideV w:val="single" w:sz="4" w:space="0" w:color="auto"/></w:tblBorders></w:tblPr></w:style>
'@
        $stylesPath = Join-Path $wordDir 'styles.xml'
        if ($resolvedTemplatePath -and (Test-Path -LiteralPath $stylesPath)) {
            $stylesXml = Get-Content -Raw -LiteralPath $stylesPath
            if ($stylesXml -notmatch 'w:styleId="LisanTitle"') {
                $stylesXml = $stylesXml -replace '</w:styles>\s*$', ($lisanStylesXml + "`r`n</w:styles>")
                Set-Content -LiteralPath $stylesPath -Value $stylesXml -Encoding UTF8
            }
        } else {
            @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/><w:rPr><w:rFonts w:ascii="Aptos" w:hAnsi="Aptos"/><w:sz w:val="21"/></w:rPr></w:style>
$lisanStylesXml
</w:styles>
"@ | Set-Content -LiteralPath $stylesPath -Encoding UTF8
        }

        $artifactRows = @($Artifacts | ForEach-Object {
            $state = if ($_.exists) { 'Present' } else { 'Missing' }
            @($_.name, $state, $_.path, $_.purpose)
        })
        $rowNumber = 0
        $checklistRows = @($Checklist | ForEach-Object {
            ++$rowNumber
            @($rowNumber, $_.area, $_.test, $_.expectedResult, $_.status, $_.notes, '')
        })
        $blockerRows = @($BlockerWatchlist | ForEach-Object {
            @($_.status, $_.category, $_.description)
        })
        $summaryRows = @(
            @('Product', 'Lisan Studio'),
            @('Version', $ReleaseLabel),
            @('Review Type', 'Private Beta - Manual Installed-App QA'),
            @('Target Environment', 'LisanStudio-QA (isolated VM)'),
            @('Validation Boundary', 'GUI / MSI / installed-app validation must NOT run on active WHITEDRAGON'),
            @('Manual QA Status', $ManualQaStatus),
            @('Reviewer', ''),
            @('Review Date', ''),
            @('Overall Decision', '')
        )
        $body = @(
            (New-DocxParagraphXml -Text "Lisan Studio $ReleaseLabel" -Style Title),
            (New-DocxParagraphXml -Text 'Manual QA Review Packet' -Style Subtitle),
            (New-DocxTableXml -Headers @('Field', 'Value') -Rows $summaryRows -ColumnWidths @(2600, 10600)),
            (New-DocxParagraphXml -Text '1. Automated Evidence Summary' -Style Heading1),
            (New-DocxParagraphXml -Text 'The automated validation lane has already completed its run through the isolated Homelab lane inside the LisanStudio-QA VM. No manual re-execution of automated checks is required. The table below lists all artifacts produced and confirmed present at their respective paths.'),
            (New-DocxTableXml -Headers @('Artifact', 'Status', 'Path / Location', 'Purpose') -Rows $artifactRows -ColumnWidths @(1700, 1100, 7700, 2700)),
            (New-DocxParagraphXml -Text 'Reviewer Instructions' -Style Heading1),
            (New-DocxParagraphXml -Text 'Perform all testing exclusively inside the LisanStudio-QA isolated VM. Do not install, run, or uninstall the MSI on active WHITEDRAGON.'),
            (New-DocxParagraphXml -Text 'Locate the beta MSI at the path listed in Section 1 and install it fresh before beginning the checklist.'),
            (New-DocxParagraphXml -Text 'For each checklist item, record exactly one result: Pass, Fail, Blocked, or Not Applicable. For any Fail or Blocked result, provide a clear note and include the file path or filename of any screenshot or log evidence.' -Style Instruction),
            (New-DocxParagraphXml -Text 'Manual QA is not complete until every checklist row carries a recorded result. Leave no row as Not Recorded at sign-off.'),
            (New-DocxParagraphXml -Text 'If a beta blocker is encountered, stop, record the failure, and escalate before proceeding to dependent tests.'),
            (New-DocxParagraphXml -Text 'Retain all screenshots, logs, and notes generated during this review for inclusion in the release record.'),
            (New-DocxParagraphXml -Text "Generated: $GeneratedAt"),
            (New-DocxParagraphXml -Text "Repository: $Repository"),
            (New-DocxParagraphXml -Text "Guest MSI Path: $GuestMsiPath"),
            (New-DocxParagraphXml -Text "This packet is for the human installed-app beta pass. It does not launch the app, install or uninstall MSI packages, run GUI automation, mutate Hyper-V, or use active WHITEDRAGON for validation."),
            (New-DocxParagraphXml -Text '3. Manual Installed-App QA Checklist' -Style Heading1),
            (New-DocxParagraphXml -Text 'Complete every row. Record exactly one result per test: Pass, Fail, Blocked, or Not Applicable. For Fail/Blocked, add notes and evidence.' -Style Instruction),
            (New-DocxTableXml -Headers @('#', 'Area', 'Test', 'Expected Result', 'Result', 'Reviewer Notes', 'Evidence / Screenshot') -Rows $checklistRows -ColumnWidths @(500, 1100, 3100, 4100, 1300, 1600, 1500)),
            (New-DocxParagraphXml -Text '4. Beta Blocker Watchlist' -Style Heading1),
            (New-DocxParagraphXml -Text 'Any confirmed blocker listed below must be resolved before final sign-off. Tick the box when the category has been verified as clear. Leave unticked if the issue is open or untested.'),
            (New-DocxTableXml -Headers @('Status', 'Blocker Category', 'Description') -Rows $blockerRows -ColumnWidths @(900, 3600, 8700)),
            (New-DocxParagraphXml -Text '5. Final Review Decision' -Style Heading1),
            (New-DocxParagraphXml -Text 'Select exactly one decision below. Complete all sign-off fields before submitting this packet.'),
            (New-DocxTableXml -Headers @('Decision', 'Meaning', 'Select One') -Rows @(
                    @('Ready for private beta handoff', 'No blocking manual QA failures found', ''),
                    @('Ready with known non-blocking issues', 'Issues are documented and acceptable for private beta', ''),
                    @('Blocked', 'One or more beta blockers must be fixed before handoff', '')
                ) -ColumnWidths @(4700, 6900, 1600)),
            (New-DocxTableXml -Headers @('Field', 'Value') -Rows @(
                    @('Reviewer Name', ''),
                    @('Date', ''),
                    @('Summary Notes', ''),
                    @('Follow-up Issue Links or File Paths', '')
                ) -ColumnWidths @(3200, 10000))
        ) -join ''
        $documentXml = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>' + $body + '<w:sectPr><w:pgSz w:w="15840" w:h="12240" w:orient="landscape"/><w:pgMar w:top="720" w:right="720" w:bottom="720" w:left="720" w:header="720" w:footer="720" w:gutter="0"/></w:sectPr></w:body></w:document>'
        $documentXml | Set-Content -LiteralPath (Join-Path $wordDir 'document.xml') -Encoding UTF8

        $created = (Get-Date).ToUniversalTime().ToString('s') + 'Z'
        ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?><cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><dc:title>Lisan Studio Manual Beta QA</dc:title><dc:creator>Codex</dc:creator><dcterms:created xsi:type="dcterms:W3CDTF">' + $created + '</dcterms:created></cp:coreProperties>') |
            Set-Content -LiteralPath (Join-Path $docPropsDir 'core.xml') -Encoding UTF8

        if (Test-Path -LiteralPath $Path) {
            Remove-Item -LiteralPath $Path -Force
        }
        Get-ChildItem -LiteralPath $tempDocxRoot -Recurse -File -Include '*.xml', '*.rels' | ForEach-Object {
            $bytes = [System.IO.File]::ReadAllBytes($_.FullName)
            if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
                $stripped = New-Object byte[] ($bytes.Length - 3)
                [Array]::Copy($bytes, 3, $stripped, 0, $stripped.Length)
                [System.IO.File]::WriteAllBytes($_.FullName, $stripped)
            }
        }
        $zipStream = [System.IO.File]::Open($Path, [System.IO.FileMode]::CreateNew)
        try {
            $zipArchive = New-Object System.IO.Compression.ZipArchive($zipStream, [System.IO.Compression.ZipArchiveMode]::Create, $false)
            try {
                Get-ChildItem -LiteralPath $tempDocxRoot -Recurse -File | Sort-Object FullName | ForEach-Object {
                    $relativePath = $_.FullName.Substring($tempDocxRoot.Length).TrimStart('\', '/')
                    $entryName = $relativePath -replace '\\', '/'
                    $entry = $zipArchive.CreateEntry($entryName, [System.IO.Compression.CompressionLevel]::Optimal)
                    $entryStream = $entry.Open()
                    try {
                        $sourceStream = [System.IO.File]::OpenRead($_.FullName)
                        try {
                            $sourceStream.CopyTo($entryStream)
                        } finally {
                            $sourceStream.Dispose()
                        }
                    } finally {
                        $entryStream.Dispose()
                    }
                }
            } finally {
                $zipArchive.Dispose()
            }
        } finally {
            $zipStream.Dispose()
        }
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

$checklistSpecs = @(
    @{ Area = 'Launch'; Test = 'Start Menu launch opens Lisan Studio'; ExpectedResult = 'App opens without crash and shows Lisan Studio branding' },
    @{ Area = 'Launch'; Test = 'Desktop shortcut launch opens Lisan Studio'; ExpectedResult = 'Shortcut targets the installed LisanStudio.exe, not any legacy app' },
    @{ Area = 'Shell / RTL'; Test = 'Open the installed sample project and verify the RTL shell'; ExpectedResult = 'Top shell, project sidebar, editor tabs, bottom panel, and status bar appear RTL and readable' },
    @{ Area = 'Project'; Test = 'Open a real `.apy` folder from the project sidebar'; ExpectedResult = 'Project tree loads file names cleanly and remains RTL' },
    @{ Area = 'Editing'; Test = 'Arabic mixed-direction edit, save, close, and reopen preserves text'; ExpectedResult = 'Arabic, English, numbers, paths, and punctuation survive save/reopen without corruption' },
    @{ Area = 'Editing'; Test = 'Cursor, selection, copy, paste, undo, redo, backspace, and delete behave around Arabic text'; ExpectedResult = 'No cursor jumps, selection corruption, or text loss around mixed-direction text' },
    @{ Area = 'Search'; Test = 'Project search opens a clicked file and line'; ExpectedResult = 'Search results are readable and clicking a result opens the correct file/line' },
    @{ Area = 'Problems'; Test = 'Problems panel reports an inserted hidden BiDi control'; ExpectedResult = 'Hidden BiDi diagnostic appears with useful file/line detail' },
    @{ Area = 'Runtime'; Test = 'Run current `.apy` shows readable UTF-8 Arabic output'; ExpectedResult = 'Output panel shows Arabic text without mojibake' },
    @{ Area = 'Runtime'; Test = 'Output panel shows command, working directory, exit code, elapsed time, and cancel behavior'; ExpectedResult = 'Runtime feedback is structured and readable' },
    @{ Area = 'Settings'; Test = 'Settings opens, runtime diagnostics are readable, and editor font changes persist after restart'; ExpectedResult = 'Settings dialog is RTL, diagnostics are clear, and font preference persists' },
    @{ Area = 'Installer'; Test = 'Uninstall removes app payload and shortcuts'; ExpectedResult = 'Installed payload and shortcuts are removed; user settings are not treated as MSI payload' },
    @{ Area = 'Installer'; Test = 'Reinstall from the same beta MSI succeeds without manual PATH or Python setup'; ExpectedResult = 'Reinstall succeeds cleanly and app runs using bundled runtime' }
)
$checklist = $checklistSpecs | ForEach-Object {
    [PSCustomObject]@{
        area = $_.Area
        test = $_.Test
        expectedResult = $_.ExpectedResult
        status = 'Not Recorded'
        notes = ''
    }
}

$blockerWatchlist = @(
    @{ Status = ''; Category = 'Arabic output mojibake'; Description = 'Arabic text appears as garbled characters in the output panel.' },
    @{ Status = ''; Category = 'Cursor or selection corruption'; Description = 'Cursor jumps, incorrect selection, or text loss around mixed-direction content.' },
    @{ Status = ''; Category = 'Save / open data loss'; Description = 'Any content lost or corrupted between save and reopen.' },
    @{ Status = ''; Category = 'Crash on launch, run, close, install, or uninstall'; Description = 'Any unhandled exception or process termination.' },
    @{ Status = ''; Category = 'MSI requiring manual PATH or system Python setup'; Description = 'Installation must succeed using the bundled runtime only.' },
    @{ Status = ''; Category = 'Shortcuts pointing to wrong or legacy app'; Description = 'Start Menu and desktop shortcuts must target the installed LisanStudio.exe.' },
    @{ Status = ''; Category = 'Missing Qt, Python, or lughat-althuban license payloads'; Description = 'All required license files must be present in the installed payload.' },
    @{ Status = ''; Category = 'Hidden BiDi controls inserted by the editor'; Description = 'The editor must not silently insert Unicode BiDi control characters.' },
    @{ Status = ''; Category = 'RTL shell regression'; Description = 'Any panel, sidebar, or status bar rendering in LTR when it should be RTL.' },
    @{ Status = ''; Category = 'App launching on WHITEDRAGON instead of isolated QA VM'; Description = 'All GUI/MSI testing must remain within LisanStudio-QA.' }
) | ForEach-Object {
    [PSCustomObject]@{
        status = $_.Status
        category = $_.Category
        description = $_.Description
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
$checklistLines = @("| # | Area | Test | Expected Result | Result | Reviewer Notes | Evidence / Screenshot |", "| --- | --- | --- | --- | --- | --- | --- |")
$rowNumber = 0
$checklistLines += $checklist | ForEach-Object {
    ++$rowNumber
    "| $rowNumber | $($_.area) | $($_.test) | $($_.expectedResult) | $($_.status) | $($_.notes) |  |"
}
$blockerLines = @("| Status | Blocker Category | Description |", "| --- | --- | --- |")
$blockerLines += $blockerWatchlist | ForEach-Object { "| $($_.status) | $($_.category) | $($_.description) |" }

$markdownLines = @(
    "# Lisan Studio $ReleaseLabel Manual QA Review Packet",
    "",
    "Generated: $generatedAt",
    "Repository: $repo",
    "Manual QA Status: $manualQaStatus",
    "Guest MSI Path: $GuestMsiPath",
    "",
    "This report is an installed-app checklist for a human beta pass. It does not launch the app, install or uninstall MSI packages, run GUI automation, mutate Hyper-V, or use active WHITEDRAGON for validation.",
    "",
    "## 1. Automated Evidence Summary",
    "",
    $artifactLines,
    "",
    "## 2. Reviewer Instructions",
    "",
    "- Perform all testing exclusively inside the LisanStudio-QA isolated VM.",
    "- Do not install, run, or uninstall the MSI on active WHITEDRAGON.",
    "- Record exactly one result per checklist item: Pass, Fail, Blocked, or Not Applicable.",
    "- Add notes and evidence paths for every Fail or Blocked item.",
    "",
    "## 3. Manual Installed-App QA Checklist",
    "",
    $checklistLines,
    "",
    "## 4. Beta Blocker Watchlist",
    "",
    $blockerLines,
    "",
    "## 5. Final Review Decision",
    "",
    "| Decision | Meaning | Select One |",
    "| --- | --- | --- |",
    "| Ready for private beta handoff | No blocking manual QA failures found |  |",
    "| Ready with known non-blocking issues | Issues are documented and acceptable for private beta |  |",
    "| Blocked | One or more beta blockers must be fixed before handoff |  |",
    "",
    "| Field | Value |",
    "| --- | --- |",
    "| Reviewer Name |  |",
    "| Date |  |",
    "| Summary Notes |  |",
    "| Follow-up Issue Links or File Paths |  |",
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
    -GuestMsiPath $GuestMsiPath `
    -Artifacts $artifacts `
    -Checklist $checklist `
    -BlockerWatchlist $blockerWatchlist `
    -TemplatePath $TemplatePath

$result = [PSCustomObject]@{
    ok = $true
    generatedAt = $generatedAt
    releaseLabel = $ReleaseLabel
    releaseDir = $ReleaseDir
    msiPath = $MsiPath
    guestMsiPath = $GuestMsiPath
    outputDirectory = $outputDirectory
    markdownPath = $markdownPath
    wordPath = $wordPath
    jsonPath = $jsonPath
    templatePath = $TemplatePath
    manualQaStatus = $manualQaStatus
    artifacts = $artifacts
    checklist = $checklist
    blockerWatchlist = $blockerWatchlist
}
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonPath -Encoding UTF8

if ($Json) {
    $result | ConvertTo-Json -Depth 6
} else {
    $result
}
