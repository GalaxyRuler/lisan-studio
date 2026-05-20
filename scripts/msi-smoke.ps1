param(
    [string]$ProductVersion = "0.1.0",
    [string]$MsiPath = "",
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA "LisanStudio"),
    [switch]$KeepInstalled
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($MsiPath)) {
    $MsiPath = Join-Path (Join-Path $PSScriptRoot "..\artifacts") "LisanStudio-$ProductVersion-beta.msi"
}
$MsiPath = (Resolve-Path $MsiPath).Path
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$smokeLogDir = Join-Path $repo "artifacts\msi-smoke"
New-Item -ItemType Directory -Force -Path $smokeLogDir | Out-Null

$installLog = Join-Path $smokeLogDir "install.log"
$uninstallLog = Join-Path $smokeLogDir "uninstall.log"
$precleanLog = Join-Path $smokeLogDir "preclean-uninstall.log"
$startMenuFolder = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\Lisan Studio"
$startMenuShortcut = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\Lisan Studio\Lisan Studio.lnk"
$desktopShortcut = Join-Path ([Environment]::GetFolderPath("Desktop")) "Lisan Studio.lnk"

function Invoke-Msi {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][int[]]$AllowedExitCodes,
        [Parameter(Mandatory = $true)][string]$Operation
    )

    $process = Start-Process -FilePath "msiexec.exe" -ArgumentList $Arguments -Wait -PassThru -WindowStyle Hidden
    if ($AllowedExitCodes -notcontains $process.ExitCode) {
        throw "$Operation failed with msiexec exit code $($process.ExitCode)."
    }
    return $process.ExitCode
}

