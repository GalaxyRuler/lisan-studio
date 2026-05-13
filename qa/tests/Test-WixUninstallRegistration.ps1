[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$wxsPath = Join-Path $repoRoot 'packaging\wix\LisanStudio.wxs'

if (-not (Test-Path -LiteralPath $wxsPath)) {
    throw "Missing WiX source: $wxsPath"
}

$source = Get-Content -Raw -LiteralPath $wxsPath

foreach ($requiredToken in @(
        'Component Id="UninstallRegistryEntry"',
        'Software\Microsoft\Windows\CurrentVersion\Uninstall\LisanStudio',
        'Name="DisplayName"',
        'Name="DisplayVersion"',
        'Name="Publisher"',
        'Name="InstallLocation"',
        'Name="DisplayIcon"',
        'Name="UninstallString"',
        'Name="QuietUninstallString"',
        'msiexec.exe /x [ProductCode]',
        'ComponentRef Id="UninstallRegistryEntry"'
    )) {
    if ($source -notmatch [regex]::Escape($requiredToken)) {
        throw "LisanStudio.wxs should declare uninstall registration token: $requiredToken"
    }
}

"Test-WixUninstallRegistration.ps1 passed"
