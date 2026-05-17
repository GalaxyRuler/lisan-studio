param(
    [string]$ProductVersion = "0.1.0",
    [string]$ReleaseLabel = "0.1.0-beta",
    [string]$ApythonRoot = "C:\Users\Admin\apython",
    [string]$PythonRoot = "C:\Users\Admin\AppData\Local\Programs\Python\Python313",
    [string]$BashPath = "C:\msys64\usr\bin\bash.exe",
    [string]$WindeployQtPath = "C:\msys64\ucrt64\bin\windeployqt6.exe",
    [string]$WixPath = "C:\Program Files\WiX Toolset v7.0\bin\wix.exe",
    [string]$QtLicenseRoot = "C:\msys64\ucrt64\share\licenses\qt6-base",
    [string]$SourceMetadataPath
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $SourceMetadataPath) {
    $SourceMetadataPath = Join-Path $repo ".codex\source-metadata.json"
}
$releaseDir = Join-Path $repo "artifacts\release\$ReleaseLabel"
$logsDir = Join-Path $releaseDir "logs"
$screenshotsDir = Join-Path $releaseDir "screenshots"
$msiFileName = "LisanStudio-$ProductVersion-beta.msi"
$msiPath = Join-Path (Join-Path $repo "artifacts") $msiFileName
$installRoot = Join-Path $env:LOCALAPPDATA "LisanStudio"
$app = Join-Path $installRoot "LisanStudio.exe"

New-Item -ItemType Directory -Force -Path $releaseDir, $logsDir, $screenshotsDir | Out-Null

function Invoke-LoggedStep {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$Command
    )

    $logPath = Join-Path $logsDir "$Name.log"
    $started = Get-Date
    $outputLines = [System.Collections.Generic.List[string]]::new()
    try {
        & $Command 2>&1 | ForEach-Object {
            [void]$outputLines.Add($_.ToString())
        }
        $outputLines | Set-Content -LiteralPath $logPath -Encoding UTF8
        [PSCustomObject]@{
            Name = $Name
            Status = "PASS"
            Started = $started
            Finished = Get-Date
            Log = $logPath
        }
    } catch {
        if ($outputLines.Count -gt 0) {
            $outputLines | Set-Content -LiteralPath $logPath -Encoding UTF8
        } else {
            @() | Set-Content -LiteralPath $logPath -Encoding UTF8
        }
        @("", "ERROR: $($_.Exception.Message)", "", $_.ScriptStackTrace) | Add-Content -LiteralPath $logPath -Encoding UTF8
        throw
    }
}

function Get-LisanInstalledProducts {
    $installer = New-Object -ComObject WindowsInstaller.Installer
    foreach ($product in @($installer.ProductsEx("", "", 7))) {
        if ($product.InstallProperty("ProductName") -eq "Lisan Studio") {
            [PSCustomObject]@{
                ProductCode = $product.ProductCode()
                Name = $product.InstallProperty("ProductName")
                LocalPackage = $product.InstallProperty("LocalPackage")
            }
        }
    }
}

