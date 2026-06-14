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
        'Package',
        'Name="Lisan Studio"',
        'Manufacturer="Lisan Studio"',
        'Version="$(var.ProductVersion)"',
        'UpgradeCode="b45833fc-87f8-4656-8cc4-dc87769ea386"',
        'Scope="perUser"',
        'MajorUpgrade',
        'DowngradeErrorMessage="A newer Lisan Studio is already installed."',
        'ComponentGroupRef Id="ApplicationFiles"',
        'ComponentRef Id="StartMenuShortcut"',
        'ComponentRef Id="DesktopShortcut"',
        'RegistryValue Root="HKCU" Key="Software\LisanStudio" Name="BuildId"',
        'RegistryValue Root="HKCU" Key="Software\LisanStudio" Name="installed"'
    )) {
    if ($source -notmatch [regex]::Escape($requiredToken)) {
        throw "LisanStudio.wxs should declare uninstall registration token: $requiredToken"
    }
}

"Test-WixUninstallRegistration.ps1 passed"
