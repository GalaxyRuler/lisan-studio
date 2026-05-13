[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$packageScript = Join-Path $repoRoot 'scripts\package.ps1'

if (-not (Test-Path -LiteralPath $packageScript)) {
    throw "Missing package script: $packageScript"
}

$tokens = $null
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($packageScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "package.ps1 parse failed: $($parseErrors[0].Message)"
}

$functionAst = $ast.Find({
    param($node)
    $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and
        $node.Name -eq 'Install-ApythonRuntimeOffline'
}, $true)

if (-not $functionAst) {
    throw 'Install-ApythonRuntimeOffline function missing.'
}

$source = $functionAst.Extent.Text

if ($source -match 'Copy-Item\s+-LiteralPath\s+\$sourcePackage') {
    throw 'Apython package copy should not copy the source package container directly because Copy-Item directory semantics can flatten it.'
}

if ($source -match 'Copy-Item\s+-Path\s+\$sourcePackageContents') {
    throw 'Apython package copy should not rely on wildcard Copy-Item for package contents in the VM lane.'
}

if ($source -notmatch 'Copy-DirectoryContents\s+-Source\s+\$sourcePackage\s+-Destination\s+\$destinationPackage') {
    throw 'Apython package copy should mirror each package directory into the explicit destination package directory.'
}

if ($source -notmatch 'Apython package source file missing') {
    throw 'Apython package copy should verify concrete source files before copying.'
}

if ($source -notmatch 'Join-Path\s+\$DestinationSitePackages\s+\$packageName') {
    throw 'Apython package copy should still clean existing package directories before copying.'
}

$scriptSource = Get-Content -Raw -LiteralPath $packageScript
if ($scriptSource -notmatch 'function\s+Copy-DirectoryContents') {
    throw 'package.ps1 should define a deterministic directory contents copy helper.'
}

foreach ($requiredCopyToken in @(
        'Get-ChildItem -LiteralPath $sourceRoot -Recurse -Force -File',
        '$_.FullName -notmatch ''\\__pycache__\\''',
        '$_.Extension -notin @(''.pyc'', ''.pyo'')',
        '$relativePath = $file.FullName.Substring($sourceRoot.Length).TrimStart(''\'')',
        'Apython package destination file missing after copy'
    )) {
    if ($scriptSource -notmatch [regex]::Escape($requiredCopyToken)) {
        throw "package.ps1 should deterministically copy Apython files token: $requiredCopyToken"
    }
}

"Test-PackageOfflineApythonCopyShape.ps1 passed"