function Save-InstalledAppScreenshot {
    Add-Type -AssemblyName System.Drawing
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class LisanReleaseWindowOps {
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
  [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
  [DllImport("dwmapi.dll")] public static extern int DwmGetWindowAttribute(IntPtr hWnd, int attr, out RECT rect, int size);
}
'@

    Get-Process LisanStudio -ErrorAction SilentlyContinue | Stop-Process -Force
    $sampleProject = Join-Path $installRoot "samples\torture-project"
    $screenshotSmokeExitMs = 30000
    $screenshotArguments = @($sampleProject, "--smoke-exit-ms", "$screenshotSmokeExitMs")
    $process = Start-Process -FilePath $app -ArgumentList $screenshotArguments -PassThru
    try {
        $handle = [IntPtr]::Zero
        for ($index = 0; $index -lt 60; ++$index) {
            Start-Sleep -Milliseconds 200
            $process.Refresh()
            if ($process.MainWindowHandle -ne 0) {
                $handle = $process.MainWindowHandle
                break
            }
        }
        if ($handle -eq [IntPtr]::Zero) {
            throw "Lisan Studio did not expose a main window handle for screenshot capture."
        }

        $secondary = [System.Windows.Forms.Screen]::AllScreens | Where-Object { -not $_.Primary } | Select-Object -First 1
        if ($secondary) {
            $x = $secondary.WorkingArea.X + 40
            $y = $secondary.WorkingArea.Y + 80
        } else {
            $x = 100
            $y = 100
        }

        [LisanReleaseWindowOps]::ShowWindow($handle, 9) | Out-Null
        [LisanReleaseWindowOps]::MoveWindow($handle, $x, $y, 1600, 950, $true) | Out-Null
        Start-Sleep -Milliseconds 900

        $rect = New-Object LisanReleaseWindowOps+RECT
        $dwmResult = [LisanReleaseWindowOps]::DwmGetWindowAttribute(
            $handle,
            9,
            [ref]$rect,
            [System.Runtime.InteropServices.Marshal]::SizeOf([type][LisanReleaseWindowOps+RECT])
        )
        if ($dwmResult -ne 0) {
            [LisanReleaseWindowOps]::GetWindowRect($handle, [ref]$rect) | Out-Null
        }
        $width = [Math]::Max(1, $rect.Right - $rect.Left)
        $height = [Math]::Max(1, $rect.Bottom - $rect.Top)
        $bitmap = New-Object System.Drawing.Bitmap($width, $height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            try {
                $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
            } catch {
                $hdc = $graphics.GetHdc()
                try {
                    $printed = [LisanReleaseWindowOps]::PrintWindow($handle, $hdc, 2)
                } finally {
                    $graphics.ReleaseHdc($hdc)
                }
                if (-not $printed) {
                    throw "Screenshot capture failed for Lisan Studio window."
                }
            }
            $screenshotPath = Join-Path $screenshotsDir "main-window.png"
            $bitmap.Save($screenshotPath, [System.Drawing.Imaging.ImageFormat]::Png)
            $screenshotPath
        } finally {
            $graphics.Dispose()
            $bitmap.Dispose()
        }
    } finally {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }
}

$steps = @()
$steps += Invoke-LoggedStep -Name "package" -Command {
    & (Join-Path $PSScriptRoot "package.ps1") `
        -ProductVersion $ProductVersion `
        -ApythonRoot $ApythonRoot `
        -PythonRoot $PythonRoot `
        -BashPath $BashPath `
        -WindeployQtPath $WindeployQtPath `
        -WixPath $WixPath `
        -QtLicenseRoot $QtLicenseRoot
}
$steps += Invoke-LoggedStep -Name "msi-smoke-keep-installed" -Command {
    & (Join-Path $PSScriptRoot "msi-smoke.ps1") -MsiPath $msiPath -KeepInstalled
}

if (-not (Test-Path -LiteralPath $msiPath)) {
    throw "Release MSI missing after package step: $msiPath"
}
if (-not (Test-Path -LiteralPath $app)) {
    throw "Installed app missing after MSI smoke: $app"
}

$screenshot = Save-InstalledAppScreenshot
$hashRows = Get-FileHash -Algorithm SHA256 -LiteralPath $msiPath, $app
$checksumsPath = Join-Path $releaseDir "CHECKSUMS-SHA256.txt"
$hashRows | ForEach-Object { "$($_.Hash)  $($_.Path)" } | Set-Content -LiteralPath $checksumsPath -Encoding ASCII

$knownIssuesPath = Join-Path $releaseDir "KNOWN_ISSUES.md"
@"
# Lisan Studio $ReleaseLabel Known Issues

- Private beta only; no public distribution yet.
- Deferred features: Git UI, AI panel, auto-update, and plugin system are not included in this beta.
- Manual QA is not complete until the installed-app checklist is recorded.
- Manual Arabic editor torture validation is still required before tagging.
- `windeployqt6` may warn that Qt translations and DirectX shader compiler DLLs are unavailable in this local toolchain; these are tracked as non-blocking for the current private beta smoke.
"@ | Set-Content -LiteralPath $knownIssuesPath -Encoding UTF8

$gitCommit = 'unavailable'
$gitBranch = 'unavailable'
$gitStatus = 'Git metadata unavailable in this workspace.'
if (Test-Path -LiteralPath $SourceMetadataPath) {
    try {
        $sourceMetadata = Get-Content -Raw -LiteralPath $SourceMetadataPath | ConvertFrom-Json
        if ($sourceMetadata.commit) {
            $gitCommit = [string]$sourceMetadata.commit
        }
        if ($sourceMetadata.branch) {
            $gitBranch = [string]$sourceMetadata.branch
        }
        if ($sourceMetadata.status) {
            $gitStatus = [string]$sourceMetadata.status
        }
    } catch {
        $gitStatus = "Source metadata unavailable: $($_.Exception.Message)"
    }
}
try {
    $isInsideWorkTree = if ($gitCommit -eq 'unavailable' -or $gitBranch -eq 'unavailable') {
        (& git -C $repo rev-parse --is-inside-work-tree 2>$null)
    } else {
        $null
    }
    if ($isInsideWorkTree -and $LASTEXITCODE -eq 0 -and $isInsideWorkTree.Trim() -eq 'true') {
        $gitCommit = (& git -C $repo rev-parse --short HEAD).Trim()
        $gitBranch = (& git -C $repo branch --show-current).Trim()
        $gitStatusOutput = (& git -C $repo status --short 2>$null)
        if ($LASTEXITCODE -eq 0) {
            $gitStatus = if ($gitStatusOutput) { $gitStatusOutput -join "`n" } else { 'Clean working tree' }
        }
    }
} catch {
    $gitStatus = "Git metadata unavailable: $($_.Exception.Message)"
}
$products = @(Get-LisanInstalledProducts)
$msiHash = ($hashRows | Where-Object { $_.Path -eq $msiPath }).Hash
$validationLog = Join-Path $releaseDir "VALIDATION_LOG.md"

