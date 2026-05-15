[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$releaseEvidenceScript = Join-Path $repoRoot 'scripts\release-evidence.ps1'

if (-not (Test-Path -LiteralPath $releaseEvidenceScript)) {
    throw "Missing release evidence script: $releaseEvidenceScript"
}

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($releaseEvidenceScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "release-evidence.ps1 parse failed: $($parseErrors[0].Message)"
}

$source = Get-Content -Raw -LiteralPath $releaseEvidenceScript

if ($source -match '\$output\s*=\s*&\s*\$Command\s+2>&1') {
    throw 'Invoke-LoggedStep should stream captured command output so failures keep partial logs.'
}

if ($source -notmatch 'System\.Collections\.Generic\.List\[string\]') {
    throw 'Invoke-LoggedStep should accumulate output lines while the command runs.'
}

if ($source -notmatch '\[void\]\$outputLines\.Add') {
    throw 'Invoke-LoggedStep should add each emitted output line before failure handling.'
}

if ($source -notmatch '(?s)catch\s*\{.*\$outputLines\.Count\s*-gt\s*0.*Set-Content') {
    throw 'Invoke-LoggedStep should write captured command output when a step fails.'
}

if ($source -notmatch '(?s)catch\s*\{.*\$_.Exception.Message.*Add-Content') {
    throw 'Invoke-LoggedStep should append the failure message after captured command output.'
}

foreach ($requiredScreenshotToken in @(
        '$screenshotSmokeExitMs = 30000',
        '$screenshotArguments = @($sampleProject, "--smoke-exit-ms", "$screenshotSmokeExitMs")',
        'Start-Process -FilePath $app -ArgumentList $screenshotArguments -PassThru',
        'PrintWindow',
        'CopyFromScreen',
        'ReleaseHdc',
        'throw "Screenshot capture failed for Lisan Studio window."',
        'finally',
        'Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue'
    )) {
    if ($source -notmatch [regex]::Escape($requiredScreenshotToken)) {
        throw "release-evidence.ps1 should keep screenshot launch bounded and cleaned up: $requiredScreenshotToken"
    }
}

"Test-ReleaseEvidenceFailureLogging.ps1 passed"
