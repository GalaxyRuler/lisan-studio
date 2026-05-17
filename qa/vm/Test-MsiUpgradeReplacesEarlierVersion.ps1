[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EarlierMsiPath,

    [Parameter(Mandatory = $true)]
    [string]$ReplacementMsiPath,

    [string]$DowngradeMsiPath = "",
    [string]$ExpectedEarlierVersion = "",
    [string]$ExpectedReplacementVersion = "",
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA "LisanStudio"),
    [switch]$KeepInstalled,
    [switch]$AllowMutation,
    [switch]$IUnderstandThisRunsMsiUpgrade
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = "Stop"

& (Join-Path $PSScriptRoot "..\..\scripts\msi-upgrade-smoke.ps1") `
    -EarlierMsiPath $EarlierMsiPath `
    -ReplacementMsiPath $ReplacementMsiPath `
    -DowngradeMsiPath $DowngradeMsiPath `
    -ExpectedEarlierVersion $ExpectedEarlierVersion `
    -ExpectedReplacementVersion $ExpectedReplacementVersion `
    -InstallRoot $InstallRoot `
    -KeepInstalled:$KeepInstalled `
    -AllowMutation:$AllowMutation `
    -IUnderstandThisRunsMsiUpgrade:$IUnderstandThisRunsMsiUpgrade
