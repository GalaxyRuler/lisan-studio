param(
    [string]$MsiPath = (Join-Path (Join-Path $PSScriptRoot "..\artifacts") "LisanStudio-0.1.0-beta.msi"),
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA "LisanStudio"),
    [switch]$KeepInstalled
)

$ErrorActionPreference = "Stop"

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

function Get-LisanUninstallRegistryEntries {
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
            if ($entry.DisplayName -eq 'Lisan Studio') {
                [PSCustomObject]@{
                    RegistryPath = $key.Name
                    DisplayName = [string]$entry.DisplayName
                    DisplayVersion = [string]$entry.DisplayVersion
                    Publisher = [string]$entry.Publisher
                    InstallLocation = [string]$entry.InstallLocation
                    DisplayIcon = [string]$entry.DisplayIcon
                    UninstallString = [string]$entry.UninstallString
                    QuietUninstallString = [string]$entry.QuietUninstallString
                }
            }
        }
    }
}

function Assert-LisanUninstallRegistryEntry {
    $entries = @(Get-LisanUninstallRegistryEntries)
    if ($entries.Count -eq 0) {
        throw 'Windows Apps uninstall entry missing for Lisan Studio.'
    }

    foreach ($entry in $entries) {
        if ($entry.UninstallString -notmatch 'msiexec(\.exe)?') {
            throw "UninstallString should reference msiexec: $($entry.RegistryPath)"
        }
    }

    $projectEntry = @($entries | Where-Object { $_.RegistryPath -like '*\Uninstall\LisanStudio' }) | Select-Object -First 1
    if (-not $projectEntry) {
        throw 'Windows Apps project uninstall entry missing: HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\LisanStudio'
    }
    if (-not $projectEntry.DisplayVersion) {
        throw "Windows Apps project uninstall entry missing DisplayVersion: $($projectEntry.RegistryPath)"
    }
    if ($projectEntry.QuietUninstallString -notmatch 'msiexec(\.exe)?') {
        throw "QuietUninstallString should reference msiexec: $($projectEntry.RegistryPath)"
    }
    if ($projectEntry.InstallLocation -and -not $projectEntry.InstallLocation.StartsWith($InstallRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "InstallLocation should point at the Lisan install root: $($projectEntry.InstallLocation)"
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
