param(
    [Parameter()]
    [string]$ApythonRoot = "",
    [Parameter()]
    [string]$PythonRoot = "",
    [string]$DebugpySourceSitePackages = "",
    [string]$Configuration = "Release",
    [string]$ProductVersion = "0.1.0",
    [string]$BuildId = "",
    [string]$BashPath = "C:\msys64\usr\bin\bash.exe",
    [string]$WindeployQtPath = "C:\msys64\ucrt64\bin\windeployqt6.exe",
    [string]$WixPath = "C:\Program Files\WiX Toolset v7.0\bin\wix.exe",
    [string]$SignToolPath = "",
    [string]$SigningCertificateThumbprint = "",
    [string]$TimestampServer = "http://timestamp.digicert.com",
    [string]$QtLicenseRoot = "C:\msys64\ucrt64\share\licenses\qt6-base",
    [switch]$SkipMsi
)

$ErrorActionPreference = "Stop"

function Convert-ToMsysPath {
    param([string]$WindowsPath)

    $resolved = (Resolve-Path -LiteralPath $WindowsPath).Path
    $drive = $resolved.Substring(0, 1).ToLowerInvariant()
    $rest = $resolved.Substring(2).Replace('\', '/')
    return "/$drive$rest"
}

function Resolve-PackagingInputPath {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [string]$ExplicitPath,
        [string]$EnvironmentVariableName,
        [string]$FallbackPath
    )

    $candidate = $ExplicitPath
    if ([string]::IsNullOrWhiteSpace($candidate) -and -not [string]::IsNullOrWhiteSpace($EnvironmentVariableName)) {
        $candidate = [Environment]::GetEnvironmentVariable($EnvironmentVariableName)
    }
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        $candidate = $FallbackPath
    }
    if ([string]::IsNullOrWhiteSpace($candidate) -or -not (Test-Path -LiteralPath $candidate)) {
        throw "$Name not found. Pass -$Name or set $EnvironmentVariableName."
    }

    return (Resolve-Path -LiteralPath $candidate).Path
}

function Invoke-NativeToolAllowingStderr {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [string]$DisplayName = (Split-Path -Leaf $FilePath)
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = & $FilePath @Arguments 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($output) {
        $output | Write-Output
    }
    if ($exitCode -ne 0) {
        throw "$DisplayName exited with code $exitCode."
    }
}

