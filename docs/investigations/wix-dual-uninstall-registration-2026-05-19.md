# WiX Dual Uninstall Registration Investigation - 2026-05-19

## 1. Current Behaviour

The failed Homelab run was `prompt13-upgrade-20260519T170322`:

```text
C:\Users\Admin\Documents\Codex\Homelab\codex-isolated-test-runners\tools\codex-runner\artifacts\prompt13-upgrade-20260519T170322
```

The package currently declares a per-user WiX package, allows same-version major upgrades, and includes a manually-authored uninstall registry component in the main feature:

```xml
<!-- packaging/wix/LisanStudio.wxs:3-11 -->
<Package
  Name="Lisan Studio"
  Manufacturer="Lisan Studio"
  Version="$(var.ProductVersion)"
  UpgradeCode="b45833fc-87f8-4656-8cc4-dc87769ea386"
  Scope="perUser">
  <MajorUpgrade
    AllowSameVersionUpgrades="yes"
    DowngradeErrorMessage="A newer Lisan Studio is already installed." />
```

```xml
<!-- packaging/wix/LisanStudio.wxs:14-19 -->
<Feature Id="MainFeature" Title="Lisan Studio" Level="1">
  <ComponentGroupRef Id="ApplicationFiles" />
  <ComponentRef Id="StartMenuShortcut" />
  <ComponentRef Id="DesktopShortcut" />
  <ComponentRef Id="UninstallRegistryEntry" />
</Feature>
```

```xml
<!-- packaging/wix/LisanStudio.wxs:25-78 -->
<DirectoryRef Id="INSTALLFOLDER">
  <Component Id="UninstallRegistryEntry" Guid="1f87785b-1f4a-4a77-b1ec-386f92be8c29">
    <RegistryValue Root="HKCU"
                   Key="Software\Microsoft\Windows\CurrentVersion\Uninstall\LisanStudio"
                   Name="DisplayName"
                   Type="string"
                   Value="Lisan Studio"
                   KeyPath="yes" />
    ...
    <RegistryValue Root="HKCU"
                   Key="Software\Microsoft\Windows\CurrentVersion\Uninstall\LisanStudio"
                   Name="UninstallString"
                   Type="string"
                   Value="msiexec.exe /x [ProductCode]" />
    <RegistryValue Root="HKCU"
                   Key="Software\Microsoft\Windows\CurrentVersion\Uninstall\LisanStudio"
                   Name="QuietUninstallString"
                   Type="string"
                   Value="msiexec.exe /x [ProductCode] /qn /norestart" />
    ...
  </Component>
</DirectoryRef>
```

The same-version-replace wrapper calls the shared upgrade smoke script with the same MSI version on both sides:

```powershell
# qa/vm/Test-MsiSameVersionReplaceLeavesRegistryClean.ps1:19-28
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
```

The assertion expects exactly one uninstall entry and expects that entry to be the stable `...\Uninstall\LisanStudio` key:

```powershell
# scripts/msi-upgrade-smoke.ps1:126-140
function Assert-SingleLisanUninstallRegistryEntry {
    ...
    $entries = @(Get-LisanUninstallRegistryEntries)
    if ($entries.Count -ne 1) {
        throw "$Context should leave exactly one Windows Apps uninstall entry for Lisan Studio; found $($entries.Count): $($entries.RegistryPath -join ', ')"
    }

    $entry = $entries[0]
    if ($entry.RegistryPath -notlike '*\Uninstall\LisanStudio') {
        throw "$Context left an unexpected uninstall registry path: $($entry.RegistryPath)"
    }
```

The failed run summary confirms the same-version scenario failed before the replacement install. The remoted PowerShell error text was truncated in JSON, but it points at `scripts\msi-upgrade-smoke.ps1`:

```json
// prompt13-upgrade-20260519T170322/.../upgrade-summary.json:10-18
{
  "name": "same-version-replace",
  "ok": false,
  "scriptPath": "C:\\CodexRunner\\work\\artifacts-prompt13-smoke-20260519T165202\\arabic-code-studio-qt\\qa\\vm\\Test-MsiSameVersionReplaceLeavesRegistryClean.ps1",
  "error": "C:\\CodexRunner\\work\\artifacts-prompt13-smoke-20260519T165202\\arabic-code-studio-qt\\scripts\\msi-upgrade-smoke.ps1 : The "
}
```

