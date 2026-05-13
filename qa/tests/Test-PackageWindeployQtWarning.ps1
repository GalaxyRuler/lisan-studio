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

if ($source -notmatch 'function\s+Invoke-NativeToolAllowingStderr') {
    throw 'package.ps1 should wrap native tools that may write non-fatal stderr.'
}

if ($source -notmatch '\$ErrorActionPreference\s*=\s*''Continue''') {
    throw 'native stderr wrapper should temporarily use ErrorActionPreference=Continue.'
}

if ($source -notmatch '\$LASTEXITCODE') {
    throw 'native stderr wrapper should inspect LASTEXITCODE instead of treating stderr as failure.'
}

if ($source -notmatch 'Invoke-NativeToolAllowingStderr\s+-FilePath\s+\$windeployqt') {
    throw 'windeployqt should be invoked through the native stderr wrapper.'
}

$directPipMatches = @($source | Select-String -Pattern '&\s+\$python\s+-m\s+pip' -AllMatches)
$directPipCount = 0
foreach ($matchInfo in $directPipMatches) {
    $directPipCount += $matchInfo.Matches.Count
}
if ($directPipCount -gt 0) {
    throw 'package.ps1 should not shell out to pip during isolated packaging.'
}

foreach ($offlineToken in @('function Install-ApythonRuntimeOffline', 'lughat_althuban-', 'METADATA')) {
    if ($source -notmatch [regex]::Escape($offlineToken)) {
        throw "package.ps1 should keep offline apython runtime staging token: $offlineToken"
    }
}

foreach ($nativeRuntimeToken in @(
        'function Copy-RequiredNativeRuntimeDlls',
        'ldd',
        '/ucrt64/bin/',
        'TargetExecutable',
        'libgcc_s_seh-1.dll',
        'libstdc++-6.dll',
        'libwinpthread-1.dll',
        'Required native runtime DLL missing'
    )) {
    if ($source -notmatch [regex]::Escape($nativeRuntimeToken)) {
        throw "package.ps1 should stage required native runtime DLL token: $nativeRuntimeToken"
    }
}

"Test-PackageWindeployQtWarning.ps1 passed"