$stepLines = $steps | ForEach-Object {
    "- {0}: {1} ({2})" -f $_.Name, $_.Status, $_.Log
}
$productLines = $products | ForEach-Object {
    "- {0} {1} {2}" -f $_.ProductCode, $_.Name, $_.LocalPackage
}
if (-not $productLines) {
    $productLines = @("- None")
}

$validationLines = @(
    "# Lisan Studio $ReleaseLabel Validation Log",
    "",
    "Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz")",
    "Repository: $repo",
    "Branch: $gitBranch",
    "Commit: $gitCommit",
    "",
    "## Handoff Summary",
    "",
    "This evidence bundle is the automated private beta handoff record for the Lisan Studio MSI. It proves the package, MSI smoke, installed runtime smoke, checksum, and screenshot flow completed, but it does not claim that human manual QA is complete.",
    "",
    "## Isolation And Runner Boundary",
    "",
    "- GUI, MSI, installed-app, and release-evidence validation belongs in LisanStudio-QA through Homelab.",
    "- Do not use active WHITEDRAGON for GUI automation, MSI install/uninstall, installed-app validation, registry mutation, or destructive validation.",
    "- Homelab core stays generic; Lisan Studio owns this release evidence workflow in the project repo.",
    "",
    "## Automated Validation Completed",
    "",
    "## Gate Results",
    "",
    $stepLines,
    "",
    "## Release Artifacts",
    "",
    "- MSI: $msiPath",
    "- MSI SHA256: $msiHash",
    "- Checksums: $checksumsPath",
    "- Known issues: $knownIssuesPath",
    "- Screenshot: $screenshot",
    "",
    "## Manual QA Required",
    "",
    "Manual QA is not complete until the installed-app checklist is recorded.",
    "Generate the checklist from an existing release bundle with:",
    "",
    '```powershell',
    '.\scripts\beta-manual-check.ps1',
    '```',
    "",
    'The manual pass must cover Start Menu launch, Desktop shortcut launch, Arabic mixed-direction editing, project open, current `.apy` run output, RTL layout, settings persistence, uninstall, and reinstall.',
    "",
    "## Deferred Features",
    "",
    "- Git UI",
    "- AI panel",
    "- public distribution",
    "- auto-update",
    "- plugin system",
    "",
    "## Installed Product State",
    "",
    $productLines,
    "",
    "## Git Status At Evidence Time",
    "",
    '```text',
    $gitStatus,
    '```'
)
$validationLines | Set-Content -LiteralPath $validationLog -Encoding UTF8

[PSCustomObject]@{
    ReleaseDir = $releaseDir
    ValidationLog = $validationLog
    Checksums = $checksumsPath
    KnownIssues = $knownIssuesPath
    Screenshot = $screenshot
    Msi = $msiPath
    MsiSha256 = $msiHash
}
