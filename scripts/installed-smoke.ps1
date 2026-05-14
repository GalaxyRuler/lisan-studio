param(
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA "LisanStudio"),
    [int]$SmokeExitMs = 1500
)

$ErrorActionPreference = "Stop"

$app = Join-Path $InstallRoot "LisanStudio.exe"
$python = Join-Path $InstallRoot "runtime\python\python.exe"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$smokeArtifactDir = Join-Path $repo "artifacts\msi-smoke"
New-Item -ItemType Directory -Force -Path $smokeArtifactDir | Out-Null
$appSmokeDiagnosticsPath = Join-Path $smokeArtifactDir "app-smoke-diagnostics.json"

if (-not (Test-Path $app)) {
    throw "Installed app not found: $app"
}
if (-not (Test-Path $python)) {
    throw "Installed Python runtime not found: $python"
}

$sampleRoot = Join-Path $env:TEMP "LisanStudioSmokeProject"
if (Test-Path $sampleRoot) {
    Remove-Item -LiteralPath $sampleRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path (Join-Path $sampleRoot "src") | Out-Null

$sampleFile = Join-Path $sampleRoot "src\main.apy"
$print = -join @([char]0x0627, [char]0x0637, [char]0x0628, [char]0x0639)
$expected = -join @(
    [char]0x062A, [char]0x062D, [char]0x0642, [char]0x0642, " ",
    [char]0x0645, [char]0x0646, " ",
    [char]0x0627, [char]0x0644, [char]0x0646, [char]0x0633, [char]0x062E, [char]0x0629, " ",
    [char]0x0627, [char]0x0644, [char]0x0645, [char]0x062B, [char]0x0628, [char]0x062A, [char]0x0629
)
$lines = @(
    "$print(`"$expected`")",
    "$print(`"Lisan Studio`")",
    "$print(`"C:/Users/Admin/project/main.apy`")"
)
$lines | Set-Content -LiteralPath $sampleFile -Encoding UTF8

$env:PYTHONNOUSERSITE = "1"
$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"

$runtimeStdoutPath = Join-Path $sampleRoot "runtime.stdout.log"
$runtimeStderrPath = Join-Path $sampleRoot "runtime.stderr.log"
$runtimeProcess = Start-Process -FilePath $python `
    -ArgumentList @("-m", "arabicpython.cli", $sampleFile) `
    -RedirectStandardOutput $runtimeStdoutPath `
    -RedirectStandardError $runtimeStderrPath `
    -Wait `
    -PassThru `
    -WindowStyle Hidden

$runtimeOutput = if (Test-Path -LiteralPath $runtimeStdoutPath) {
    Get-Content -Raw -LiteralPath $runtimeStdoutPath -Encoding UTF8
} else {
    ""
}
$runtimeError = if (Test-Path -LiteralPath $runtimeStderrPath) {
    Get-Content -Raw -LiteralPath $runtimeStderrPath -Encoding UTF8
} else {
    ""
}

if ($runtimeProcess.ExitCode -ne 0) {
    if ($runtimeOutput) { $runtimeOutput | Write-Output }
    if ($runtimeError) { $runtimeError | Write-Output }
    throw "Bundled runtime failed with exit code $($runtimeProcess.ExitCode)"
}
if ($runtimeOutput -notmatch [regex]::Escape($expected)) {
    if ($runtimeOutput) { $runtimeOutput | Write-Output }
    if ($runtimeError) { $runtimeError | Write-Output }
    throw "Bundled runtime output did not contain expected Arabic text"
}

function Get-AppSmokeProcessDiagnostic {
    param(
        [Parameter(Mandatory = $true)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)][string]$Phase,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $Process.Refresh()
    $processInfo = Get-CimInstance -ClassName Win32_Process -Filter "ProcessId = $($Process.Id)" -ErrorAction SilentlyContinue
    [ordered]@{
        phase = $Phase
        capturedAt = (Get-Date).ToString('o')
        processId = $Process.Id
        hasExited = $Process.HasExited
        exitCode = if ($Process.HasExited) { $Process.ExitCode } else { $null }
        mainWindowTitle = if ($Process.HasExited) { $null } else { $Process.MainWindowTitle }
        responding = if ($Process.HasExited) { $null } else { $Process.Responding }
        startInfoArguments = $Arguments
        executablePath = if ($processInfo) { $processInfo.ExecutablePath } else { $null }
        commandLine = if ($processInfo) { $processInfo.CommandLine } else { $null }
        creationDate = if ($processInfo) { $processInfo.CreationDate } else { $null }
    }
}

function Wait-AppSmokeProcess {
    param(
        [Parameter(Mandatory = $true)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)][string]$Phase,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$DiagnosticsPath,
        [Parameter(Mandatory = $true)][int]$TimeoutMs
    )

    if (-not $Process.WaitForExit($TimeoutMs)) {
        $diagnostic = Get-AppSmokeProcessDiagnostic -Process $Process -Phase $Phase -Arguments $Arguments
        $diagnostic | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $DiagnosticsPath -Encoding UTF8
        Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
        throw "Installed app did not exit after $Phase smoke timeout. Diagnostic: $DiagnosticsPath"
    }
    if ($Process.ExitCode -ne 0) {
        throw "Installed app $Phase smoke exited with code $($Process.ExitCode)"
    }
}

$previousSmokeExitEnv = $env:LISAN_STUDIO_SMOKE_EXIT_MS
try {
    $env:LISAN_STUDIO_SMOKE_EXIT_MS = "$SmokeExitMs"

    $projectArguments = @($sampleRoot, "--smoke-exit-ms", "$SmokeExitMs")
    $projectProcess = Start-Process -FilePath $app -ArgumentList $projectArguments -PassThru
    Wait-AppSmokeProcess -Process $projectProcess -Phase "project" -Arguments $projectArguments -DiagnosticsPath $appSmokeDiagnosticsPath -TimeoutMs ($SmokeExitMs + 10000)

    $fileArguments = @($sampleFile, "--smoke-exit-ms", "$SmokeExitMs")
    $fileProcess = Start-Process -FilePath $app -ArgumentList $fileArguments -PassThru
    Wait-AppSmokeProcess -Process $fileProcess -Phase "file" -Arguments $fileArguments -DiagnosticsPath $appSmokeDiagnosticsPath -TimeoutMs ($SmokeExitMs + 10000)
} finally {
    if ($null -eq $previousSmokeExitEnv) {
        Remove-Item Env:\LISAN_STUDIO_SMOKE_EXIT_MS -ErrorAction SilentlyContinue
    } else {
        $env:LISAN_STUDIO_SMOKE_EXIT_MS = $previousSmokeExitEnv
    }
}

$sitePackages = Join-Path $InstallRoot "runtime\python\Lib\site-packages"
$editableMarkers = Get-ChildItem -LiteralPath $sitePackages -Force -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -like "__editable__*" -or $_.Name -like "apython-*.dist-info" }
if ($editableMarkers) {
    throw "Installed runtime contains editable/local apython markers"
}

[PSCustomObject]@{
    App = $app
    Runtime = $python
    SampleProject = $sampleRoot
    RuntimeOutput = $runtimeOutput
    EditableMarkers = 0
}
