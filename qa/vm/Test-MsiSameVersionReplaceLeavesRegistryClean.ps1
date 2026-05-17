[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$FirstMsiPath,

    [Parameter(Mandatory = $true)]
    [string]$ReplacementMsiPath,

    [string]$ExpectedProductVersion = "",
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA "LisanStudio"),
    [switch]$KeepInstalled,
    [switch]$AllowMutation,
    [switch]$IUnderstandThisRunsMsiUpgrade
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = "Stop"

& (Join-Path $PSScriptRoot "..\..\scripts\msi-upgrade-smoke.ps1") `
    -EarlierMsiPath $FirstMsiPath `
    -ReplacementMsiPath $ReplacementMsiPath `
    -ExpectedEarlierVersion $ExpectedProductVersion `
    -ExpectedReplacementVersion $ExpectedProductVersion `
    -InstallRoot $InstallRoot `
    -SameVersionReplace `
    -KeepInstalled:$KeepInstalled `
    -AllowMutation:$AllowMutation `
    -IUnderstandThisRunsMsiUpgrade:$IUnderstandThisRunsMsiUpgrade