The assertion text at the pointed script location is:

```powershell
# scripts/msi-upgrade-smoke.ps1:132-135
$entries = @(Get-LisanUninstallRegistryEntries)
if ($entries.Count -ne 1) {
    throw "$Context should leave exactly one Windows Apps uninstall entry for Lisan Studio; found $($entries.Count): $($entries.RegistryPath -join ', ')"
}
```

The copied `install-earlier.log` shows two distinct registration paths during the single earlier-MSI install:

```text
# prompt13-upgrade-20260519T170322/.../install-earlier.log
 7114: MSI ... Doing action: RegisterUser
 7118: MSI ... Doing action: RegisterProduct
 7123: Action start 17:04:16: RegisterProduct.
 7127: Action ended 17:04:16: RegisterProduct. Return value 1.
 7145: MSI ... ProductInfo(ProductKey={4ACF9205-4C6B-442E-B035-7C213FBE78E0},ProductName=Lisan Studio,...,Assignment=0,...,ProductDeploymentFlags=2)
 7157: MSI ... ComponentRegister(ComponentId={1F87785B-1F4A-4A77-B1EC-386F92BE8C29},KeyPath=01:\Software\Microsoft\Windows\CurrentVersion\Uninstall\LisanStudio\DisplayName,...)
29954: MSI ... RegOpenKey(Root=-2147483647,Key=Software\Microsoft\Windows\CurrentVersion\Uninstall\LisanStudio,...)
29955: MSI ... RegAddValue(Name=DisplayName,Value=Lisan Studio,)
29956: MSI ... RegAddValue(Name=DisplayVersion,Value=0.1.0,)
29960: MSI ... RegAddValue(Name=UninstallString,Value=msiexec.exe /x {4ACF9205-4C6B-442E-B035-7C213FBE78E0},)
29961: MSI ... RegAddValue(Name=QuietUninstallString,Value=msiexec.exe /x {4ACF9205-4C6B-442E-B035-7C213FBE78E0} /qn /norestart,)
29967: MSI ... ActionStart(Name=RegisterProduct,Description=Registering product [1],)
29969: MSI ... DatabaseCopy(DatabasePath=C:\Windows\Installer\1dc6bd00.msi,ProductCode={4ACF9205-4C6B-442E-B035-7C213FBE78E0},,,)
29973: MSI ... ProductRegister(UpgradeCode={B45833FC-87F8-4656-8CC4-DC87769EA386},VersionString=0.1.0,...,InstallSource=C:\CodexRunner\work\prompt13-upgrade-20260519T170322\earlier\,Publisher=Lisan Studio,...)
29994: MSI ... UpgradeCodePublish(UpgradeCode={B45833FC-87F8-4656-8CC4-DC87769EA386})
30552: MSI ... Product: Lisan Studio -- Installation completed successfully.
30554: MSI ... Windows Installer installed the product. Product Name: Lisan Studio. Product Version: 0.1.0. ... Installation success or error status: 0.
```

After that install, the QA VM had:

- `HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\LisanStudio`, with `UninstallString = msiexec.exe /x {4ACF9205-4C6B-442E-B035-7C213FBE78E0}`.
- `HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{4ACF9205-4C6B-442E-B035-7C213FBE78E0}`, with Windows Installer's default `MsiExec.exe /I{...}` registration.

The earlier MSI used by this failed scenario has these MSI properties:

```text
ProductName: Lisan Studio
ProductVersion: 0.1.0
ProductCode: {4ACF9205-4C6B-442E-B035-7C213FBE78E0}
UpgradeCode: {B45833FC-87F8-4656-8CC4-DC87769EA386}
ALLUSERS: <missing>
MSIINSTALLPERUSER: <missing>
ARPSYSTEMCOMPONENT: <missing>
ARPNOMODIFY: <missing>
ARPNOREPAIR: <missing>
```

## 2. Root-Cause Hypothesis

The dual entry is probably not a random WiX 4 failure. It is the combination of:

1. Our explicit `UninstallRegistryEntry` component writing a stable HKCU Apps/Uninstall key.
2. Windows Installer's normal `RegisterProduct` action writing product registration metadata for the MSI product code.

