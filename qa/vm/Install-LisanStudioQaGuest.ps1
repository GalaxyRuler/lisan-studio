[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [string]$ApprovalPacketPath,

    [string]$HomelabRoot,

    [string]$OutputDirectory,

    [switch]$DryRun,

    [switch]$NoDryRun,

    [switch]$AllowMutation,

    [switch]$IUnderstandThisMutatesGuest,

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

function Test-PathUnderRoot {
    param(
        [string]$Path,
        [string]$Root
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $false
    }

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $fullRoot = [System.IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    return $fullPath.StartsWith($fullRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
        $fullPath.TrimEnd('\').Equals($fullRoot.TrimEnd('\'), [System.StringComparison]::OrdinalIgnoreCase)
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
    return Join-Path $ResolvedHomelabRoot "tools\codex-runner\artifacts\project-guest-provisioning-$stamp"
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

$effectiveDryRun = $true
if ($PSBoundParameters.ContainsKey('DryRun')) {
    $effectiveDryRun = [bool]$DryRun
}
if ($NoDryRun) {
    $effectiveDryRun = $false
}
if (-not $effectiveDryRun -and -not ($AllowMutation -and $IUnderstandThisMutatesGuest)) {
    Write-FailureAndExit -Message 'Non-dry-run guest provisioning requires -AllowMutation and -IUnderstandThisMutatesGuest.' -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
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
    Write-FailureAndExit -Message "Guest provisioning is bounded to LisanStudio-QA. Packet VM is '$($packet.vmName)'." -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}
if ($packet.route -ne 'project-vm') {
    Write-FailureAndExit -Message "Guest provisioning requires route project-vm. Packet route is '$($packet.route)'." -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}
if ($packet.forbidden -notcontains 'active-whitedragon') {
    Write-FailureAndExit -Message 'Approval packet must forbid active WHITEDRAGON.' -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}
if (Test-ActiveWhitedragonTarget -Packet $packet) {
    Write-FailureAndExit -Message 'active WHITEDRAGON is forbidden for guest provisioning validation.' -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}
if (-not (Test-PathUnderRoot -Path $packet.runnerWorkPath -Root 'C:\CodexRunner\work')) {
    Write-FailureAndExit -Message "Runner work path is outside C:\CodexRunner\work: $($packet.runnerWorkPath)" -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}
if (-not (Test-PathUnderRoot -Path $packet.artifactsPath -Root 'C:\CodexRunner\artifacts')) {
    Write-FailureAndExit -Message "Artifacts path is outside C:\CodexRunner\artifacts: $($packet.artifactsPath)" -AsJson:$Json -EffectiveDryRun:$effectiveDryRun
}

if (-not $OutputDirectory) {
    $OutputDirectory = New-DefaultOutputDirectory -ResolvedHomelabRoot $resolvedHomelabRoot
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$resolvedOutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$resultPath = Join-Path $resolvedOutputDirectory 'guest-provisioning-result.json'

if ($effectiveDryRun) {
    $intentPath = Join-Path $resolvedOutputDirectory 'guest-provisioning-intent.json'
    $intent = [ordered]@{
        generatedAt = (Get-Date).ToString('o')
        project = 'Lisan Studio'
        approvalPacketPath = (Resolve-Path -LiteralPath $ApprovalPacketPath).Path
        homelabRoot = $resolvedHomelabRoot
        targetVmName = $packet.vmName
        dryRun = $true
        mutationPerformed = $false
        crossedApprovalGate = $false
        plannedGuestTasks = @(
            'Create C:\CodexRunner guest work, artifact, log, and bootstrap directories.',
            'Use PowerShell Direct to validate or install Microsoft.PowerShell, Git.Git, Python.Python.3.13, MSYS2.MSYS2, and WiXToolset.WiXCLI.',
            'Use MSYS2 pacman to install mingw-w64-ucrt-x86_64-gcc, mingw-w64-ucrt-x86_64-cmake, mingw-w64-ucrt-x86_64-ninja, and mingw-w64-ucrt-x86_64-qt6-base.',
            'Write guest-provisioning-readiness.json and tooling-report.json under the approved C:\CodexRunner artifact lane.'
        )
    }
    $intent | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $intentPath -Encoding UTF8

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
        "Guest provisioning dry-run intent written: $intentPath"
    }
    exit 0
}

if (-not (Get-IsAdministrator)) {
    Write-FailureAndExit -Message 'An elevated PowerShell session is required to provision LisanStudio-QA through PowerShell Direct.' -AsJson:$Json -EffectiveDryRun:$effectiveDryRun -ResultPath $resultPath
}

try {
    $credentialInfo = Get-LisanQaCredential
    $guestArtifactLeaf = Split-Path -Leaf $resolvedOutputDirectory
    $guestReport = Invoke-Command -VMName 'LisanStudio-QA' -Credential $credentialInfo.credential -ArgumentList $guestArtifactLeaf -ScriptBlock {
        param([string]$GuestArtifactLeaf)

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

            $pythonHomes = @()
            $searchRoot = Join-Path $env:LOCALAPPDATA 'Programs\Python'
            if (Test-Path -LiteralPath $searchRoot) {
                $pythonHomes += Get-ChildItem -LiteralPath $searchRoot -Directory -ErrorAction SilentlyContinue |
                    Sort-Object Name -Descending |
                    ForEach-Object { $_.FullName }
            }

            foreach ($candidate in $pythonHomes) {
                if (Test-PythonHomeCandidate -CandidatePath $candidate) {
                    return $candidate
                }
            }

            $pythonCommand = Get-Command -Name python.exe -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($pythonCommand) {
                $stdoutPath = Join-Path $env:TEMP ([System.IO.Path]::GetRandomFileName())
                $stderrPath = Join-Path $env:TEMP ([System.IO.Path]::GetRandomFileName())
                try {
                    $process = Start-Process -FilePath $pythonCommand.Source -ArgumentList @(
                        '-c',
                        'import pathlib, sys; print(pathlib.Path(sys.base_prefix).resolve())'
                    ) -Wait -PassThru -NoNewWindow -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
                    if ($process.ExitCode -eq 0) {
                        $reportedRoot = ((Get-Content -LiteralPath $stdoutPath -ErrorAction SilentlyContinue | Select-Object -First 1) -as [string]).Trim()
                        if (Test-PythonHomeCandidate -CandidatePath $reportedRoot) {
                            return (Resolve-Path -LiteralPath $reportedRoot).Path
                        }
                    }
                } finally {
                    Remove-Item -LiteralPath $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue
                }
            }

            return $null
        }

        function Invoke-WingetInstall {
            param(
                [Parameter(Mandatory = $true)]
                [string]$PackageId,

                [Parameter(Mandatory = $true)]
                [string]$DisplayName,

                [string[]]$ResolveCandidates,

                [string]$CommandName,

                [string]$LogPath,

                [string]$Scope,

                [string]$Location,

                [switch]$ForceInstall
            )

            $before = Resolve-FirstExistingPath -Candidates $ResolveCandidates -CommandName $CommandName
            if ($before) {
                return [ordered]@{
                    packageId = $PackageId
                    displayName = $DisplayName
                    changed = $false
                    ok = $true
                    path = $before
                    logPath = $LogPath
                    exitCode = 0
                    installAttempted = $false
                }
            }

            $arguments = @(
                'install',
                '--exact',
                '--id', $PackageId,
                '--accept-package-agreements',
                '--accept-source-agreements',
                '--disable-interactivity',
                '--silent'
            )
            if ($Scope) {
                $arguments += @('--scope', $Scope)
            }
            if ($Location) {
                $arguments += @('--location', $Location)
            }
            if ($ForceInstall) {
                $arguments += '--force'
            }
            $invoke = Invoke-NativeProcess -FilePath $script:WingetPath -ArgumentList $arguments -LogPath $LogPath
            $output = @($invoke.output)
            $exitCode = [int]$invoke.exitCode

            $after = Resolve-FirstExistingPath -Candidates $ResolveCandidates -CommandName $CommandName
            $ok = ($exitCode -eq 0) -and -not [string]::IsNullOrWhiteSpace($after)
            return [ordered]@{
                packageId = $PackageId
                displayName = $DisplayName
                changed = $true
                ok = $ok
                path = $after
                logPath = $LogPath
                exitCode = $exitCode
                installAttempted = $true
                tail = if ($output) { @($output | Select-Object -Last 20) -join [Environment]::NewLine } else { '' }
            }
        }

        function Invoke-NativeProcess {
            param(
                [Parameter(Mandatory = $true)]
                [string]$FilePath,

                [Parameter(Mandatory = $true)]
                [string[]]$ArgumentList,

                [Parameter(Mandatory = $true)]
                [string]$LogPath
            )

            $argumentLine = @(
                $ArgumentList | ForEach-Object {
                    if ($_ -match '[\s"]') {
                        '"' + ($_ -replace '\\(?=")', '\\' -replace '"', '\"') + '"'
                    } else {
                        $_
                    }
                }
            ) -join ' '
            $stdoutPath = Join-Path $env:TEMP ([System.IO.Path]::GetRandomFileName())
            $stderrPath = Join-Path $env:TEMP ([System.IO.Path]::GetRandomFileName())
            try {
                $process = Start-Process -FilePath $FilePath -ArgumentList $argumentLine -Wait -PassThru -NoNewWindow `
                    -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
                $stdout = if (Test-Path -LiteralPath $stdoutPath) { Get-Content -LiteralPath $stdoutPath -ErrorAction SilentlyContinue } else { @() }
                $stderr = if (Test-Path -LiteralPath $stderrPath) { Get-Content -LiteralPath $stderrPath -ErrorAction SilentlyContinue } else { @() }
                $combined = @($stdout) + @($stderr)
                $combined | Set-Content -LiteralPath $LogPath -Encoding UTF8
                return [ordered]@{
                    exitCode = $process.ExitCode
                    output = @($combined)
                }
            } finally {
                Remove-Item -LiteralPath $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue
            }
        }

        function Wait-ForPacmanDatabase {
            param(
                [Parameter(Mandatory = $true)]
                [string]$LockPath,

                [int]$MaxAttempts = 12,

                [int]$SleepSeconds = 5
            )

            $attemptLog = New-Object System.Collections.ArrayList
            for ($attempt = 1; $attempt -le $MaxAttempts; $attempt++) {
                $lockExists = Test-Path -LiteralPath $LockPath
                if (-not $lockExists) {
                    [void]$attemptLog.Add("Attempt ${attempt}: pacman database lock not present.")
                    return [ordered]@{
                        ok = $true
                        actionLog = @($attemptLog)
                    }
                }

                $pacmanProcesses = @(
                    Get-CimInstance -ClassName Win32_Process -ErrorAction SilentlyContinue |
                        Where-Object {
                            $_.Name -in @('pacman.exe', 'bash.exe', 'sh.exe', 'msys-2.0.dll') -or
                            (($_.CommandLine -as [string]) -match 'pacman')
                        }
                )

                if ($pacmanProcesses.Count -gt 0) {
                    [void]$attemptLog.Add("Attempt ${attempt}: pacman lock present while an MSYS2 or pacman process appears active; waiting $SleepSeconds seconds.")
                    Start-Sleep -Seconds $SleepSeconds
                    continue
                }

                try {
                    Remove-Item -LiteralPath $LockPath -Force
                    [void]$attemptLog.Add("Attempt ${attempt}: removed stale pacman lock file at $LockPath.")
                } catch {
                    [void]$attemptLog.Add("Attempt ${attempt}: failed to remove stale pacman lock file at $LockPath. $($_.Exception.Message)")
                }

                Start-Sleep -Seconds 2
                if (-not (Test-Path -LiteralPath $LockPath)) {
                    return [ordered]@{
                        ok = $true
                        actionLog = @($attemptLog)
                    }
                }
            }

            return [ordered]@{
                ok = $false
                message = "MSYS2 pacman database remained locked: $LockPath"
                actionLog = @($attemptLog)
            }
        }

        $directories = @(
            'C:\CodexRunner',
            'C:\CodexRunner\work',
            'C:\CodexRunner\artifacts',
            'C:\CodexRunner\logs',
            'C:\CodexRunner\bootstrap'
        )
        foreach ($directory in $directories) {
            New-Item -ItemType Directory -Force -Path $directory | Out-Null
        }

        $guestArtifactDirectory = Join-Path 'C:\CodexRunner\artifacts' $GuestArtifactLeaf
        $guestLogDirectory = Join-Path 'C:\CodexRunner\logs' $GuestArtifactLeaf
        New-Item -ItemType Directory -Force -Path $guestArtifactDirectory, $guestLogDirectory | Out-Null

        $toolingReportPath = Join-Path $guestArtifactDirectory 'tooling-report.json'
        $guestReportPath = Join-Path $guestArtifactDirectory 'guest-provisioning-readiness.json'

        $script:WingetPath = Resolve-FirstExistingPath -Candidates @() -CommandName 'winget.exe'
        if (-not $script:WingetPath) {
            throw 'winget.exe is required in LisanStudio-QA for tooling-first provisioning.'
        }

        $toolingChanges = New-Object System.Collections.ArrayList
        $installFailures = New-Object System.Collections.ArrayList

        $wingetPackages = @(
            [ordered]@{
                packageId = 'Microsoft.PowerShell'
                displayName = 'PowerShell 7'
                candidates = @(
                    (Join-Path $env:ProgramFiles 'PowerShell\7\pwsh.exe'),
                    (Join-Path $env:LOCALAPPDATA 'Microsoft\WindowsApps\pwsh.exe')
                )
                commandName = 'pwsh.exe'
            },
            [ordered]@{
                packageId = 'Git.Git'
                displayName = 'Git'
                candidates = @(
                    (Join-Path $env:ProgramFiles 'Git\cmd\git.exe'),
                    (Join-Path $env:ProgramFiles 'Git\bin\git.exe'),
                    (Join-Path $env:LOCALAPPDATA 'Programs\Git\cmd\git.exe'),
                    (Join-Path $env:LOCALAPPDATA 'Programs\Git\bin\git.exe')
                )
                commandName = 'git.exe'
            },
            [ordered]@{
                packageId = 'Python.Python.3.13'
                displayName = 'Python 3.13'
                candidates = @(
                    (Join-Path $env:LOCALAPPDATA 'Programs\Python\Python313\python.exe'),
                    (Join-Path $env:ProgramFiles 'Python313\python.exe')
                )
                commandName = $null
                scope = 'user'
                location = (Join-Path $env:LOCALAPPDATA 'Programs\Python\Python313')
                forceInstall = $true
            },
            [ordered]@{
                packageId = 'MSYS2.MSYS2'
                displayName = 'MSYS2'
                candidates = @(
                    'C:\msys64\usr\bin\bash.exe'
                )
                commandName = $null
            },
            [ordered]@{
                packageId = 'WiXToolset.WiXCLI'
                displayName = 'WiX Toolset 7'
                candidates = @(
                    'C:\Program Files\WiX Toolset v7.0\bin\wix.exe'
                )
                commandName = 'wix.exe'
            }
        )

        foreach ($package in $wingetPackages) {
            $logPath = Join-Path $guestLogDirectory ("winget-{0}.log" -f ($package.packageId -replace '[^A-Za-z0-9]+', '-'))
            $result = Invoke-WingetInstall `
                -PackageId $package.packageId `
                -DisplayName $package.displayName `
                -ResolveCandidates $package.candidates `
                -CommandName $package.commandName `
                -LogPath $logPath `
                -Scope $package.scope `
                -Location $package.location `
                -ForceInstall:([bool]$package.forceInstall)
            [void]$toolingChanges.Add($result)
            if (-not $result.ok) {
                [void]$installFailures.Add("winget install failed for $($package.displayName) ($($package.packageId)). See $logPath")
            }
        }

        $bashPath = 'C:\msys64\usr\bin\bash.exe'
        $msys2LogPath = Join-Path $guestLogDirectory 'msys2-pacman.log'
        $msys2Packages = @(
            'mingw-w64-ucrt-x86_64-gcc',
            'mingw-w64-ucrt-x86_64-cmake',
            'mingw-w64-ucrt-x86_64-ninja',
            'mingw-w64-ucrt-x86_64-qt6-base'
        )
        if (Test-Path -LiteralPath $bashPath) {
            $pacmanLockPath = 'C:\msys64\var\lib\pacman\db.lck'
            $lockCheck = Wait-ForPacmanDatabase -LockPath $pacmanLockPath
            $pacmanCommand = 'pacman -Sy --noconfirm --needed {0}' -f ($msys2Packages -join ' ')
            $pacmanAttempts = New-Object System.Collections.ArrayList

            foreach ($line in @($lockCheck.actionLog)) {
                [void]$pacmanAttempts.Add($line)
            }

            if (-not $lockCheck.ok) {
                [void]$pacmanAttempts.Add($lockCheck.message)
                $pacmanExitCode = 1
                $pacmanOutput = @($lockCheck.message)
                $pacmanOutput | Set-Content -LiteralPath $msys2LogPath -Encoding UTF8
            } else {
                $invoke = Invoke-NativeProcess -FilePath $bashPath -ArgumentList @('-lc', $pacmanCommand) -LogPath $msys2LogPath
                $pacmanExitCode = [int]$invoke.exitCode
                $pacmanOutput = @($invoke.output)
                foreach ($line in @($pacmanOutput)) {
                    [void]$pacmanAttempts.Add($line)
                }

                if ($pacmanExitCode -ne 0 -and (($pacmanOutput -join [Environment]::NewLine) -match 'unable to lock database')) {
                    $retryLockCheck = Wait-ForPacmanDatabase -LockPath $pacmanLockPath
                    foreach ($line in @($retryLockCheck.actionLog)) {
                        [void]$pacmanAttempts.Add($line)
                    }
                    if ($retryLockCheck.ok) {
                        $retryInvoke = Invoke-NativeProcess -FilePath $bashPath -ArgumentList @('-lc', $pacmanCommand) -LogPath $msys2LogPath
                        $pacmanExitCode = [int]$retryInvoke.exitCode
                        $pacmanOutput = @($retryInvoke.output)
                        foreach ($line in @($pacmanOutput)) {
                            [void]$pacmanAttempts.Add($line)
                        }
                    } else {
                        [void]$pacmanAttempts.Add($retryLockCheck.message)
                    }
                }

                @($pacmanAttempts) | Set-Content -LiteralPath $msys2LogPath -Encoding UTF8
            }

            $pacmanResult = [ordered]@{
                packageManager = 'pacman'
                packages = $msys2Packages
                logPath = $msys2LogPath
                exitCode = $pacmanExitCode
                ok = ($pacmanExitCode -eq 0)
                changed = $true
                tail = if ($pacmanAttempts.Count -gt 0) { @($pacmanAttempts | Select-Object -Last 20) -join [Environment]::NewLine } else { '' }
            }
            [void]$toolingChanges.Add($pacmanResult)
            if (-not $pacmanResult.ok) {
                [void]$installFailures.Add("MSYS2 package install failed. See $msys2LogPath")
            }
        } else {
            [void]$installFailures.Add("MSYS2 bash missing after installation attempt: $bashPath")
        }

        $pythonRoot = Resolve-PythonRoot
        $gitPath = Resolve-FirstExistingPath -Candidates @(
            (Join-Path $env:ProgramFiles 'Git\cmd\git.exe'),
            (Join-Path $env:ProgramFiles 'Git\bin\git.exe'),
            (Join-Path $env:LOCALAPPDATA 'Programs\Git\cmd\git.exe'),
            (Join-Path $env:LOCALAPPDATA 'Programs\Git\bin\git.exe')
        ) -CommandName 'git.exe'
        $pwshPath = Resolve-FirstExistingPath -Candidates @(
            (Join-Path $env:ProgramFiles 'PowerShell\7\pwsh.exe'),
            (Join-Path $env:LOCALAPPDATA 'Microsoft\WindowsApps\pwsh.exe')
        ) -CommandName 'pwsh.exe'
        $wixPath = Resolve-FirstExistingPath -Candidates @(
            'C:\Program Files\WiX Toolset v7.0\bin\wix.exe'
        ) -CommandName 'wix.exe'
        $cmakePath = Resolve-FirstExistingPath -Candidates @(
            'C:\msys64\ucrt64\bin\cmake.exe'
        ) -CommandName $null
        $ninjaPath = Resolve-FirstExistingPath -Candidates @(
            'C:\msys64\ucrt64\bin\ninja.exe'
        ) -CommandName $null
        $windeployQtPath = Resolve-FirstExistingPath -Candidates @(
            'C:\msys64\ucrt64\bin\windeployqt6.exe'
        ) -CommandName $null
        $qtLicenseRoot = if (Test-Path -LiteralPath 'C:\msys64\ucrt64\share\licenses\qt6-base') {
            (Resolve-Path -LiteralPath 'C:\msys64\ucrt64\share\licenses\qt6-base').Path
        } else {
            $null
        }
        $msbuildPath = Resolve-FirstExistingPath -Candidates @(
            (Join-Path $env:ProgramFiles 'Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'),
            (Join-Path $env:ProgramFiles 'Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe')
        ) -CommandName 'msbuild.exe'

        $toolSnapshots = @(
            [ordered]@{
                key = 'pwsh'
                displayName = 'PowerShell 7'
                packageId = 'Microsoft.PowerShell'
                required = $true
                path = $pwshPath
                rationale = 'Requested by the project-owned guest provisioning lane.'
            },
            [ordered]@{
                key = 'git'
                displayName = 'Git'
                packageId = 'Git.Git'
                required = $true
                path = $gitPath
                rationale = 'scripts\release-evidence.ps1 records branch, commit, and status through git.'
            },
            [ordered]@{
                key = 'python'
                displayName = 'Python 3.13'
                packageId = 'Python.Python.3.13'
                required = $true
                path = if ($pythonRoot) { Join-Path $pythonRoot 'python.exe' } else { $null }
                root = $pythonRoot
                rationale = 'scripts\package.ps1 stages the bundled runtime and copies LICENSE.txt from PythonRoot.'
            },
            [ordered]@{
                key = 'bash'
                displayName = 'MSYS2 bash'
                packageId = 'MSYS2.MSYS2'
                required = $true
                path = if (Test-Path -LiteralPath $bashPath) { $bashPath } else { $null }
                rationale = 'scripts\build.ps1, scripts\validate.ps1, and scripts\package.ps1 invoke MSYS2 bash.'
            },
            [ordered]@{
                key = 'cmake'
                displayName = 'CMake'
                packageId = 'mingw-w64-ucrt-x86_64-cmake'
                required = $true
                path = $cmakePath
                rationale = 'scripts\build.ps1 and scripts\validate.ps1 configure the Qt build with CMake.'
            },
            [ordered]@{
                key = 'ninja'
                displayName = 'Ninja'
                packageId = 'mingw-w64-ucrt-x86_64-ninja'
                required = $true
                path = $ninjaPath
                rationale = 'scripts\build.ps1 and scripts\validate.ps1 build through Ninja.'
            },
            [ordered]@{
                key = 'qt'
                displayName = 'Qt UCRT64 runtime tools'
                packageId = 'mingw-w64-ucrt-x86_64-qt6-base'
                required = $true
                path = $windeployQtPath
                licenseRoot = $qtLicenseRoot
                rationale = 'scripts\package.ps1 calls windeployqt6 and stages Qt license payloads.'
            },
            [ordered]@{
                key = 'wix'
                displayName = 'WiX Toolset 7'
                packageId = 'WiXToolset.WiXCLI'
                required = $true
                path = $wixPath
                rationale = 'scripts\package.ps1 builds the MSI through wix.exe.'
            },
            [ordered]@{
                key = 'msbuild'
                displayName = 'MSBuild'
                packageId = $null
                required = $false
                path = $msbuildPath
                rationale = 'Current repo scripts do not call MSBuild or Visual Studio build tools directly.'
            }
        )

        $missingRequired = @($toolSnapshots | Where-Object { $_.required -and [string]::IsNullOrWhiteSpace($_.path) })
        $toolingOk = (@($missingRequired).Count -eq 0)
        $readinessWarnings = @($installFailures)

        $os = Get-CimInstance -ClassName Win32_OperatingSystem
        $computer = Get-CimInstance -ClassName Win32_ComputerSystem

        $toolingReport = [ordered]@{
            ok = $toolingOk
            generatedAt = (Get-Date).ToString('o')
            computerName = $env:COMPUTERNAME
            userName = [Security.Principal.WindowsIdentity]::GetCurrent().Name
            guestArtifactDirectory = $guestArtifactDirectory
            guestLogDirectory = $guestLogDirectory
            runnerDirectories = $directories
            toolingInstallAttempted = $true
            toolingChanges = @($toolingChanges)
            tools = $toolSnapshots
            missingRequiredTools = @($missingRequired | ForEach-Object { $_.displayName })
            installFailures = @($installFailures)
            readinessWarnings = @($readinessWarnings)
            repoScriptRequirements = [ordered]@{
                buildScript = 'scripts\build.ps1'
                validateScript = 'scripts\validate.ps1'
                packageScript = 'scripts\package.ps1'
                releaseEvidenceScript = 'scripts\release-evidence.ps1'
                msbuildRequired = $false
            }
        }
        $toolingReport | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $toolingReportPath -Encoding UTF8

        $readinessReport = [ordered]@{
            ok = $toolingOk
            generatedAt = (Get-Date).ToString('o')
            computerName = $env:COMPUTERNAME
            userName = [Security.Principal.WindowsIdentity]::GetCurrent().Name
            directories = $directories
            guestArtifactDirectory = $guestArtifactDirectory
            guestLogDirectory = $guestLogDirectory
            guestReportPath = $guestReportPath
            toolingReportPath = $toolingReportPath
            os = [ordered]@{
                caption = $os.Caption
                version = $os.Version
                buildNumber = $os.BuildNumber
                osArchitecture = $os.OSArchitecture
            }
            computer = [ordered]@{
                manufacturer = $computer.Manufacturer
                model = $computer.Model
                totalPhysicalMemory = [UInt64]$computer.TotalPhysicalMemory
            }
            tools = $toolSnapshots
            toolingInstallAttempted = $true
            mutationPerformed = $true
            installFailures = @($installFailures)
            readinessWarnings = @($readinessWarnings)
        }
        $readinessReport | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $guestReportPath -Encoding UTF8

        return [ordered]@{
            ok = $toolingOk
            message = if ($toolingOk) { $null } else { "Tooling-first guest provisioning left missing requirements: $(@($missingRequired | ForEach-Object { $_.displayName }) -join ', ')" }
            generatedAt = (Get-Date).ToString('o')
            guestReportPath = $guestReportPath
            toolingReportPath = $toolingReportPath
            guestArtifactDirectory = $guestArtifactDirectory
            guestLogDirectory = $guestLogDirectory
            toolingInstallAttempted = $true
            mutationPerformed = $true
            crossedApprovalGate = $true
            toolSnapshots = $toolSnapshots
            installFailures = @($installFailures)
            readinessWarnings = @($readinessWarnings)
        }
    }

    $hostGuestReportCopyPath = Join-Path $resolvedOutputDirectory 'guest-provisioning-readiness.copy.json'
    $hostToolingReportCopyPath = Join-Path $resolvedOutputDirectory 'tooling-report.copy.json'
    $guestReport | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $hostGuestReportCopyPath -Encoding UTF8
    [ordered]@{
        ok = [bool]$guestReport.ok
        generatedAt = $guestReport.generatedAt
        guestReportPath = $guestReport.guestReportPath
        toolingReportPath = $guestReport.toolingReportPath
        toolingInstallAttempted = [bool]$guestReport.toolingInstallAttempted
        tools = $guestReport.toolSnapshots
        installFailures = $guestReport.installFailures
        readinessWarnings = $guestReport.readinessWarnings
    } | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $hostToolingReportCopyPath -Encoding UTF8

    $result = [ordered]@{
        ok = [bool]$guestReport.ok
        message = $guestReport.message
        dryRun = $false
        mutationPerformed = $true
        crossedApprovalGate = $true
        targetVmName = 'LisanStudio-QA'
        approvalPacketPath = (Resolve-Path -LiteralPath $ApprovalPacketPath).Path
        guestCredentialPath = $credentialInfo.path
        guestCredentialUserName = $credentialInfo.userName
        guestReportPath = $guestReport.guestReportPath
        toolingReportPath = $guestReport.toolingReportPath
        hostGuestReportCopyPath = $hostGuestReportCopyPath
        hostToolingReportCopyPath = $hostToolingReportCopyPath
        installFailures = $guestReport.installFailures
        readinessWarnings = $guestReport.readinessWarnings
        resultPath = $resultPath
        outputDirectory = $resolvedOutputDirectory
    }
    $result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $resultPath -Encoding UTF8
    if ($Json) {
        $result | ConvertTo-Json -Depth 12
    } else {
        "Guest provisioning result written: $resultPath"
    }
} catch {
    Write-FailureAndExit -Message $_.Exception.Message -AsJson:$Json -EffectiveDryRun:$effectiveDryRun -ResultPath $resultPath
}