function Remove-CleanInstallPayload {
    $resolvedInstallParent = (Resolve-Path $env:LOCALAPPDATA).Path
    $targetFull = [System.IO.Path]::GetFullPath($InstallRoot)
    if (-not $targetFull.StartsWith($resolvedInstallParent, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove unexpected install root: $targetFull"
    }
    if (Test-Path -LiteralPath $targetFull) {
        Remove-Item -LiteralPath $targetFull -Recurse -Force
    }
}

function Get-InstalledLisanProductCodes {
    $installer = New-Object -ComObject WindowsInstaller.Installer
    foreach ($product in @($installer.ProductsEx("", "", 7))) {
        if ($product.InstallProperty("ProductName") -eq "Lisan Studio") {
            $product.ProductCode()
        }
    }
}

function Get-RegistryValueOrDefault {
    param(
        [Parameter(Mandatory = $true)]$Entry,
        [Parameter(Mandatory = $true)][string]$Name,
        [string]$Default = ''
    )

    if ($null -eq $Entry) {
        return $Default
    }

    $prop = $Entry.PSObject.Properties[$Name]
    if ($prop) {
        return [string]$prop.Value
    }

    return $Default
}

function Get-LisanUninstallRegistryEntries {
    $lisanUpgradeCode = "{B45833FC-87F8-4656-8CC4-DC87769EA386}"
    $uninstallRoots = @(
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
    )

    foreach ($root in $uninstallRoots) {
        if (-not (Test-Path -LiteralPath $root)) {
            continue
        }
        foreach ($key in @(Get-ChildItem -LiteralPath $root -ErrorAction SilentlyContinue)) {
            $entry = Get-ItemProperty -LiteralPath $key.PSPath -ErrorAction SilentlyContinue
            $entryDisplayName = Get-RegistryValueOrDefault -Entry $entry -Name 'DisplayName'
            $entryDisplayVersion = Get-RegistryValueOrDefault -Entry $entry -Name 'DisplayVersion'
            $entryUpgradeCode = Get-RegistryValueOrDefault -Entry $entry -Name 'UpgradeCode'
            $entryPublisher = Get-RegistryValueOrDefault -Entry $entry -Name 'Publisher'
            $entryInstallLocation = Get-RegistryValueOrDefault -Entry $entry -Name 'InstallLocation'
            $entryDisplayIcon = Get-RegistryValueOrDefault -Entry $entry -Name 'DisplayIcon'
            $entryUninstallString = Get-RegistryValueOrDefault -Entry $entry -Name 'UninstallString'
            $entryQuietUninstallString = Get-RegistryValueOrDefault -Entry $entry -Name 'QuietUninstallString'
            $matchesDisplayName = [string]::Equals($entryDisplayName, 'Lisan Studio', [System.StringComparison]::OrdinalIgnoreCase)
            $matchesUpgradeCode = [string]::Equals($entryUpgradeCode, $lisanUpgradeCode, [System.StringComparison]::OrdinalIgnoreCase)
            if ($matchesDisplayName -or $matchesUpgradeCode) {
                [PSCustomObject]@{
                    RegistryPath = $key.Name
                    DisplayName = $entryDisplayName
                    DisplayVersion = $entryDisplayVersion
                    UpgradeCode = $entryUpgradeCode
                    Publisher = $entryPublisher
                    InstallLocation = $entryInstallLocation
                    DisplayIcon = $entryDisplayIcon
                    UninstallString = $entryUninstallString
                    QuietUninstallString = $entryQuietUninstallString
                }
            }
        }
    }
}

function Assert-LisanUninstallRegistryEntry {
    $entries = @(Get-LisanUninstallRegistryEntries)
    if ($entries.Count -ne 1) {
        throw "Install should leave exactly one Windows Apps uninstall entry for Lisan Studio; found $($entries.Count): $($entries.RegistryPath -join ', ')"
    }

    $entry = $entries[0]
    $expectedHive = 'HKEY_LOCAL_MACHINE\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
    if (-not $entry.RegistryPath.StartsWith($expectedHive, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Install uninstall registry entry should be under $expectedHive; offending path: $($entry.RegistryPath)"
    }
    if (-not [string]::Equals($entry.DisplayName, 'Lisan Studio', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Windows Apps uninstall entry should have DisplayName 'Lisan Studio': $($entry.RegistryPath)"
    }
    if (-not $entry.DisplayVersion) {
        throw "Windows Apps uninstall entry missing DisplayVersion: $($entry.RegistryPath)"
    }
    if ($entry.UninstallString -notmatch 'msiexec(\.exe)?') {
        throw "UninstallString should reference msiexec: $($entry.RegistryPath)"
    }
    if ($entry.QuietUninstallString -and $entry.QuietUninstallString -notmatch 'msiexec(\.exe)?') {
        throw "QuietUninstallString should reference msiexec: $($entry.RegistryPath)"
    }
    if ($entry.InstallLocation -and -not $entry.InstallLocation.StartsWith($InstallRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "InstallLocation should point at the Lisan install root: $($entry.InstallLocation)"
    }

    return $entries
}

Get-Process LisanStudio,ArabicCodeStudioQt -ErrorAction SilentlyContinue | Stop-Process -Force

foreach ($productCode in @(Get-InstalledLisanProductCodes)) {
    Invoke-Msi -Operation "Pre-clean uninstall $productCode" -AllowedExitCodes @(0, 1605, 1614, 3010) -Arguments @(
        "/x", $productCode,
        "/qn",
        "/norestart",
        "/L*v", "`"$precleanLog`""
    ) | Out-Null
}

Remove-CleanInstallPayload
Remove-Item -LiteralPath $startMenuShortcut -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $desktopShortcut -Force -ErrorAction SilentlyContinue
if (Test-Path -LiteralPath $startMenuFolder) {
    $remainingStartMenuItems = Get-ChildItem -LiteralPath $startMenuFolder -Force -ErrorAction SilentlyContinue
    if (-not $remainingStartMenuItems) {
        Remove-Item -LiteralPath $startMenuFolder -Force
    }
}

Invoke-Msi -Operation "Install" -AllowedExitCodes @(0, 3010) -Arguments @(
    "/i", "`"$MsiPath`"",
    "/qn",
    "/norestart",
    "/L*v", "`"$installLog`""
) | Out-Null

$app = Join-Path $InstallRoot "LisanStudio.exe"
$requiredPayloadFiles = @(
    $app,
    (Join-Path $InstallRoot "README.md"),
    (Join-Path $InstallRoot "RELEASE_NOTES.md"),
    (Join-Path $InstallRoot "BETA_VALIDATION.md"),
    (Join-Path $InstallRoot "LICENSES.md"),
    (Join-Path $InstallRoot "licenses\Python-LICENSE.txt"),
    (Join-Path $InstallRoot "licenses\lughat-althuban-LICENSE"),
    (Join-Path $InstallRoot "licenses\qt6-base\LGPL-3.0-only.txt"),
    (Join-Path $InstallRoot "libgcc_s_seh-1.dll"),
    (Join-Path $InstallRoot "libstdc++-6.dll"),
    (Join-Path $InstallRoot "libwinpthread-1.dll"),
    (Join-Path $InstallRoot "libicuin78.dll"),
    (Join-Path $InstallRoot "libicuuc78.dll"),
    (Join-Path $InstallRoot "libicudt78.dll"),
    (Join-Path $InstallRoot "runtime\python\python.exe")
)

foreach ($file in $requiredPayloadFiles) {
    if (-not (Test-Path -LiteralPath $file)) {
        throw "Installed payload file missing: $file"
    }
}
if (-not (Test-Path -LiteralPath $startMenuShortcut)) {
    throw "Start Menu shortcut missing: $startMenuShortcut"
}
if (-not (Test-Path -LiteralPath $desktopShortcut)) {
    throw "Desktop shortcut missing: $desktopShortcut"
}
$uninstallRegistryEntries = @(Assert-LisanUninstallRegistryEntry)

& (Join-Path $PSScriptRoot "installed-smoke.ps1") -InstallRoot $InstallRoot

if (-not $KeepInstalled) {
    Get-Process LisanStudio,ArabicCodeStudioQt -ErrorAction SilentlyContinue | Stop-Process -Force
    Invoke-Msi -Operation "Uninstall" -AllowedExitCodes @(0, 3010) -Arguments @(
        "/x", "`"$MsiPath`"",
        "/qn",
        "/norestart",
        "/L*v", "`"$uninstallLog`""
    ) | Out-Null

    if (Test-Path -LiteralPath $app) {
        throw "Uninstall left app executable behind: $app"
    }
    if (Test-Path -LiteralPath $startMenuShortcut) {
        throw "Uninstall left Start Menu shortcut behind: $startMenuShortcut"
    }
    if (Test-Path -LiteralPath $desktopShortcut) {
        throw "Uninstall left Desktop shortcut behind: $desktopShortcut"
    }
    $remainingUninstallRegistryEntries = @(Get-LisanUninstallRegistryEntries)
    if ($remainingUninstallRegistryEntries.Count -gt 0) {
        throw "Uninstall left Windows Apps uninstall entry behind: $($remainingUninstallRegistryEntries.RegistryPath -join ', ')"
    }
}

[PSCustomObject]@{
    Msi = $MsiPath
    InstallRoot = $InstallRoot
    KeptInstalled = [bool]$KeepInstalled
    InstallLog = $installLog
    PrecleanLog = $precleanLog
    UninstallLog = $uninstallLog
    StartMenuShortcut = $startMenuShortcut
    DesktopShortcut = $desktopShortcut
    UninstallRegistryEntries = $uninstallRegistryEntries
}
