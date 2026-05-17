[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$upgradeSmokeScript = Join-Path $repoRoot 'scripts\msi-upgrade-smoke.ps1'
$upgradeScenarioScript = Join-Path $repoRoot 'qa\vm\Test-MsiUpgradeReplacesEarlierVersion.ps1'
$sameVersionScenarioScript = Join-Path $repoRoot 'qa\vm\Test-MsiSameVersionReplaceLeavesRegistryClean.ps1'
$profilePath = Join-Path $repoRoot 'qa\vm\profiles\lisanstudio-msi-upgrade.json'
$runnerConfigPath = Join-Path $repoRoot '.codex\homelab-runner.json'

foreach ($requiredPath in @(
        $upgradeSmokeScript,
        $upgradeScenarioScript,
        $sameVersionScenarioScript,
        $profilePath,
        $runnerConfigPath
    )) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Missing MSI upgrade evidence file: $requiredPath"
    }
}

foreach ($scriptPath in @($upgradeSmokeScript, $upgradeScenarioScript, $sameVersionScenarioScript)) {
    $tokens = $null
    $parseErrors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$parseErrors)
    if ($parseErrors -and $parseErrors.Count -gt 0) {
        throw "$([System.IO.Path]::GetFileName($scriptPath)) parse failed: $($parseErrors[0].Message)"
    }
}

$upgradeSource = Get-Content -Raw -LiteralPath $upgradeSmokeScript
foreach ($requiredToken in @(
        'param(',
        '$EarlierMsiPath',
        '$ReplacementMsiPath',
        '$DowngradeMsiPath',
        '$SameVersionReplace',
        '$AllowMutation',
        '$IUnderstandThisRunsMsiUpgrade',
        'WHITEDRAGON',
        'function Assert-SingleLisanUninstallRegistryEntry',
        'DisplayVersion',
        'BuildId',
        'Invoke-Msi -Operation ''Install earlier MSI''',
        'Invoke-Msi -Operation ''Install replacement MSI''',
        'same-version replacement',
        'Invoke-MsiExpectFailure',
        'Downgrade unexpectedly succeeded',
        'Uninstall left Windows Apps uninstall entry behind',
        'UpgradeRegistryEntries'
    )) {
    if ($upgradeSource -notmatch [regex]::Escape($requiredToken)) {
        throw "msi-upgrade-smoke.ps1 should include token: $requiredToken"
    }
}

$upgradeScenarioSource = Get-Content -Raw -LiteralPath $upgradeScenarioScript
foreach ($requiredToken in @(
        '$EarlierMsiPath',
        '$ReplacementMsiPath',
        '$ExpectedEarlierVersion',
        '$ExpectedReplacementVersion',
        '-AllowMutation',
        '-IUnderstandThisRunsMsiUpgrade'
    )) {
    if ($upgradeScenarioSource -notmatch [regex]::Escape($requiredToken)) {
        throw "Test-MsiUpgradeReplacesEarlierVersion.ps1 should include token: $requiredToken"
    }
}

$sameVersionSource = Get-Content -Raw -LiteralPath $sameVersionScenarioScript
foreach ($requiredToken in @(
        '$FirstMsiPath',
        '$ReplacementMsiPath',
        '$ExpectedProductVersion',
        '-SameVersionReplace',
        '-AllowMutation',
        '-IUnderstandThisRunsMsiUpgrade'
    )) {
    if ($sameVersionSource -notmatch [regex]::Escape($requiredToken)) {
        throw "Test-MsiSameVersionReplaceLeavesRegistryClean.ps1 should include token: $requiredToken"
    }
}

$profile = Get-Content -Raw -LiteralPath $profilePath | ConvertFrom-Json
if ($profile.class -ne 'vm-gui') {
    throw 'MSI upgrade profile must run through the VM GUI class.'
}
if (-not [bool]$profile.approvalRequired) {
    throw 'MSI upgrade profile must require approval.'
}
if ([bool]$profile.activeWhitedragonAllowed) {
    throw 'MSI upgrade profile must not allow active WHITEDRAGON.'
}
foreach ($requiredProfileToken in @(
        'same-version reinstall leaves exactly one uninstall entry',
        'downgrade refusal keeps the replacement install active'
    )) {
    $profileText = Get-Content -Raw -LiteralPath $profilePath
    if ($profileText -notmatch [regex]::Escape($requiredProfileToken)) {
        throw "MSI upgrade profile should include token: $requiredProfileToken"
    }
}
if ($profile.projectScripts.upgradeScenario -ne 'qa\vm\Test-MsiUpgradeReplacesEarlierVersion.ps1') {
    throw 'MSI upgrade profile should point at the upgrade scenario script.'
}
if ($profile.projectScripts.sameVersionScenario -ne 'qa\vm\Test-MsiSameVersionReplaceLeavesRegistryClean.ps1') {
    throw 'MSI upgrade profile should point at the same-version scenario script.'
}

$runnerConfig = Get-Content -Raw -LiteralPath $runnerConfigPath | ConvertFrom-Json
if (-not $runnerConfig.profiles.desktopMsiUpgrade) {
    throw 'Homelab runner config should expose a desktopMsiUpgrade profile.'
}
if ($runnerConfig.profiles.desktopMsiUpgrade.profilePath -ne 'qa\vm\profiles\lisanstudio-msi-upgrade.json') {
    throw 'desktopMsiUpgrade should point at the project-owned MSI upgrade profile.'
}
if (-not [bool]$runnerConfig.profiles.desktopMsiUpgrade.approvalRequired) {
    throw 'desktopMsiUpgrade must require approval.'
}

"Test-MsiUpgradeEvidencePolicy.ps1 passed"
