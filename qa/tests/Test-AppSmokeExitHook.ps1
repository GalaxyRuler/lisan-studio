[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$mainSource = Join-Path $repoRoot 'src\main.cpp'
$installedSmokeScript = Join-Path $repoRoot 'scripts\installed-smoke.ps1'

foreach ($requiredPath in @($mainSource, $installedSmokeScript)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Missing file: $requiredPath"
    }
}

$main = Get-Content -Raw -LiteralPath $mainSource
$installedSmoke = Get-Content -Raw -LiteralPath $installedSmokeScript

foreach ($requiredMainToken in @(
        'LISAN_STUDIO_SMOKE_EXIT_MS',
        'argv[index]',
        'qEnvironmentVariableIntValue',
        'const auto openRequestedPath',
        'QTimer::singleShot(0, &window',
        'QTimer::singleShot(smokeExitMs, &app',
        'app.quit()',
        'window.close()',
        'QCoreApplication::exit(0)',
        'std::thread',
        'std::this_thread::sleep_for',
        'std::exit(0)'
    )) {
    if ($main -notmatch [regex]::Escape($requiredMainToken)) {
        throw "main.cpp should keep the installed-app smoke exit hook token: $requiredMainToken"
    }
}

$timerIndex = $main.IndexOf('QTimer::singleShot(smokeExitMs')
$openIndex = $main.IndexOf('window.openPath(pathToOpen)')
if ($timerIndex -lt 0 -or $openIndex -lt 0 -or $timerIndex -gt $openIndex) {
    throw 'main.cpp should arm the smoke exit timer before opening a requested path.'
}

foreach ($requiredSmokeToken in @(
        '$previousSmokeExitEnv = $env:LISAN_STUDIO_SMOKE_EXIT_MS',
        '$env:LISAN_STUDIO_SMOKE_EXIT_MS = "$SmokeExitMs"',
        'Remove-Item Env:\LISAN_STUDIO_SMOKE_EXIT_MS'
    )) {
    if ($installedSmoke -notmatch [regex]::Escape($requiredSmokeToken)) {
        throw "installed-smoke.ps1 should set and restore smoke exit environment token: $requiredSmokeToken"
    }
}

"Test-AppSmokeExitHook.ps1 passed"
