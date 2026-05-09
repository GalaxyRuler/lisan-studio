param(
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA "LisanStudio"),
    [int]$SmokeExitMs = 1500
)

$ErrorActionPreference = "Stop"

$app = Join-Path $InstallRoot "LisanStudio.exe"
$python = Join-Path $InstallRoot "runtime\python\python.exe"

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

$runtimeOutput = & $python -m arabicpython.cli $sampleFile 2>&1
if ($LASTEXITCODE -ne 0) {
    $runtimeOutput | Write-Output
    throw "Bundled runtime failed with exit code $LASTEXITCODE"
}
if (($runtimeOutput -join "`n") -notmatch [regex]::Escape($expected)) {
    $runtimeOutput | Write-Output
    throw "Bundled runtime output did not contain expected Arabic text"
}

$projectProcess = Start-Process -FilePath $app -ArgumentList @($sampleRoot, "--smoke-exit-ms", "$SmokeExitMs") -PassThru -WindowStyle Hidden
if (-not $projectProcess.WaitForExit($SmokeExitMs + 10000)) {
    Stop-Process -Id $projectProcess.Id -Force
    throw "Installed app did not exit after project smoke timeout"
}
if ($projectProcess.ExitCode -ne 0) {
    throw "Installed app project smoke exited with code $($projectProcess.ExitCode)"
}

$fileProcess = Start-Process -FilePath $app -ArgumentList @($sampleFile, "--smoke-exit-ms", "$SmokeExitMs") -PassThru -WindowStyle Hidden
if (-not $fileProcess.WaitForExit($SmokeExitMs + 10000)) {
    Stop-Process -Id $fileProcess.Id -Force
    throw "Installed app did not exit after file smoke timeout"
}
if ($fileProcess.ExitCode -ne 0) {
    throw "Installed app file smoke exited with code $($fileProcess.ExitCode)"
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
    RuntimeOutput = ($runtimeOutput -join "`n")
    EditableMarkers = 0
}