Microsoft documents that `RegisterProduct` "registers the product information with the installer and with Add/Remove Programs" and stores the MSI database locally: https://learn.microsoft.com/en-us/windows/win32/msi/registerproduct-action. That matches `install-earlier.log:29967-29973`.

Microsoft also documents the automatic uninstall-key values as Windows Installer properties stored under the Uninstall registry key, keyed by product-code GUID: https://learn.microsoft.com/en-us/windows/win32/msi/uninstall-registry-key. That aligns with the observed `{4ACF9205-...}` entry.

The confusing part is scope. WiX says `Package/@Scope="perUser"` declares a per-user install and sets limited install privileges: https://docs.firegiant.com/wix/schema/wxs/packagescopetype/. Microsoft says `ALLUSERS` controls installation context and that unset `ALLUSERS` uses per-user context: https://learn.microsoft.com/en-us/windows/win32/msi/allusers. The inspected MSI has `ALLUSERS` and `MSIINSTALLPERUSER` missing, which is consistent with the per-user default path. Microsoft also says per-user installs appear in Add/Remove Programs only for the current user: https://learn.microsoft.com/en-us/windows/win32/msi/configuring-add-remove-programs-with-windows-installer.

However, Microsoft separately documents the product-code uninstall registry key under `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall`, and the failed run observed the 32-bit view of that key under `HKLM\Software\WOW6432Node\...`. The log does not spell the hive name for `ProductRegister`; it records the product registration operation, and the post-failure registry read showed where Windows materialized it.

My current hypothesis:

- `Scope="perUser"` is affecting install context, folders, shortcuts, and authored HKCU registry rows.
- It is not preventing Windows Installer from creating its own product-code registration under the machine uninstall hive on this 64-bit QA VM for this 32-bit MSI package.
- The stable HKCU `...\Uninstall\LisanStudio` component is redundant with Windows Installer's own ARP/product registration if all we need is a single Windows Apps entry.
- It is not equivalent to the automatic Windows Installer entry: our key is stable and under HKCU; Windows Installer's key is product-code based and currently appears under HKLM WOW6432Node.

What is not proven without a probe:

- Whether adding `ARPSYSTEMCOMPONENT=1` would merely hide the automatic MSI entry from Apps/Control Panel, or also materially reduce the raw HKLM/WOW6432Node values our tests currently enumerate.
- Whether removing `UninstallRegistryEntry` would leave a clean, user-visible Apps entry in the exact UX we want across Windows versions and user contexts.

No MSI rebuild or probe was run in this investigation slice.

## 3. Candidate Fix Approaches

### A. Remove `UninstallRegistryEntry`; rely on Windows Installer registration

Pros:

- Removes the self-authored duplicate source at `packaging/wix/LisanStudio.wxs:25-78`.
- Aligns with Windows Installer's built-in `RegisterProduct` behavior.
- Reduces custom uninstall metadata that can drift from MSI properties.
- Likely makes the upgrade test's "exactly one entry" invariant true if the test accepts the product-code key.

Cons:

- The remaining entry is probably `HKLM\WOW6432Node\...\{ProductCode}`, not the stable HKCU `...\LisanStudio` key the current test expects.
- ProductCode changes on each build because `Package/@ProductCode` is not fixed; operational scripts must discover by `DisplayName`/UpgradeCode, not by a stable key.
- If the product requirement is "no raw HKLM uninstall key for a per-user app", this does not satisfy it.

Blast radius:

- New installs would stop writing the stable HKCU key.
- Existing installs with both entries would need normal MSI upgrade/uninstall verification; removing a component can be safe only if component rules and remove behavior are checked.
- Tests in `scripts/msi-upgrade-smoke.ps1:126-140` must be updated to accept the Windows Installer product-code key, or the fix will still fail.

Would it have prevented the May 11 WHITEDRAGON contamination?

- No. The May 11 contamination was the Windows Installer product-code entry `{E3F2CD4B-...}` under HKLM/WOW6432Node, which this option continues to rely on.

### B. Keep explicit HKCU key; set/honor `ARPSYSTEMCOMPONENT`

Pros:

