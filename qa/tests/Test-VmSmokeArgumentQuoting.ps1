[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$smokeScript = Join-Path $repoRoot 'qa\vm\Invoke-LisanStudioVmSmoke.ps1'

if (-not (Test-Path -LiteralPath $smokeScript)) {
    throw "Missing VM smoke script: $smokeScript"
}

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($smokeScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors -and $parseErrors.Count -gt 0) {
    throw "Invoke-LisanStudioVmSmoke.ps1 parse failed: $($parseErrors[0].Message)"
}

$source = Get-Content -Raw -LiteralPath $smokeScript

if ($source -notmatch 'function\s+Quote-ProcessArgument') {
    throw 'guest interactive runner should quote process arguments before Start-Process.'
}

if ($source -match 'Start-Process\s+-FilePath\s+''powershell\.exe''\s+-ArgumentList\s+\(@\(') {
    throw 'Start-Process should not receive a raw argument array because paths with spaces get split.'
}

if ($source -notmatch '-ArgumentList\s+\$argumentLine') {
    throw 'Start-Process should receive the quoted argument line.'
}

if ($source -notmatch 'ForEach-Object\s+\{\s+Quote-ProcessArgument') {
    throw 'argument line should be built by quoting every argument.'
}

if ($source -match 'Copy-Item\s+-ToSession\s+\$session\s+-LiteralPath\s+\$hostApythonRoot\s+-Destination\s+\(Split-Path\s+-Parent\s+\$guestApythonRoot\)') {
    throw 'Apython staging should copy the source contents into the guest root, not the host root directory container into the parent.'
}

if ($source -match [regex]::Escape('LisanStudio-0.1.0-beta.msi')) {
    throw 'VM smoke should derive the packaged MSI path from productVersion instead of hardcoding 0.1.0-beta.'
}

if ($source -match [regex]::Escape('artifacts\release\0.1.0-beta')) {
    throw 'VM smoke should derive planned release artifact paths from releaseLabel instead of hardcoding 0.1.0-beta.'
}

foreach ($requiredToken in @(
        'host-progress.jsonl',
        'function Write-SmokeProgress',
        'function Get-HostGitMetadata',
        'source-metadata.json',
        'Writing source metadata into guest workspace.',
        'scheduled-task-poll',
        'Copying project item into guest workspace.',
        'Copying guest path back to host.',
        'timeout-process-snapshot.json',
        'Stop-ScheduledTask -TaskName $TaskName',
        '$timeout = [TimeSpan]::FromMinutes(75)',
        'New-TimeSpan -Minutes 90',
        'Copy-GuestSmokeArtifacts',
        '$guestLaneLeaf = Split-Path -Leaf (Split-Path -Parent $resolvedOutputDirectory)',
        '$guestWorkLeaf = "$guestLaneLeaf-$guestArtifactLeaf"',
        '$guestWorkRoot = Join-Path ''C:\CodexRunner\work'' $guestWorkLeaf',
        '$guestRepoRoot = Join-Path $guestWorkRoot ''arabic-code-studio-qt''',
        '$guestApythonRoot = Join-Path $guestWorkRoot ''apython''',
        '$productVersion = ''0.1.0''',
        '$releaseLabel = "$productVersion-beta"',
        '$packagedMsi = Join-Path $GuestRepoRoot "artifacts\LisanStudio-$productVersion-beta.msi"',
        '$releaseDir = Join-Path $GuestRepoRoot "artifacts\release\$releaseLabel"',
        '"artifacts\release\$releaseLabel\VALIDATION_LOG.md"',
        '"artifacts\release\$releaseLabel\CHECKSUMS-SHA256.txt"',
        '"artifacts\release\$releaseLabel\KNOWN_ISSUES.md"',
        '"artifacts\release\$releaseLabel\screenshots\main-window.png"',
        '$productVersion,',
        '$releaseLabel,',
        'guestWorkLeaf = $guestWorkLeaf',
        'guestWorkRoot = $guestWorkRoot',
        'New-Item -ItemType Directory -Force -Path $GuestApythonRoot',
        'Remove-Item -LiteralPath $GuestArtifactDirectory -Recurse -Force',
        'Remove-Item -LiteralPath $GuestLogDirectory -Recurse -Force',
        '$hostApythonContents = Join-Path $hostApythonRoot ''*''',
        'Copy-Item -ToSession $session -Path $hostApythonContents -Destination $guestApythonRoot',
        '$requiredApythonFiles',
        'arabicpython\cli.py',
        'arabicpython_kernel\__init__.py',
        'Guest Apython payload file missing after staging'
    )) {
    if ($source -notmatch [regex]::Escape($requiredToken)) {
        throw "VM smoke should verify Apython payload staging token: $requiredToken"
    }
}

"Test-VmSmokeArgumentQuoting.ps1 passed"