function Copy-DirectoryContents {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    $sourceRoot = (Resolve-Path -LiteralPath $Source).Path.TrimEnd('\')
    $files = @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -Force -File |
        Where-Object {
            $_.FullName -notmatch '\\__pycache__\\' -and
            $_.Extension -notin @('.pyc', '.pyo')
        })
    foreach ($file in $files) {
        $relativePath = $file.FullName.Substring($sourceRoot.Length).TrimStart('\')
        $destinationPath = Join-Path $Destination $relativePath
        $destinationParent = Split-Path -Parent $destinationPath
        New-Item -ItemType Directory -Force -Path $destinationParent | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $destinationPath -Force
    }

    Write-Output "Copied $($files.Count) files from $Source to $Destination"
}

function Install-ApythonRuntimeOffline {
    param(
        [Parameter(Mandatory = $true)][string]$SourceRoot,
        [Parameter(Mandatory = $true)][string]$DestinationSitePackages
    )

    $eggInfo = Join-Path $SourceRoot "lughat_althuban.egg-info"
    $pkgInfo = Join-Path $eggInfo "PKG-INFO"
    if (-not (Test-Path -LiteralPath $pkgInfo)) {
        throw "Apython package metadata missing: $pkgInfo"
    }

    $requiredSourceFilesByPackage = @{
        arabicpython = @("__init__.py", "cli.py")
        arabicpython_kernel = @("__init__.py")
    }

    foreach ($packageName in @("arabicpython", "arabicpython_kernel")) {
        $sourcePackage = Join-Path $SourceRoot $packageName
        if (-not (Test-Path -LiteralPath $sourcePackage)) {
            throw "Apython package source missing: $sourcePackage"
        }

        foreach ($requiredSourceFile in $requiredSourceFilesByPackage[$packageName]) {
            $requiredSourcePath = Join-Path $sourcePackage $requiredSourceFile
            if (-not (Test-Path -LiteralPath $requiredSourcePath)) {
                throw "Apython package source file missing: $requiredSourcePath"
            }
        }

        $destinationPackage = Join-Path $DestinationSitePackages $packageName
        if (Test-Path -LiteralPath $destinationPackage) {
            Remove-Item -LiteralPath $destinationPackage -Recurse -Force
        }
        Copy-DirectoryContents -Source $sourcePackage -Destination $destinationPackage

        foreach ($requiredSourceFile in $requiredSourceFilesByPackage[$packageName]) {
            $requiredDestinationPath = Join-Path $destinationPackage $requiredSourceFile
            if (-not (Test-Path -LiteralPath $requiredDestinationPath)) {
                throw "Apython package destination file missing after copy: $requiredDestinationPath"
            }
        }
    }

    $metadata = Get-Content -Raw -LiteralPath $pkgInfo
    if ($metadata -notmatch '(?m)^Version:\s*(?<Version>[^\r\n]+)') {
        throw "Apython package metadata does not declare a Version: $pkgInfo"
    }

    $version = $Matches.Version.Trim()
    $distInfo = Join-Path $DestinationSitePackages "lughat_althuban-$version.dist-info"
    if (Test-Path -LiteralPath $distInfo) {
        Remove-Item -LiteralPath $distInfo -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $distInfo | Out-Null

    Set-Content -LiteralPath (Join-Path $distInfo "METADATA") -Value $metadata -Encoding UTF8
    foreach ($metadataFile in @("entry_points.txt", "top_level.txt")) {
        $sourceMetadataFile = Join-Path $eggInfo $metadataFile
        if (Test-Path -LiteralPath $sourceMetadataFile) {
            Copy-Item -LiteralPath $sourceMetadataFile -Destination (Join-Path $distInfo $metadataFile) -Force
        }
    }
    @"
Wheel-Version: 1.0
Generator: LisanStudio package.ps1 offline runtime staging
Root-Is-Purelib: true
Tag: py3-none-any
"@ | Set-Content -LiteralPath (Join-Path $distInfo "WHEEL") -Encoding ASCII
    "package.ps1" | Set-Content -LiteralPath (Join-Path $distInfo "INSTALLER") -Encoding ASCII
    "" | Set-Content -LiteralPath (Join-Path $distInfo "RECORD") -Encoding ASCII
}

function Install-DebugpyRuntimeOffline {
    param(
        [Parameter(Mandatory = $true)][string]$SourceSitePackages,
        [Parameter(Mandatory = $true)][string]$DestinationSitePackages
    )

    $sourceDebugpy = Join-Path $SourceSitePackages "debugpy"
    if (-not (Test-Path -LiteralPath $sourceDebugpy)) {
        throw "debugpy package source missing: $sourceDebugpy. Install debugpy into PythonRoot or pass -DebugpySourceSitePackages."
    }

    $sourceDistInfo = @(Get-ChildItem -LiteralPath $SourceSitePackages -Directory -Filter "debugpy-*.dist-info" -ErrorAction SilentlyContinue | Select-Object -First 1)
    if ($sourceDistInfo.Count -eq 0) {
        throw "debugpy dist-info source missing under: $SourceSitePackages"
    }

    foreach ($destinationName in @("debugpy") + @($sourceDistInfo[0].Name)) {
        $destinationPath = Join-Path $DestinationSitePackages $destinationName
        if (Test-Path -LiteralPath $destinationPath) {
            Remove-Item -LiteralPath $destinationPath -Recurse -Force
        }
    }

    Copy-DirectoryContents -Source $sourceDebugpy -Destination (Join-Path $DestinationSitePackages "debugpy")
    Copy-DirectoryContents -Source $sourceDistInfo[0].FullName -Destination (Join-Path $DestinationSitePackages $sourceDistInfo[0].Name)

    $adapterEntrypoint = Join-Path $DestinationSitePackages "debugpy\adapter\__main__.py"
    if (-not (Test-Path -LiteralPath $adapterEntrypoint)) {
        throw "debugpy adapter entrypoint missing after copy: $adapterEntrypoint"
    }
}

function Assert-StagedApythonRuntime {
    param(
        [Parameter(Mandatory = $true)][string]$PythonExe,
        [Parameter(Mandatory = $true)][string]$SitePackages
    )

    foreach ($requiredFile in @(
            (Join-Path $SitePackages "arabicpython\__init__.py"),
            (Join-Path $SitePackages "arabicpython\cli.py"),
            (Join-Path $SitePackages "arabicpython_kernel\__init__.py"),
            (Join-Path $SitePackages "debugpy\adapter\__main__.py")
        )) {
        if (-not (Test-Path -LiteralPath $requiredFile)) {
            throw "Staged Apython runtime file missing: $requiredFile"
        }
    }

    $runtimeCheck = @"
import importlib.metadata as md
import importlib.util
import pathlib

site = pathlib.Path(r'''$SitePackages''').resolve()
for package in ('arabicpython', 'arabicpython_kernel', 'debugpy'):
    spec = importlib.util.find_spec(package)
    if spec is None or spec.origin is None:
        raise SystemExit(f'{package} import spec missing')
    origin = pathlib.Path(spec.origin).resolve()
    if site != origin and site not in origin.parents:
        raise SystemExit(f'{package} imported from outside staged runtime: {origin}')
    print(f'{package} {origin}')
if importlib.util.find_spec('debugpy.adapter') is None:
    raise SystemExit('debugpy.adapter import spec missing')
print('lughat-althuban', md.version('lughat-althuban'))
print('debugpy', md.version('debugpy'))
"@

    & $PythonExe -I -c $runtimeCheck
}

function Copy-RequiredNativeRuntimeDlls {
    param(
        [Parameter(Mandatory = $true)][string]$BashPath,
        [Parameter(Mandatory = $true)][string]$TargetExecutable,
        [Parameter(Mandatory = $true)][string]$SourceBinDirectory,
        [Parameter(Mandatory = $true)][string]$DestinationDirectory
    )

    $targetExecutableUnix = Convert-ToMsysPath -WindowsPath $TargetExecutable
    $lddOutput = & $BashPath -lc "export PATH=/ucrt64/bin:/usr/bin:`$PATH; ldd '$targetExecutableUnix'"
    if ($LASTEXITCODE -ne 0) {
        throw "ldd failed for native runtime dependency scan: $TargetExecutable"
    }

    $dllNames = New-Object System.Collections.Generic.HashSet[string] ([StringComparer]::OrdinalIgnoreCase)
    foreach ($line in $lddOutput) {
        if ($line -match '=>\s+/ucrt64/bin/(?<Name>[^ \t]+\.dll)') {
            [void]$dllNames.Add($Matches.Name)
        }
    }

    foreach ($requiredDllName in @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")) {
        [void]$dllNames.Add($requiredDllName)
    }

    foreach ($dllName in ($dllNames | Sort-Object)) {
        $sourceDll = Join-Path $SourceBinDirectory $dllName
        if (-not (Test-Path -LiteralPath $sourceDll)) {
            throw "Required native runtime DLL missing: $sourceDll"
        }
        Copy-Item -LiteralPath $sourceDll -Destination (Join-Path $DestinationDirectory $dllName) -Force
    }
}

function Write-SigningStatus {
    param(
        [Parameter(Mandatory = $true)][string]$MsiPath,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $signature = Get-AuthenticodeSignature -LiteralPath $MsiPath
    $authenticodeStatus = [string]$signature.Status
    $signer = if ($signature.SignerCertificate) {
        $signature.SignerCertificate.Subject
    } else {
        'None'
    }

    @(
        "MSI: $MsiPath",
        "AuthenticodeStatus: $authenticodeStatus",
        "Signer: $signer",
        "Policy: Public distribution requires a valid Authenticode signature.",
        "PrivateBetaPolicy: unsigned or unverifiable packages are allowed only for private QA handoff evidence."
    ) | Set-Content -LiteralPath $OutputPath -Encoding UTF8
}

function Resolve-SignToolPath {
    param([string]$ExplicitPath)

    if (-not [string]::IsNullOrWhiteSpace($ExplicitPath)) {
        if (-not (Test-Path -LiteralPath $ExplicitPath)) {
            throw "signtool.exe not found: $ExplicitPath"
        }
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }

    $pathCandidate = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($pathCandidate) {
        return $pathCandidate.Source
    }

    $kitRoots = @(
        (Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"),
        (Join-Path $env:ProgramFiles "Windows Kits\10\bin")
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) -and (Test-Path -LiteralPath $_) }

    foreach ($kitRoot in $kitRoots) {
        $candidate = Get-ChildItem -LiteralPath $kitRoot -Recurse -Filter signtool.exe -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\x64\\signtool\.exe$' } |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($candidate) {
            return $candidate.FullName
        }
    }

    throw "signtool.exe not found. Install the Windows SDK or pass -SignToolPath."
}

function Invoke-MsiAuthenticodeSigning {
    param(
        [Parameter(Mandatory = $true)][string]$MsiPath,
        [Parameter(Mandatory = $true)][string]$CertificateThumbprint,
        [string]$SignToolPath,
        [string]$TimestampServer
    )

    $normalizedThumbprint = $CertificateThumbprint -replace '\s', ''
    if ([string]::IsNullOrWhiteSpace($normalizedThumbprint)) {
        throw "Signing certificate thumbprint is empty."
    }

    $resolvedSignToolPath = Resolve-SignToolPath -ExplicitPath $SignToolPath
    $arguments = @(
        'sign',
        '/fd', 'SHA256',
        '/sha1', $normalizedThumbprint,
        '/tr', $TimestampServer,
        '/td', 'SHA256',
        $MsiPath
    )
    Invoke-NativeToolAllowingStderr -FilePath $resolvedSignToolPath -Arguments $arguments -DisplayName 'signtool.exe'
}

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$repoParent = Split-Path -Parent $repo
$stage = Join-Path $repo "stage"
$artifacts = Join-Path $repo "artifacts"
$build = Join-Path $repo "build"
$bash = $BashPath
$windeployqt = $WindeployQtPath
$nativeRuntimeBin = Split-Path -Parent $windeployqt
$wix = $WixPath
$wxs = Join-Path $repo "packaging\wix\LisanStudio.wxs"
$msiFileName = "LisanStudio-$ProductVersion-beta.msi"
$msiPath = Join-Path $artifacts $msiFileName
$repoUnix = Convert-ToMsysPath -WindowsPath $repo

if ([string]::IsNullOrWhiteSpace($SigningCertificateThumbprint) -and -not [string]::IsNullOrWhiteSpace($env:LISAN_SIGNING_CERT_THUMBPRINT)) {
    $SigningCertificateThumbprint = $env:LISAN_SIGNING_CERT_THUMBPRINT
}
if ([string]::IsNullOrWhiteSpace($SignToolPath) -and -not [string]::IsNullOrWhiteSpace($env:LISAN_SIGNTOOL_PATH)) {
    $SignToolPath = $env:LISAN_SIGNTOOL_PATH
}
if (-not [string]::IsNullOrWhiteSpace($env:LISAN_TIMESTAMP_URL)) {
    $TimestampServer = $env:LISAN_TIMESTAMP_URL
}

$ApythonRoot = Resolve-PackagingInputPath `
    -Name "ApythonRoot" `
    -ExplicitPath $ApythonRoot `
    -EnvironmentVariableName "LISAN_APYTHON_ROOT" `
    -FallbackPath (Join-Path $repoParent "lughat-althuban")
$PythonRoot = Resolve-PackagingInputPath `
    -Name "PythonRoot" `
    -ExplicitPath $PythonRoot `
    -EnvironmentVariableName "LISAN_PYTHON_ROOT" `
    -FallbackPath (Join-Path $env:LOCALAPPDATA "Programs\Python\Python313")

if ([string]::IsNullOrWhiteSpace($BuildId)) {
    $gitBuildId = ""
    try {
        $gitOutput = @(git -C $repo rev-parse --short=12 HEAD 2>$null)
        if ($LASTEXITCODE -eq 0 -and $gitOutput.Count -gt 0) {
            $gitBuildId = [string]$gitOutput[0]
        }
    } catch {
        $gitBuildId = ""
    }

    $BuildId = if ([string]::IsNullOrWhiteSpace($gitBuildId)) {
        (Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssZ")
    } else {
        $gitBuildId.Trim()
    }
}

if (-not (Test-Path $bash)) { throw "MSYS2 bash not found: $bash" }
if (-not (Test-Path $windeployqt)) { throw "windeployqt6 not found: $windeployqt" }
if (-not (Test-Path $qtLicenseRoot)) { throw "Qt license folder not found: $qtLicenseRoot" }

& (Join-Path $PSScriptRoot "validate.ps1") -BashPath $bash

if (Test-Path $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
if (Test-Path $artifacts) {
    New-Item -ItemType Directory -Force -Path $artifacts | Out-Null
} else {
    New-Item -ItemType Directory -Force -Path $artifacts | Out-Null
}
New-Item -ItemType Directory -Force -Path $stage | Out-Null

& $bash -lc @"
set -euo pipefail
export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH
cd "$repoUnix"
cmake --install build --prefix stage
"@

Invoke-NativeToolAllowingStderr -FilePath $windeployqt -Arguments @(
    '--release',
    '--no-translations',
    (Join-Path $stage "LisanStudio.exe")
) -DisplayName 'windeployqt6.exe'
Copy-RequiredNativeRuntimeDlls -BashPath $bash -TargetExecutable (Join-Path $stage "LisanStudio.exe") -SourceBinDirectory $nativeRuntimeBin -DestinationDirectory $stage

$runtimeRoot = Join-Path $stage "runtime\python"
Copy-Item -Path $PythonRoot -Destination $runtimeRoot -Recurse -Force
$python = Join-Path $runtimeRoot "python.exe"
if (-not (Test-Path $python)) {
    throw "Staged Python executable missing: $python"
}

$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"
$env:PYTHONNOUSERSITE = "1"

$sitePackages = Join-Path $runtimeRoot "Lib\site-packages"
if (Test-Path $sitePackages) {
    Get-ChildItem -LiteralPath $sitePackages -Force |
        Where-Object {
            $_.Name -notmatch '^pip(-|$)' -and
            $_.Name -ne "distutils-precedence.pth"
        } |
        Remove-Item -Recurse -Force
}

Install-ApythonRuntimeOffline -SourceRoot $ApythonRoot -DestinationSitePackages $sitePackages
$resolvedDebugpySourceSitePackages = if ([string]::IsNullOrWhiteSpace($DebugpySourceSitePackages)) {
    Join-Path $PythonRoot "Lib\site-packages"
} else {
    $DebugpySourceSitePackages
}
Install-DebugpyRuntimeOffline -SourceSitePackages $resolvedDebugpySourceSitePackages -DestinationSitePackages $sitePackages

$editableMarkers = Get-ChildItem -LiteralPath $sitePackages -Force -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -like "__editable__*" -or $_.Name -like "apython-*.dist-info" }
if ($editableMarkers) {
    throw "Staged runtime still contains editable/local apython markers."
}

Assert-StagedApythonRuntime -PythonExe $python -SitePackages $sitePackages

Copy-Item -LiteralPath (Join-Path $repo "README.md") -Destination (Join-Path $stage "README.md") -Force
Copy-Item -LiteralPath (Join-Path $repo "docs\RELEASE_NOTES.md") -Destination (Join-Path $stage "RELEASE_NOTES.md") -Force
Copy-Item -LiteralPath (Join-Path $repo "docs\BETA_VALIDATION.md") -Destination (Join-Path $stage "BETA_VALIDATION.md") -Force
Copy-Item -LiteralPath (Join-Path $repo "licenses\LICENSES.md") -Destination (Join-Path $stage "LICENSES.md") -Force
Copy-Item -LiteralPath (Join-Path $repo "samples") -Destination (Join-Path $stage "samples") -Recurse -Force

$stageLicenses = Join-Path $stage "licenses"
New-Item -ItemType Directory -Force -Path $stageLicenses | Out-Null
Copy-Item -LiteralPath $qtLicenseRoot -Destination (Join-Path $stageLicenses "qt6-base") -Recurse -Force
Copy-Item -LiteralPath (Join-Path $PythonRoot "LICENSE.txt") -Destination (Join-Path $stageLicenses "Python-LICENSE.txt") -Force
Copy-Item -LiteralPath (Join-Path $ApythonRoot "LICENSE") -Destination (Join-Path $stageLicenses "lughat-althuban-LICENSE") -Force

if (-not $SkipMsi) {
    if (-not (Test-Path $wix)) {
        throw "WiX wix.exe not found: $wix"
    }
    & $wix build `
        -acceptEula wix7 `
        $wxs `
        -define "ProductVersion=$ProductVersion" `
        -define "BuildId=$BuildId" `
        -define "SourceDir=$stage" `
        -out $msiPath
    if (-not [string]::IsNullOrWhiteSpace($SigningCertificateThumbprint)) {
        Invoke-MsiAuthenticodeSigning `
            -MsiPath $msiPath `
            -CertificateThumbprint $SigningCertificateThumbprint `
            -SignToolPath $SignToolPath `
            -TimestampServer $TimestampServer
    }
    Write-SigningStatus -MsiPath $msiPath -OutputPath (Join-Path $artifacts "SIGNING_STATUS.txt")
}

Get-ChildItem $artifacts -File -ErrorAction SilentlyContinue | Get-FileHash -Algorithm SHA256 |
    Format-Table -AutoSize
