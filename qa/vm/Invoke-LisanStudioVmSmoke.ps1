[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [string]$ApprovalPacketPath,

    [string]$HomelabRoot,

    [string]$OutputDirectory,

    [switch]$DryRun,

    [switch]$NoDryRun,

    [switch]$AllowMutation,

    [switch]$IUnderstandThisRunsSmoke,

    [switch]$Json
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Write-FailureAndExit {
    param(
        [string]$Message,
        [switch]$AsJson,
        [bool]$EffectiveDryRun = $true,
        [string]$ResultPath
    )

    $result = [ordered]@{
        ok = $false
        message = $Message
        dryRun = $EffectiveDryRun
        mutationPerformed = $false
        crossedApprovalGate = $false
    }

    if ($ResultPath) {
        $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ResultPath -Encoding UTF8
    }
    if ($AsJson) {
        $result | ConvertTo-Json -Depth 8
    } else {
        Write-Error $Message
    }
    exit 1
}

function Resolve-HomelabRoot {
    param([string]$ExplicitRoot)

    if ($ExplicitRoot) {
        return (Resolve-Path -LiteralPath $ExplicitRoot).Path
    }
    if ($env:CODEX_HOMELAB_ROOT) {
        return (Resolve-Path -LiteralPath $env:CODEX_HOMELAB_ROOT).Path
    }

    $fallback = 'C:\Users\Admin\Documents\Codex\Homelab\codex-isolated-test-runners'
    return (Resolve-Path -LiteralPath $fallback).Path
}

function Test-ActiveWhitedragonTarget {
    param([object]$Packet)

    foreach ($field in @('targetHost', 'targetComputer', 'targetMachine', 'targetRunner', 'targetDesktop', 'runnerName', 'computerName', 'route')) {
        if ($Packet.PSObject.Properties.Name -contains $field) {
            $value = [string]$Packet.$field
            if ($value -match '^(active[-_\s]*)?WHITEDRAGON$') {
                return $true
            }
        }
    }
    if ($Packet.PSObject.Properties.Name -contains 'activeWhitedragonAllowed' -and [bool]$Packet.activeWhitedragonAllowed) {
        return $true
    }
    return $false
}

function Get-IsAdministrator {
    $principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
    return $principal.IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)
}

function New-DefaultOutputDirectory {
    param([string]$ResolvedHomelabRoot)

    $stamp = Get-Date -Format 'yyyyMMddTHHmmss'
    return Join-Path $ResolvedHomelabRoot "tools\codex-runner\artifacts\project-msi-gui-smoke-$stamp"
}

function Get-LisanQaCredential {
    $credentialPath = 'C:\CodexRunner\work\LisanStudio-QA\guest-credential.xml'
    if (-not (Test-Path -LiteralPath $credentialPath)) {
        throw "Guest credential file not found: $credentialPath"
    }

    $storedCredential = Import-Clixml -LiteralPath $credentialPath
    $userName = [string]$storedCredential.UserName
    if ($userName -notmatch '\\') {
        $userName = "LISAN-QA\$userName"
    }

    return [ordered]@{
        credential = [pscredential]::new($userName, $storedCredential.Password)
        path = $credentialPath
        userName = $userName
    }
}

function Get-HostApythonRoot {
    $candidate = 'C:\Users\Admin\apython'
    if (-not (Test-Path -LiteralPath $candidate)) {
        throw "Apython root not found on the host: $candidate"
    }
    return (Resolve-Path -LiteralPath $candidate).Path
}

