param(
    [Parameter(Mandatory = $true)]
    [string]$EarlierMsiPath,

    [Parameter(Mandatory = $true)]
    [string]$ReplacementMsiPath,

    [string]$DowngradeMsiPath = "",
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA "LisanStudio"),
    [string]$ExpectedEarlierVersion = "",
    [string]$ExpectedReplacementVersion = "",
    [switch]$SameVersionReplace,
    [switch]$KeepInstalled,
    [switch]$AllowMutation,
    [switch]$IUnderstandThisRunsMsiUpgrade
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = "Stop"

if (-not $AllowMutation -or -not $IUnderstandThisRunsMsiUpgrade) {
    throw "MSI upgrade smoke installs, replaces, and uninstalls packages. Pass -AllowMutation and -IUnderstandThisRunsMsiUpgrade from an approved VM lane."
}

if ([string]::Equals([string]$env:COMPUTERNAME, "WHITEDRAGON", [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to run MSI upgrade smoke on active WHITEDRAGON. Use the LisanStudio-QA Homelab route."
}

$EarlierMsiPath = (Resolve-Path -LiteralPath $EarlierMsiPath).Path
$ReplacementMsiPath = (Resolve-Path -LiteralPath $ReplacementMsiPath).Path
if (-not [string]::IsNullOrWhiteSpace($DowngradeMsiPath)) {
    $DowngradeMsiPath = (Resolve-Path -LiteralPath $DowngradeMsiPath).Path
}

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$smokeLogDir = Join-Path $repo "artifacts\msi-upgrade-smoke"
New-Item -ItemType Directory -Force -Path $smokeLogDir | Out-Null

$precleanLog = Join-Path $smokeLogDir "preclean-uninstall.log"
$earlierInstallLog = Join-Path $smokeLogDir "install-earlier.log"
$replacementInstallLog = Join-Path $smokeLogDir "install-replacement.log"
$downgradeAttemptLog = Join-Path $smokeLogDir "downgrade-attempt.log"
$uninstallLog = Join-Path $smokeLogDir "uninstall.log"
$startMenuFolder = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\Lisan Studio"
$startMenuShortcut = Join-Path $startMenuFolder "Lisan Studio.lnk"
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

function Invoke-MsiExpectFailure {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][int[]]$UnexpectedSuccessExitCodes,
        [Parameter(Mandatory = $true)][string]$Operation
    )

    $process = Start-Process -FilePath "msiexec.exe" -ArgumentList $Arguments -Wait -PassThru -WindowStyle Hidden
    if ($UnexpectedSuccessExitCodes -contains $process.ExitCode) {
        throw "$Operation failed: Downgrade unexpectedly succeeded with msiexec exit code $($process.ExitCode)."
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
            $entryBuildId = Get-RegistryValueOrDefault -Entry $entry -Name 'BuildId'
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
                    BuildId = $entryBuildId
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

function Assert-SingleLisanUninstallRegistryEntry {
    param(
        [string]$ExpectedVersion,
        [string]$Context
    )

    $entries = @(Get-LisanUninstallRegistryEntries)
    if ($entries.Count -ne 1) {
        throw "$Context should leave exactly one Windows Apps uninstall entry for Lisan Studio; found $($entries.Count): $($entries.RegistryPath -join ', ')"
    }

    $entry = $entries[0]
    $expectedHive = 'HKEY_LOCAL_MACHINE\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
    if (-not $entry.RegistryPath.StartsWith($expectedHive, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "$Context uninstall registry entry should be under $expectedHive; offending path: $($entry.RegistryPath)"
    }
    if ($entry.UninstallString -notmatch 'msiexec(\.exe)?') {
        throw "$Context UninstallString should reference msiexec: $($entry.RegistryPath)"
    }
    if ($entry.QuietUninstallString -and $entry.QuietUninstallString -notmatch 'msiexec(\.exe)?') {
        throw "$Context QuietUninstallString should reference msiexec: $($entry.RegistryPath)"
    }
    if ($entry.InstallLocation -and -not $entry.InstallLocation.StartsWith($InstallRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "$Context InstallLocation should point at the Lisan install root: $($entry.InstallLocation)"
    }
    if ($ExpectedVersion -and $entry.DisplayVersion -ne $ExpectedVersion) {
        throw "$Context DisplayVersion should be $ExpectedVersion, got $($entry.DisplayVersion)"
    }

    return $entry
}

function Assert-InstalledPayload {
    $app = Join-Path $InstallRoot "LisanStudio.exe"
    foreach ($file in @(
            $app,
            (Join-Path $InstallRoot "README.md"),
            (Join-Path $InstallRoot "RELEASE_NOTES.md"),
            (Join-Path $InstallRoot "BETA_VALIDATION.md"),
            (Join-Path $InstallRoot "LICENSES.md"),
            (Join-Path $InstallRoot "runtime\python\python.exe")
        )) {
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

Invoke-Msi -Operation 'Install earlier MSI' -AllowedExitCodes @(0, 3010) -Arguments @(
    "/i", "`"$EarlierMsiPath`"",
    "/qn",
    "/norestart",
    "/L*v", "`"$earlierInstallLog`""
) | Out-Null
$earlierRegistryEntry = Assert-SingleLisanUninstallRegistryEntry -ExpectedVersion $ExpectedEarlierVersion -Context 'Earlier install'
Assert-InstalledPayload

$replacementContext = if ($SameVersionReplace) { 'same-version replacement' } else { 'upgrade replacement' }
Invoke-Msi -Operation 'Install replacement MSI' -AllowedExitCodes @(0, 3010) -Arguments @(
    "/i", "`"$ReplacementMsiPath`"",
    "/qn",
    "/norestart",
    "/L*v", "`"$replacementInstallLog`""
) | Out-Null
$replacementRegistryEntry = Assert-SingleLisanUninstallRegistryEntry -ExpectedVersion $ExpectedReplacementVersion -Context $replacementContext
Assert-InstalledPayload

$downgradeExitCode = $null
if (-not [string]::IsNullOrWhiteSpace($DowngradeMsiPath)) {
    $downgradeExitCode = Invoke-MsiExpectFailure -Operation 'Attempt downgrade MSI' -UnexpectedSuccessExitCodes @(0, 3010) -Arguments @(
        "/i", "`"$DowngradeMsiPath`"",
        "/qn",
        "/norestart",
        "/L*v", "`"$downgradeAttemptLog`""
    )
    $replacementRegistryEntry = Assert-SingleLisanUninstallRegistryEntry -ExpectedVersion $ExpectedReplacementVersion -Context 'Post-downgrade-refusal replacement'
    Assert-InstalledPayload
}

& (Join-Path $PSScriptRoot "installed-smoke.ps1") -InstallRoot $InstallRoot

if (-not $KeepInstalled) {
    Get-Process LisanStudio,ArabicCodeStudioQt -ErrorAction SilentlyContinue | Stop-Process -Force
    Invoke-Msi -Operation "Uninstall replacement MSI" -AllowedExitCodes @(0, 3010) -Arguments @(
        "/x", "`"$ReplacementMsiPath`"",
        "/qn",
        "/norestart",
        "/L*v", "`"$uninstallLog`""
    ) | Out-Null

    $app = Join-Path $InstallRoot "LisanStudio.exe"
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
    EarlierMsi = $EarlierMsiPath
    ReplacementMsi = $ReplacementMsiPath
    DowngradeMsi = $DowngradeMsiPath
    InstallRoot = $InstallRoot
    SameVersionReplace = [bool]$SameVersionReplace
    KeptInstalled = [bool]$KeepInstalled
    EarlierInstallLog = $earlierInstallLog
    ReplacementInstallLog = $replacementInstallLog
    DowngradeAttemptLog = $downgradeAttemptLog
    DowngradeExitCode = $downgradeExitCode
    PrecleanLog = $precleanLog
    UninstallLog = $uninstallLog
    EarlierRegistryEntry = $earlierRegistryEntry
    ReplacementRegistryEntry = $replacementRegistryEntry
    UpgradeRegistryEntries = @(Get-LisanUninstallRegistryEntries)
}