- Preserves the stable HKCU `...\Uninstall\LisanStudio` entry already expected by tests.
- Microsoft documents `ARPSYSTEMCOMPONENT` as preventing display in Add/Remove Programs when set to `1`: https://learn.microsoft.com/en-us/windows/win32/msi/arpsystemcomponent.
- Could make the automatic Windows Installer product-code entry hidden from Apps/Control Panel while our HKCU key remains the visible entry.

Cons:

- `ARPSYSTEMCOMPONENT` is documented as display behavior, not as "do not create product registration".
- Raw registry scans may still find a product-code key, likely with `SystemComponent=1`; our current test would need to decide whether hidden MSI registration counts as a violation.
- If Windows Settings ignores the manually-authored HKCU key or treats it differently from MSI ARP metadata, UX may regress.
- This option needs a controlled VM probe before implementation.

Blast radius:

- Existing visible product-code entries may become hidden only after upgrade/repair applies the property; this needs verification.
- Support/uninstall docs may need to point to the stable HKCU uninstall command if Windows Installer's entry is hidden.

Would it have prevented the May 11 WHITEDRAGON contamination?

- It likely would have prevented a visible Apps/Control Panel entry, but it is not proven that it would prevent the raw HKLM/WOW6432Node key from existing. A probe is required.

### C. Change `Scope` to `perMachine`

Pros:

- Makes the machine-level product-code uninstall registration expected rather than surprising.
- Better matches the current observed product-code entry location.
- Simpler operational model for machine-wide installed desktop QA.

Cons:

- Changes product requirements: install becomes per-machine and admin-gated.
- Contradicts the current `LocalAppDataFolder` per-user install layout in `packaging/wix/LisanStudio.wxs:21-23`.
- If the explicit HKCU component remains, dual registration can still happen: `Root="HKCU"` at `packaging/wix/LisanStudio.wxs:27` is explicit.
- Does not fix the stable HKCU/product-code duplication by itself.

Blast radius:

- Existing per-user installs need a migration/uninstall story.
- QA scripts, docs, shortcuts, and user support assumptions would shift from current-user install to all-users/admin install.

Would it have prevented the May 11 WHITEDRAGON contamination?

- No. It would make that HKLM product-code registration the intended behavior.

### D. Leave dual registration and relax the upgrade test

Pros:

- No installer behavior change.
- Lowest immediate code churn.
- A relaxed test can explicitly verify that the two entries point to the same ProductCode and do not represent two installed payloads.

Cons:

- Leaves two Apps/Uninstall records for one install, which is the user-facing defect under investigation.
- Makes future installer evidence noisier.
- Does not address the WHITEDRAGON contamination class.
- Risks normalizing a packaging smell that may confuse uninstall, repair, and upgrade support.

Blast radius:

- Test-only change, but it codifies the current dual state as acceptable.

Would it have prevented the May 11 WHITEDRAGON contamination?

- No.

## 4. Recommendation

I would not relax the test as the fix. The test caught a real packaging ambiguity.

My recommended next fix slice is Option A if the product accepts Windows Installer's product-code uninstall registration as the source of truth. That removes the self-authored duplicate and aligns with MSI's built-in `RegisterProduct`/ARP path. The test should then assert exactly one Lisan Studio uninstall entry, but it should not require the stable `...\Uninstall\LisanStudio` path.

If the product requirement is instead "per-user install must expose only a stable HKCU `LisanStudio` Apps entry and no visible product-code entry", then Option B is the more likely direction, but I would not implement it without a small approved VM probe:

1. Build the same MSI with `ARPSYSTEMCOMPONENT=1`.
2. Install it in `LisanStudio-QA`.
3. Inspect both Apps visibility and raw HKCU/HKLM uninstall hives.
4. Confirm whether the automatic product-code key disappears, is hidden only, or remains visible.

Option C is a product-policy change, not a cleanup fix. It should be chosen only if Lisan Studio deliberately becomes an admin-installed per-machine app.

Open uncertainty: the current evidence proves our explicit HKCU component and Windows Installer product registration both happen in one install. It does not prove whether `ARPSYSTEMCOMPONENT` is enough to keep a stable HKCU entry while suppressing the automatic product-code entry in every place the QA tests care about.
