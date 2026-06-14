[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$installedSmokeScript = Join-Path $repoRoot 'scripts\installed-smoke.ps1'

if (-not (Test-Path -LiteralPath $installedSmokeScript)) {
    throw "Missing installed smoke script: $installedSmokeScript"
}

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($installedSmokeScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "installed-smoke.ps1 parse failed: $($parseErrors[0].Message)"
}

$source = Get-Content -Raw -LiteralPath $installedSmokeScript

foreach ($requiredToken in @(
        'runtime.stdout.log',
        'runtime.stderr.log',
        'app-smoke-diagnostics.json',
        'Get-CimInstance -ClassName Win32_Process',
        'Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue',
        '-RedirectStandardOutput $runtimeStdoutPath',
        '-RedirectStandardError $runtimeStderrPath',
        'Get-Content -Raw -LiteralPath $runtimeStdoutPath -Encoding UTF8',
        '$runtimeProcess.ExitCode'
    )) {
    if ($source -notmatch [regex]::Escape($requiredToken)) {
        throw "installed-smoke.ps1 should preserve UTF-8 runtime output token: $requiredToken"
    }
}

$projectLaunchPattern = [regex]::Escape('$projectProcess = Start-Process -FilePath $app') + '(?s).*?' + [regex]::Escape('$fileProcess = Start-Process -FilePath $app')
$projectLaunchBlock = [regex]::Match($source, $projectLaunchPattern).Value
if ($projectLaunchBlock -match '-WindowStyle\s+Hidden') {
    throw 'installed GUI smoke should run the app headed inside an isolated Windows QA environment, not hidden.'
}

if ($source -match '&\s+\$python\s+-m\s+arabicpython\.cli\s+\$sampleFile\s+2>&1') {
    throw 'installed-smoke.ps1 should not capture Arabic runtime output through PowerShell native-command decoding.'
}

"Test-InstalledSmokeUtf8Capture.ps1 passed"
