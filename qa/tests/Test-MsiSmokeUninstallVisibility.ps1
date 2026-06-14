[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$msiSmokeScript = Join-Path $repoRoot 'scripts\msi-smoke.ps1'

if (-not (Test-Path -LiteralPath $msiSmokeScript)) {
    throw "Missing MSI smoke script: $msiSmokeScript"
}

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($msiSmokeScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "msi-smoke.ps1 parse failed: $($parseErrors[0].Message)"
}

$source = Get-Content -Raw -LiteralPath $msiSmokeScript

foreach ($requiredToken in @(
        'function Get-LisanUninstallRegistryEntries',
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'DisplayName',
        'DisplayVersion',
        'UninstallString',
        'QuietUninstallString',
        'Install should leave exactly one Windows Apps uninstall entry for Lisan Studio',
        'Windows Apps uninstall entry missing DisplayVersion',
        'HKEY_LOCAL_MACHINE\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall',
        'UninstallString should reference msiexec',
        'QuietUninstallString should reference msiexec',
        'InstallLocation should point at the Lisan install root',
        'UninstallRegistryEntries'
    )) {
    if ($source -notmatch [regex]::Escape($requiredToken)) {
        throw "msi-smoke.ps1 should validate uninstall visibility token: $requiredToken"
    }
}

"Test-MsiSmokeUninstallVisibility.ps1 passed"