function Get-GuestInteractiveSmokeRunnerContent {
    return @'
param(
    [Parameter(Mandatory = $true)]
    [string]$GuestRepoRoot,

    [Parameter(Mandatory = $true)]
    [string]$GuestApythonRoot,

    [Parameter(Mandatory = $true)]
    [string]$GuestArtifactDirectory,

    [Parameter(Mandatory = $true)]
    [string]$GuestLogDirectory,

    [Parameter(Mandatory = $true)]
    [string]$ResultPath,

    [string]$ScheduledTaskName = 'unknown'
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Resolve-FirstExistingPath {
    param(
        [string[]]$Candidates,
        [string]$CommandName
    )

    foreach ($candidate in @($Candidates)) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path -LiteralPath $candidate)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    if ($CommandName) {
        $command = Get-Command -Name $CommandName -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($command) {
            return $command.Source
        }
    }

    return $null
}

function Resolve-PythonRoot {
    function Test-PythonHomeCandidate {
        param([string]$CandidatePath)

        if ([string]::IsNullOrWhiteSpace($CandidatePath)) {
            return $false
        }
        return (Test-Path -LiteralPath (Join-Path $CandidatePath 'python.exe')) -and
            (Test-Path -LiteralPath (Join-Path $CandidatePath 'LICENSE.txt'))
    }

    $candidates = @(
        (Join-Path $env:LOCALAPPDATA 'Programs\Python\Python313'),
        (Join-Path $env:ProgramFiles 'Python313')
    )
    foreach ($candidate in $candidates) {
        if (Test-PythonHomeCandidate -CandidatePath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    $pythonCommand = Get-Command -Name python.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($pythonCommand) {
        try {
            $reportedRoot = & $pythonCommand.Source -c 'import pathlib, sys; print(pathlib.Path(sys.base_prefix).resolve())' 2>$null
            if ($LASTEXITCODE -eq 0) {
                $resolvedRoot = (@($reportedRoot) | Select-Object -First 1)
                if (-not [string]::IsNullOrWhiteSpace([string]$resolvedRoot) -and
                    (Test-PythonHomeCandidate -CandidatePath $resolvedRoot.Trim())) {
                    return (Resolve-Path -LiteralPath $resolvedRoot.Trim()).Path
                }
            }
        } catch {
            # Fall through to null when the python launcher alias does not provide a real install root.
        }
    }

    return $null
}

function Quote-ProcessArgument {
    param([string]$Value)

    if ($null -eq $Value -or $Value.Length -eq 0) {
        return '""'
    }
    return '"' + $Value.Replace('"', '\"') + '"'
}

function Invoke-PowerShellScriptStep {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [string]$ScriptPath,

        [string[]]$Arguments = @()
    )

    $stdoutPath = Join-Path $GuestLogDirectory "$Name.stdout.log"
    $stderrPath = Join-Path $GuestLogDirectory "$Name.stderr.log"
    $combinedPath = Join-Path $GuestLogDirectory "$Name.log"
    $metaPath = Join-Path $GuestArtifactDirectory "$Name.result.json"
    $started = Get-Date
    $exitCode = $null

    try {
        $processArguments = @(
                '-NoProfile',
                '-ExecutionPolicy',
                'Bypass',
                '-File',
                $ScriptPath
            ) + @($Arguments)
        $argumentLine = ($processArguments | ForEach-Object { Quote-ProcessArgument -Value ([string]$_) }) -join ' '
        $process = Start-Process -FilePath 'powershell.exe' -ArgumentList $argumentLine `
            -WorkingDirectory $GuestRepoRoot `
            -Wait `
            -PassThru `
            -NoNewWindow `
            -RedirectStandardOutput $stdoutPath `
            -RedirectStandardError $stderrPath
        $exitCode = $process.ExitCode

        $stdoutLines = @(Get-Content -LiteralPath $stdoutPath -ErrorAction SilentlyContinue)
        $stderrLines = @(Get-Content -LiteralPath $stderrPath -ErrorAction SilentlyContinue)
        $combinedLines = @($stdoutLines)
        if ($stderrLines.Count -gt 0) {
            if ($combinedLines.Count -gt 0) {
                $combinedLines += ''
            }
            $combinedLines += '--- STDERR ---'
            $combinedLines += $stderrLines
        }
        $combinedLines | Set-Content -LiteralPath $combinedPath -Encoding UTF8

        $step = [ordered]@{
            name = $Name
            ok = ($exitCode -eq 0)
            started = $started.ToString('o')
            finished = (Get-Date).ToString('o')
            command = @('powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $ScriptPath) + @($Arguments)
            exitCode = $exitCode
            logPath = $combinedPath
            stdoutPath = $stdoutPath
            stderrPath = $stderrPath
            resultPath = $metaPath
        }
        if (-not $step.ok) {
            $step.errorMessage = "$([System.IO.Path]::GetFileName($ScriptPath)) exited with code $exitCode"
        }
        $step | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $metaPath -Encoding UTF8
        return $step
    } catch {
        @(
            $_.Exception.Message,
            '',
            $_.ScriptStackTrace
        ) | Set-Content -LiteralPath $combinedPath -Encoding UTF8

        $step = [ordered]@{
            name = $Name
            ok = $false
            started = $started.ToString('o')
            finished = (Get-Date).ToString('o')
            command = @('powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $ScriptPath) + @($Arguments)
            exitCode = $exitCode
            logPath = $combinedPath
            stdoutPath = $stdoutPath
            stderrPath = $stderrPath
            resultPath = $metaPath
            errorMessage = $_.Exception.Message
        }
        $step | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $metaPath -Encoding UTF8
        return $step
    }
}

$smokeSummaryPath = Join-Path $GuestArtifactDirectory 'smoke-summary.json'
$smokeSummaryMarkdownPath = Join-Path $GuestArtifactDirectory 'smoke-summary.md'
$releaseDir = Join-Path $GuestRepoRoot 'artifacts\release\0.1.0-beta'
$msiSmokeDir = Join-Path $GuestRepoRoot 'artifacts\msi-smoke'
$packagedMsi = Join-Path $GuestRepoRoot 'artifacts\LisanStudio-0.1.0-beta.msi'

$tooling = [ordered]@{}
$missingRequirements = New-Object System.Collections.ArrayList
$stepResults = New-Object System.Collections.ArrayList
$errorMessage = $null
$ok = $true
$interactiveSessionId = $null
$interactiveUserName = $null

try {
    New-Item -ItemType Directory -Force -Path $GuestArtifactDirectory, $GuestLogDirectory | Out-Null
    $interactiveSessionId = (Get-Process -Id $PID).SessionId
    $interactiveUserName = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name

    $tooling = [ordered]@{
        pwshPath = Resolve-FirstExistingPath -Candidates @(
            (Join-Path $env:ProgramFiles 'PowerShell\7\pwsh.exe'),
            (Join-Path $env:LOCALAPPDATA 'Microsoft\WindowsApps\pwsh.exe')
        ) -CommandName 'pwsh.exe'
        gitPath = Resolve-FirstExistingPath -Candidates @(
            (Join-Path $env:ProgramFiles 'Git\cmd\git.exe'),
            (Join-Path $env:ProgramFiles 'Git\bin\git.exe'),
            (Join-Path $env:LOCALAPPDATA 'Programs\Git\cmd\git.exe'),
            (Join-Path $env:LOCALAPPDATA 'Programs\Git\bin\git.exe')
        ) -CommandName 'git.exe'
        pythonRoot = Resolve-PythonRoot
        bashPath = Resolve-FirstExistingPath -Candidates @('C:\msys64\usr\bin\bash.exe') -CommandName $null
        cmakePath = Resolve-FirstExistingPath -Candidates @('C:\msys64\ucrt64\bin\cmake.exe') -CommandName $null
        ninjaPath = Resolve-FirstExistingPath -Candidates @('C:\msys64\ucrt64\bin\ninja.exe') -CommandName $null
        windeployQtPath = Resolve-FirstExistingPath -Candidates @('C:\msys64\ucrt64\bin\windeployqt6.exe') -CommandName $null
        wixPath = Resolve-FirstExistingPath -Candidates @('C:\Program Files\WiX Toolset v7.0\bin\wix.exe') -CommandName 'wix.exe'
        qtLicenseRoot = if (Test-Path -LiteralPath 'C:\msys64\ucrt64\share\licenses\qt6-base') { (Resolve-Path -LiteralPath 'C:\msys64\ucrt64\share\licenses\qt6-base').Path } else { $null }
    }

    foreach ($requirement in @(
            @{ name = 'Git'; value = $tooling.gitPath },
            @{ name = 'PythonRoot'; value = $tooling.pythonRoot },
            @{ name = 'MSYS2 bash'; value = $tooling.bashPath },
            @{ name = 'CMake'; value = $tooling.cmakePath },
            @{ name = 'Ninja'; value = $tooling.ninjaPath },
            @{ name = 'windeployqt6'; value = $tooling.windeployQtPath },
            @{ name = 'WiX'; value = $tooling.wixPath },
            @{ name = 'Qt license root'; value = $tooling.qtLicenseRoot }
        )) {
        if ([string]::IsNullOrWhiteSpace([string]$requirement.value)) {
            [void]$missingRequirements.Add($requirement.name)
        }
    }
    if (-not (Test-Path -LiteralPath (Join-Path $GuestApythonRoot 'LICENSE'))) {
        [void]$missingRequirements.Add('Apython LICENSE')
    }

    if (@($missingRequirements).Count -gt 0) {
        $ok = $false
        $errorMessage = "Guest tooling is still incomplete: $(@($missingRequirements) -join ', ')"
    } else {
        $gitDirectory = Split-Path -Parent $tooling.gitPath
        if ($gitDirectory -and -not ($env:PATH -split ';' | Where-Object { $_ -eq $gitDirectory })) {
            $env:PATH = "$gitDirectory;$env:PATH"
        }

        $validateStep = Invoke-PowerShellScriptStep -Name 'validate' -ScriptPath (Join-Path $GuestRepoRoot 'scripts\validate.ps1') -Arguments @(
            '-BashPath',
            $tooling.bashPath
        )
        [void]$stepResults.Add($validateStep)
        if (-not $validateStep.ok) {
            $ok = $false
            $errorMessage = $validateStep.errorMessage
        }

        if ($ok) {
            $releaseEvidenceStep = Invoke-PowerShellScriptStep -Name 'release-evidence' -ScriptPath (Join-Path $GuestRepoRoot 'scripts\release-evidence.ps1') -Arguments @(
                '-ProductVersion',
                '0.1.0',
                '-ReleaseLabel',
                '0.1.0-beta',
                '-ApythonRoot',
                $GuestApythonRoot,
                '-PythonRoot',
                $tooling.pythonRoot,
                '-BashPath',
                $tooling.bashPath,
                '-WindeployQtPath',
                $tooling.windeployQtPath,
                '-WixPath',
                $tooling.wixPath,
                '-QtLicenseRoot',
                $tooling.qtLicenseRoot
            )
            [void]$stepResults.Add($releaseEvidenceStep)
            if (-not $releaseEvidenceStep.ok) {
                $ok = $false
                $errorMessage = $releaseEvidenceStep.errorMessage
            }
        }
    }
} catch {
    $ok = $false
    $errorMessage = $_.Exception.Message
}

$artifacts = [ordered]@{
    packagedMsi = if (Test-Path -LiteralPath $packagedMsi) { $packagedMsi } else { $null }
    releaseDir = if (Test-Path -LiteralPath $releaseDir) { $releaseDir } else { $null }
    msiSmokeDir = if (Test-Path -LiteralPath $msiSmokeDir) { $msiSmokeDir } else { $null }
    validationLog = if (Test-Path -LiteralPath (Join-Path $releaseDir 'VALIDATION_LOG.md')) { Join-Path $releaseDir 'VALIDATION_LOG.md' } else { $null }
    checksums = if (Test-Path -LiteralPath (Join-Path $releaseDir 'CHECKSUMS-SHA256.txt')) { Join-Path $releaseDir 'CHECKSUMS-SHA256.txt' } else { $null }
    knownIssues = if (Test-Path -LiteralPath (Join-Path $releaseDir 'KNOWN_ISSUES.md')) { Join-Path $releaseDir 'KNOWN_ISSUES.md' } else { $null }
    screenshot = if (Test-Path -LiteralPath (Join-Path $releaseDir 'screenshots\main-window.png')) { Join-Path $releaseDir 'screenshots\main-window.png' } else { $null }
    packageLog = if (Test-Path -LiteralPath (Join-Path $releaseDir 'logs\package.log')) { Join-Path $releaseDir 'logs\package.log' } else { $null }
    msiSmokeLog = if (Test-Path -LiteralPath (Join-Path $releaseDir 'logs\msi-smoke-keep-installed.log')) { Join-Path $releaseDir 'logs\msi-smoke-keep-installed.log' } else { $null }
}

$summary = [ordered]@{
    ok = $ok
    generatedAt = (Get-Date).ToString('o')
    guestRepoRoot = $GuestRepoRoot
    guestApythonRoot = $GuestApythonRoot
    guestArtifactDirectory = $GuestArtifactDirectory
    guestLogDirectory = $GuestLogDirectory
    scheduledTaskName = $ScheduledTaskName
    interactiveExecution = $true
    interactiveSessionId = $interactiveSessionId
    interactiveUserName = $interactiveUserName
    scriptsUsed = @(
        'scripts\validate.ps1',
        'scripts\package.ps1',
        'scripts\installed-smoke.ps1',
        'scripts\msi-smoke.ps1',
        'scripts\release-evidence.ps1'
    )
    tooling = $tooling
    missingRequirements = @($missingRequirements)
    steps = @($stepResults)
    artifacts = $artifacts
    errorMessage = $errorMessage
    notes = @(
        'MSI, install, and GUI work stayed inside LisanStudio-QA only.',
        'active WHITEDRAGON was not used.',
        'GUI-sensitive steps ran through a one-shot interactive scheduled task in the logged-on codexqa desktop session.'
    )
}
$summary | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $smokeSummaryPath -Encoding UTF8

$summaryMarkdown = @(
    '# Lisan Studio VM Smoke Summary',
    '',
    "- Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')",
    "- VM: LisanStudio-QA",
    "- Result: $(if ($ok) { 'PASS' } else { 'FAIL' })",
    "- Interactive session: $interactiveSessionId ($interactiveUserName)",
    "- Scheduled task: $ScheduledTaskName",
    '',
    '## Steps',
    ''
)
foreach ($step in @($stepResults)) {
    $summaryMarkdown += "- $($step.name): $(if ($step.ok) { 'PASS' } else { 'FAIL' }) exit=$($step.exitCode) ($($step.logPath))"
}
$summaryMarkdown += ''
$summaryMarkdown += '## Artifacts'
$summaryMarkdown += ''
foreach ($property in $artifacts.PSObject.Properties) {
    $summaryMarkdown += "- $($property.Name): $($property.Value)"
}
if ($errorMessage) {
    $summaryMarkdown += ''
    $summaryMarkdown += '## Error'
    $summaryMarkdown += ''
    $summaryMarkdown += "- $errorMessage"
}
$summaryMarkdown | Set-Content -LiteralPath $smokeSummaryMarkdownPath -Encoding UTF8

$result = [ordered]@{
    ok = $ok
    message = $errorMessage
    smokeSummaryPath = $smokeSummaryPath
    smokeSummaryMarkdownPath = $smokeSummaryMarkdownPath
    guestRepoRoot = $GuestRepoRoot
    guestApythonRoot = $GuestApythonRoot
    guestArtifactDirectory = $GuestArtifactDirectory
    guestLogDirectory = $GuestLogDirectory
    scheduledTaskName = $ScheduledTaskName
    interactiveExecution = $true
    interactiveSessionId = $interactiveSessionId
    interactiveUserName = $interactiveUserName
    artifacts = $artifacts
    steps = @($stepResults)
}
$result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $ResultPath -Encoding UTF8

if ($ok) {
    exit 0
}
exit 1
'@
}

$effectiveDryRun = $true
if ($PSBoundParameters.ContainsKey('DryRun')) {
    $effectiveDryRun = [bool]$DryRun
}
if ($NoDryRun) {
    $effectiveDryRun = $false
}
if (-not $effectiveDryRun -and -not ($AllowMutation -and $IUnderstandThisRunsSmoke)) {
    Write-FailureAndExit -Message 'Non-dry-run MSI/GUI smoke requires -AllowMutation and -IUnderstandThisRunsSmoke.' -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}
if (-not (Test-Path -LiteralPath $ApprovalPacketPath)) {
    Write-FailureAndExit -Message "Approval packet not found: $ApprovalPacketPath" -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}

$resolvedHomelabRoot = Resolve-HomelabRoot -ExplicitRoot $HomelabRoot
$confirmScript = Join-Path $resolvedHomelabRoot 'tools\codex-runner\Confirm-CodexVmApprovalPacket.ps1'
if (-not (Test-Path -LiteralPath $confirmScript)) {
    Write-FailureAndExit -Message "Homelab approval confirmation script not found: $confirmScript" -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}

$confirmOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $confirmScript -ApprovalPacketPath $ApprovalPacketPath -Json 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-FailureAndExit -Message "Approval packet validation failed: $($confirmOutput -join ' ')" -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}

$packet = Get-Content -Raw -LiteralPath $ApprovalPacketPath | ConvertFrom-Json
if ($packet.vmName -ne 'LisanStudio-QA') {
    Write-FailureAndExit -Message "MSI/GUI smoke is bounded to LisanStudio-QA. Packet VM is '$($packet.vmName)'." -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}
if ($packet.route -ne 'project-vm') {
    Write-FailureAndExit -Message "MSI/GUI smoke requires route project-vm. Packet route is '$($packet.route)'." -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}
if ($packet.forbidden -notcontains 'active-whitedragon') {
    Write-FailureAndExit -Message 'Approval packet must forbid active WHITEDRAGON.' -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}
if (Test-ActiveWhitedragonTarget -Packet $packet) {
    Write-FailureAndExit -Message 'active WHITEDRAGON is forbidden for MSI/GUI smoke validation.' -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}

if (-not $OutputDirectory) {
    $OutputDirectory = New-DefaultOutputDirectory -ResolvedHomelabRoot $resolvedHomelabRoot
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$resolvedOutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$resultPath = Join-Path $resolvedOutputDirectory 'msi-gui-smoke-result.json'
$progressPath = Join-Path $resolvedOutputDirectory 'host-progress.jsonl'

function Write-SmokeProgress {
    param(
        [string]$Stage,
        [string]$Message,
        [object]$Data = $null
    )

    try {
        $entry = [ordered]@{
            timestamp = (Get-Date).ToString('o')
            stage = $Stage
            message = $Message
        }
        if ($null -ne $Data) {
            $entry.data = $Data
        }
        $entry | ConvertTo-Json -Depth 8 -Compress | Add-Content -LiteralPath $progressPath -Encoding UTF8
    } catch {
        # Progress breadcrumbs must never hide the real smoke failure.
    }
}

Write-SmokeProgress -Stage 'init' -Message 'MSI/GUI smoke host-side progress initialized.' -Data ([ordered]@{
        outputDirectory = $resolvedOutputDirectory
        targetVmName = $packet.vmName
    })

if ($effectiveDryRun) {
    $intentPath = Join-Path $resolvedOutputDirectory 'msi-gui-smoke-intent.json'
    $intent = [ordered]@{
        generatedAt = (Get-Date).ToString('o')
        project = 'Lisan Studio'
        approvalPacketPath = (Resolve-Path -LiteralPath $ApprovalPacketPath).Path
        homelabRoot = $resolvedHomelabRoot
        targetVmName = $packet.vmName
        dryRun = $true
        mutationPerformed = $false
        crossedApprovalGate = $false
        sourceOnly = $false
        projectScriptFlow = [ordered]@{
            build = 'scripts\build.ps1'
            package = 'scripts\package.ps1'
            installedSmoke = 'scripts\installed-smoke.ps1'
            msiSmoke = 'scripts\msi-smoke.ps1'
            releaseEvidence = 'scripts\release-evidence.ps1'
        }
        plannedSmokePhases = @(
            'Confirm isolated QA target and approval packet integrity.',
            'Copy the active arabic-code-studio-qt workspace and required apython payload to a per-run directory under C:\CodexRunner\work through Copy-Item -ToSession.',
            'Require a logged-on LISAN-QA\codexqa desktop session before GUI-sensitive work begins.',
            'Register and start a one-shot interactive scheduled task in the guest desktop session.',
            'Run scripts\validate.ps1 first through the interactive scheduled task to verify the Qt build and tests.',
            'Run scripts\release-evidence.ps1 through the same interactive scheduled task with guest-specific Bash, Python, WiX, and Qt license paths.',
            'Collect scripts\package.ps1, scripts\installed-smoke.ps1, scripts\msi-smoke.ps1, and release-evidence outputs, logs, checksums, and screenshots back to the host artifact lane.'
        )
        plannedChecks = @(
            'LisanStudio-QA only; active WHITEDRAGON stays untouched.',
            'Installed app launches from the packaged MSI and keeps Arabic output UTF-8 clean.',
            'Start Menu and Desktop shortcuts point to Lisan Studio.',
            'Release evidence captures VALIDATION_LOG.md, CHECKSUMS-SHA256.txt, KNOWN_ISSUES.md, and main-window.png when the guest desktop is interactive.',
            'If no logged-on codexqa desktop session is present, fail early with an interactive desktop session missing error.'
        )
        plannedArtifacts = @(
            'validate.stdout.log',
            'validate.stderr.log',
            'validate.result.json',
            'validate.log',
            'release-evidence.stdout.log',
            'release-evidence.stderr.log',
            'release-evidence.result.json',
            'release-evidence.log',
            'artifacts\release\0.1.0-beta\VALIDATION_LOG.md',
            'artifacts\release\0.1.0-beta\CHECKSUMS-SHA256.txt',
            'artifacts\release\0.1.0-beta\KNOWN_ISSUES.md',
            'artifacts\release\0.1.0-beta\screenshots\main-window.png',
            'artifacts\msi-smoke\install.log',
            'artifacts\msi-smoke\uninstall.log',
            'smoke-summary.json',
            'smoke-summary.md'
        )
    }
    $intent | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $intentPath -Encoding UTF8

    $result = [ordered]@{
        ok = $true
        dryRun = $true
        mutationPerformed = $false
        crossedApprovalGate = $false
        intentPath = $intentPath
        approvalPacketPath = $intent.approvalPacketPath
        resultPath = $resultPath
    }
    $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $resultPath -Encoding UTF8
    if ($Json) {
        $result | ConvertTo-Json -Depth 8
    } else {
        "MSI/GUI smoke dry-run intent written: $intentPath"
    }
    exit 0
}

if (-not (Get-IsAdministrator)) {
    Write-FailureAndExit -Message 'An elevated PowerShell session is required to run LisanStudio-QA MSI/GUI smoke through PowerShell Direct.' -AsJson:$Json -EffectiveDryRun:$effectiveDryRun -ResultPath $resultPath
}

try {
    Write-SmokeProgress -Stage 'credential' -Message 'Loading LisanStudio-QA guest credential metadata.'
    $credentialInfo = Get-LisanQaCredential
    $hostApythonRoot = Get-HostApythonRoot
    $hostRepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
    $guestArtifactLeaf = Split-Path -Leaf $resolvedOutputDirectory
    $guestWorkRoot = Join-Path 'C:\CodexRunner\work' $guestArtifactLeaf
    $guestRepoRoot = Join-Path $guestWorkRoot 'arabic-code-studio-qt'
    $guestApythonRoot = Join-Path $guestWorkRoot 'apython'
    $guestArtifactDirectory = Join-Path 'C:\CodexRunner\artifacts' $guestArtifactLeaf
    $guestLogDirectory = Join-Path 'C:\CodexRunner\logs' $guestArtifactLeaf
    $guestRunnerScriptPath = 'C:\CodexRunner\work\Invoke-LisanStudioVmSmokeInteractive.ps1'
    $guestRunnerResultPath = Join-Path $guestArtifactDirectory 'interactive-smoke-result.json'
    $guestRunnerContent = Get-GuestInteractiveSmokeRunnerContent

    Write-SmokeProgress -Stage 'pssession' -Message 'Opening PowerShell Direct session to LisanStudio-QA.' -Data ([ordered]@{
            guestCredentialUserName = $credentialInfo.userName
        })
    $session = New-PSSession -VMName 'LisanStudio-QA' -Credential $credentialInfo.credential
    Write-SmokeProgress -Stage 'pssession' -Message 'PowerShell Direct session opened.'
    $taskName = $null
    try {
        Write-SmokeProgress -Stage 'guest-reset' -Message 'Preparing guest work, artifact, and log directories.' -Data ([ordered]@{
                guestRepoRoot = $guestRepoRoot
                guestApythonRoot = $guestApythonRoot
                guestArtifactDirectory = $guestArtifactDirectory
                guestLogDirectory = $guestLogDirectory
            })
        Invoke-Command -Session $session -ArgumentList $guestRepoRoot, $guestApythonRoot, $guestArtifactDirectory, $guestLogDirectory -ScriptBlock {
            param(
                [string]$GuestRepoRoot,
                [string]$GuestApythonRoot,
                [string]$GuestArtifactDirectory,
                [string]$GuestLogDirectory
            )

            $ErrorActionPreference = 'Stop'
            foreach ($path in @(
                    'C:\CodexRunner',
                    'C:\CodexRunner\work',
                    'C:\CodexRunner\artifacts',
                    'C:\CodexRunner\logs',
                    (Split-Path -Parent $GuestRepoRoot),
                    (Split-Path -Parent $GuestApythonRoot),
                    $GuestArtifactDirectory,
                    $GuestLogDirectory
                )) {
                New-Item -ItemType Directory -Force -Path $path | Out-Null
            }

            foreach ($path in @($GuestRepoRoot, $GuestApythonRoot)) {
                if (Test-Path -LiteralPath $path) {
                    Remove-Item -LiteralPath $path -Recurse -Force
                }
            }
            New-Item -ItemType Directory -Force -Path $GuestRepoRoot | Out-Null
            New-Item -ItemType Directory -Force -Path $GuestApythonRoot | Out-Null
        } | Out-Null
        Write-SmokeProgress -Stage 'guest-reset' -Message 'Guest directories are ready.'

        $transferItems = @(
            '.gitignore',
            '.codex',
            'AGENTS.md',
            'CMakeLists.txt',
            'README.md',
            'docs',
            'licenses',
            'packaging',
            'qa',
            'resources',
            'samples',
            'scripts',
            'src',
            'tests'
        )

        foreach ($item in $transferItems) {
            $sourcePath = Join-Path $hostRepoRoot $item
            if (-not (Test-Path -LiteralPath $sourcePath)) {
                Write-SmokeProgress -Stage 'stage-repo' -Message 'Skipping missing transfer item.' -Data ([ordered]@{
                        item = $item
                    })
                continue
            }

            Write-SmokeProgress -Stage 'stage-repo' -Message 'Copying project item into guest workspace.' -Data ([ordered]@{
                    item = $item
                })
            if ((Get-Item -LiteralPath $sourcePath).PSIsContainer) {
                Copy-Item -ToSession $session -LiteralPath $sourcePath -Destination $guestRepoRoot -Recurse -Force
            } else {
                Copy-Item -ToSession $session -LiteralPath $sourcePath -Destination (Join-Path $guestRepoRoot $item) -Force
            }
            Write-SmokeProgress -Stage 'stage-repo' -Message 'Copied project item into guest workspace.' -Data ([ordered]@{
                    item = $item
                })
        }

        $hostApythonContents = Join-Path $hostApythonRoot '*'
        Write-SmokeProgress -Stage 'stage-apython' -Message 'Copying apython payload into guest workspace.' -Data ([ordered]@{
                hostApythonRoot = $hostApythonRoot
                guestApythonRoot = $guestApythonRoot
            })
        Copy-Item -ToSession $session -Path $hostApythonContents -Destination $guestApythonRoot -Recurse -Force
        Write-SmokeProgress -Stage 'stage-apython' -Message 'Copied apython payload into guest workspace.'

        Write-SmokeProgress -Stage 'stage-apython' -Message 'Verifying guest apython payload shape.'
        Invoke-Command -Session $session -ArgumentList $guestApythonRoot -ScriptBlock {
            param([string]$GuestApythonRoot)

            $requiredApythonFiles = @(
                (Join-Path $GuestApythonRoot 'LICENSE'),
                (Join-Path $GuestApythonRoot 'lughat_althuban.egg-info\PKG-INFO'),
                (Join-Path $GuestApythonRoot 'arabicpython\__init__.py'),
                (Join-Path $GuestApythonRoot 'arabicpython\cli.py'),
                (Join-Path $GuestApythonRoot 'arabicpython_kernel\__init__.py')
            )

            foreach ($requiredApythonFile in $requiredApythonFiles) {
                if (-not (Test-Path -LiteralPath $requiredApythonFile)) {
                    throw "Guest Apython payload file missing after staging: $requiredApythonFile"
                }
            }
        } | Out-Null
        Write-SmokeProgress -Stage 'stage-apython' -Message 'Guest apython payload shape verified.'

        Write-SmokeProgress -Stage 'scheduled-task' -Message 'Registering one-shot interactive scheduled task.'
        $taskSetup = Invoke-Command -Session $session -ArgumentList $guestRunnerScriptPath, $guestRunnerContent, $guestRunnerResultPath, $guestRepoRoot, $guestApythonRoot, $guestArtifactDirectory, $guestLogDirectory, $credentialInfo.userName -ScriptBlock {
            param(
                [string]$GuestRunnerScriptPath,
                [string]$GuestRunnerContent,
                [string]$GuestRunnerResultPath,
                [string]$GuestRepoRoot,
                [string]$GuestApythonRoot,
                [string]$GuestArtifactDirectory,
                [string]$GuestLogDirectory,
                [string]$ExpectedUserName
            )

            $ErrorActionPreference = 'Stop'

            function Quote-TaskValue {
                param([string]$Value)

                return '"' + $Value.Replace('"', '""') + '"'
            }

            $normalizedExpected = $ExpectedUserName.ToUpperInvariant()
            $expectedAccountName = if ($ExpectedUserName -match '\\([^\\]+)$') { $Matches[1] } else { $ExpectedUserName }
            $normalizedExpectedAccountName = $expectedAccountName.ToUpperInvariant()
            $interactiveExplorers = @(Get-Process -Name explorer -IncludeUserName -ErrorAction SilentlyContinue |
                    Where-Object { $_.UserName } |
                    Sort-Object StartTime -Descending)
            $interactiveExplorer = $interactiveExplorers |
                Where-Object { $_.UserName.ToUpperInvariant() -eq $normalizedExpected } |
                Select-Object -First 1
            if (-not $interactiveExplorer) {
                $interactiveExplorer = $interactiveExplorers |
                    Where-Object {
                        $actualAccountName = if ($_.UserName -match '\\([^\\]+)$') { $Matches[1] } else { $_.UserName }
                        $actualAccountName.ToUpperInvariant() -eq $normalizedExpectedAccountName
                    } |
                    Select-Object -First 1
            }

            if (-not $interactiveExplorer) {
                $observedSessions = @($interactiveExplorers |
                        Select-Object -First 10 |
                        ForEach-Object { "$($_.UserName) session $($_.SessionId)" })
                $observedText = if ($observedSessions.Count -gt 0) { $observedSessions -join '; ' } else { 'none' }
                throw "interactive desktop session missing for $ExpectedUserName in LisanStudio-QA. Sign into the guest desktop as codexqa before running MSI/GUI smoke. Observed explorer sessions: $observedText."
            }
            $scheduledTaskUserName = $interactiveExplorer.UserName

            foreach ($path in @(
                    (Split-Path -Parent $GuestRunnerScriptPath),
                    $GuestArtifactDirectory,
                    $GuestLogDirectory
                )) {
                New-Item -ItemType Directory -Force -Path $path | Out-Null
            }

            if (Test-Path -LiteralPath $GuestRunnerResultPath) {
                Remove-Item -LiteralPath $GuestRunnerResultPath -Force
            }
            Set-Content -LiteralPath $GuestRunnerScriptPath -Value $GuestRunnerContent -Encoding UTF8

            $taskName = 'LisanStudio-Smoke-' + ([guid]::NewGuid().ToString('N'))
            $taskArgument = @(
                '-NoProfile',
                '-ExecutionPolicy', 'Bypass',
                '-File', (Quote-TaskValue -Value $GuestRunnerScriptPath),
                '-GuestRepoRoot', (Quote-TaskValue -Value $GuestRepoRoot),
                '-GuestApythonRoot', (Quote-TaskValue -Value $GuestApythonRoot),
                '-GuestArtifactDirectory', (Quote-TaskValue -Value $GuestArtifactDirectory),
                '-GuestLogDirectory', (Quote-TaskValue -Value $GuestLogDirectory),
                '-ResultPath', (Quote-TaskValue -Value $GuestRunnerResultPath),
                '-ScheduledTaskName', (Quote-TaskValue -Value $taskName)
            ) -join ' '

            $action = New-ScheduledTaskAction -Execute 'powershell.exe' -Argument $taskArgument
            $principal = New-ScheduledTaskPrincipal -UserId $scheduledTaskUserName -LogonType Interactive -RunLevel Highest
            $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable -ExecutionTimeLimit (New-TimeSpan -Hours 2)
            Register-ScheduledTask -TaskName $taskName -Action $action -Principal $principal -Settings $settings -Description 'Lisan Studio project-owned interactive MSI/GUI smoke task.' | Out-Null
            Start-ScheduledTask -TaskName $taskName

            [ordered]@{
                taskName = $taskName
                taskResultPath = $GuestRunnerResultPath
                runnerScriptPath = $GuestRunnerScriptPath
                interactiveUserName = $interactiveExplorer.UserName
                interactiveSessionId = $interactiveExplorer.SessionId
                scheduledTaskUserName = $scheduledTaskUserName
            }
        }

        $taskName = $taskSetup.taskName
        Write-SmokeProgress -Stage 'scheduled-task' -Message 'Interactive scheduled task started.' -Data ([ordered]@{
                taskName = $taskName
                taskResultPath = $taskSetup.taskResultPath
                interactiveUserName = $taskSetup.interactiveUserName
                interactiveSessionId = $taskSetup.interactiveSessionId
                scheduledTaskUserName = $taskSetup.scheduledTaskUserName
            })
        $guestSmoke = $null
        $pollSnapshot = $null
        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        $timeout = [TimeSpan]::FromHours(2)
        $nextProgressAt = [TimeSpan]::Zero
        while ($stopwatch.Elapsed -lt $timeout) {
            $pollSnapshot = Invoke-Command -Session $session -ArgumentList $taskSetup.taskName, $taskSetup.taskResultPath -ScriptBlock {
                param(
                    [string]$TaskName,
                    [string]$TaskResultPath
                )

                $task = Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
                $taskInfo = if ($task) { Get-ScheduledTaskInfo -TaskName $TaskName -ErrorAction SilentlyContinue } else { $null }
                $resultExists = Test-Path -LiteralPath $TaskResultPath
                $result = $null
                if ($resultExists) {
                    $result = Get-Content -Raw -LiteralPath $TaskResultPath | ConvertFrom-Json
                }

                [ordered]@{
                    resultExists = $resultExists
                    result = $result
                    taskState = if ($task) { [string]$task.State } else { 'Missing' }
                    lastTaskResult = if ($taskInfo) { $taskInfo.LastTaskResult } else { $null }
                    lastRunTime = if ($taskInfo) { $taskInfo.LastRunTime.ToString('o') } else { $null }
                }
            }

            if ($pollSnapshot.resultExists) {
                $guestSmoke = $pollSnapshot.result
                break
            }

            if ($stopwatch.Elapsed -ge $nextProgressAt) {
                Write-SmokeProgress -Stage 'scheduled-task-poll' -Message 'Interactive scheduled task is still running.' -Data ([ordered]@{
                        elapsedSeconds = [int]$stopwatch.Elapsed.TotalSeconds
                        taskName = $taskSetup.taskName
                        taskState = $pollSnapshot.taskState
                        lastTaskResult = $pollSnapshot.lastTaskResult
                        resultExists = $pollSnapshot.resultExists
                    })
                $nextProgressAt = $stopwatch.Elapsed.Add([TimeSpan]::FromMinutes(1))
            }
            Start-Sleep -Seconds 5
        }

        if (-not $guestSmoke) {
            $stateText = if ($pollSnapshot) { "$($pollSnapshot.taskState) / $($pollSnapshot.lastTaskResult)" } else { 'unknown' }
            throw "Timed out waiting for interactive smoke task '$($taskSetup.taskName)' to finish in LisanStudio-QA. Last known state: $stateText"
        }

        $hostGuestArtifactsDirectory = Join-Path $resolvedOutputDirectory 'guest-artifacts'
        New-Item -ItemType Directory -Force -Path $hostGuestArtifactsDirectory | Out-Null
        Write-SmokeProgress -Stage 'copy-back' -Message 'Copying guest artifacts back to host.' -Data ([ordered]@{
                hostGuestArtifactsDirectory = $hostGuestArtifactsDirectory
            })

        foreach ($guestPath in @(
                $guestSmoke.smokeSummaryPath,
                $guestSmoke.smokeSummaryMarkdownPath,
                $guestSmoke.guestArtifactDirectory,
                $guestSmoke.guestLogDirectory,
                $guestSmoke.artifacts.releaseDir,
                $guestSmoke.artifacts.msiSmokeDir,
                $guestSmoke.artifacts.packagedMsi
            )) {
            if ([string]::IsNullOrWhiteSpace([string]$guestPath)) {
                continue
            }

            Write-SmokeProgress -Stage 'copy-back' -Message 'Copying guest path back to host.' -Data ([ordered]@{
                    guestPath = $guestPath
                })
            Copy-Item -FromSession $session -LiteralPath $guestPath -Destination $hostGuestArtifactsDirectory -Recurse -Force
            Write-SmokeProgress -Stage 'copy-back' -Message 'Copied guest path back to host.' -Data ([ordered]@{
                    guestPath = $guestPath
                })
        }

        $hostSummaryCopyPath = Join-Path $resolvedOutputDirectory 'smoke-summary.copy.json'
        $guestSmoke | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $hostSummaryCopyPath -Encoding UTF8

        $result = [ordered]@{
            ok = [bool]$guestSmoke.ok
            message = $guestSmoke.message
            dryRun = $false
            mutationPerformed = $true
            crossedApprovalGate = $true
            targetVmName = 'LisanStudio-QA'
            approvalPacketPath = (Resolve-Path -LiteralPath $ApprovalPacketPath).Path
            guestCredentialPath = $credentialInfo.path
            guestCredentialUserName = $credentialInfo.userName
            guestWorkRoot = $guestWorkRoot
            guestRepoRoot = $guestRepoRoot
            guestApythonRoot = $guestApythonRoot
            guestSummaryPath = $guestSmoke.smokeSummaryPath
            guestSummaryMarkdownPath = $guestSmoke.smokeSummaryMarkdownPath
            guestInteractiveRunnerPath = $taskSetup.runnerScriptPath
            guestInteractiveResultPath = $taskSetup.taskResultPath
            guestInteractiveSessionId = $taskSetup.interactiveSessionId
            guestInteractiveUserName = $taskSetup.interactiveUserName
            guestScheduledTaskUserName = $taskSetup.scheduledTaskUserName
            scheduledTaskName = $taskSetup.taskName
            hostSummaryCopyPath = $hostSummaryCopyPath
            hostArtifactsDirectory = $hostGuestArtifactsDirectory
            resultPath = $resultPath
            outputDirectory = $resolvedOutputDirectory
            steps = $guestSmoke.steps
            artifacts = $guestSmoke.artifacts
        }
        $result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $resultPath -Encoding UTF8
        Write-SmokeProgress -Stage 'complete' -Message 'MSI/GUI smoke completed.' -Data ([ordered]@{
                ok = [bool]$guestSmoke.ok
                resultPath = $resultPath
            })
        if ($Json) {
            $result | ConvertTo-Json -Depth 12
        } else {
            "MSI/GUI smoke result written: $resultPath"
        }
    } finally {
        if ($session) {
            if ($taskName) {
                Invoke-Command -Session $session -ArgumentList $taskName -ScriptBlock {
                    param([string]$TaskName)
                    Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false -ErrorAction SilentlyContinue
                } | Out-Null
            }
            Remove-PSSession -Session $session -ErrorAction SilentlyContinue
        }
    }
} catch {
    Write-SmokeProgress -Stage 'failure' -Message $_.Exception.Message
    Write-FailureAndExit -Message $_.Exception.Message -AsJson:$Json -EffectiveDryRun:$effectiveDryRun -ResultPath $resultPath
}
