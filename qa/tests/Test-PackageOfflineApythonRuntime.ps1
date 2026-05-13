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
[void][System.Management.Automation.Language.Parser]::ParseFile($packageScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "package.ps1 parse failed: $($parseErrors[0].Message)"
}

$source = Get-Content -Raw -LiteralPath $packageScript

foreach ($networkToken in @("'--upgrade'", "'pip'", "'wheel'", "'setuptools'")) {
    if ($source -match [regex]::Escape($networkToken)) {
        throw "package.ps1 should not run network-dependent pip bootstrap token: $networkToken"
    }
}

foreach ($requiredToken in @(
        'function Install-ApythonRuntimeOffline',
        'function Assert-StagedApythonRuntime',
        'function Copy-DirectoryContents',
        'lughat_althuban-',
        'METADATA',
        'entry_points.txt',
        'top_level.txt',
        'arabicpython',
        'arabicpython\cli.py',
        'arabicpython_kernel',
        'arabicpython_kernel\__init__.py',
        'imported from outside staged runtime'
    )) {
    if ($source -notmatch [regex]::Escape($requiredToken)) {
        throw "package.ps1 should stage offline apython runtime metadata/token: $requiredToken"
    }
}

if ($source -match 'pip\s+install') {
    throw 'package.ps1 should not use pip install for the bundled apython runtime.'
}

if ($source -match 'pip\s+wheel') {
    throw 'package.ps1 should not use pip wheel for the bundled apython runtime.'
}

"Test-PackageOfflineApythonRuntime.ps1 passed"
