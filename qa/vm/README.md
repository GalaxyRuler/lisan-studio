# VM QA Notes

Lisan Studio is a native Windows Qt desktop IDE with WiX MSI packaging, so
installed-app GUI and installer validation belong on an approved isolated
runner such as `LisanStudio-QA`, not on the active WHITEDRAGON desktop.

Default VM-oriented route:

```text
installedDesktopQa -> project-vm
```

Project-owned VM files in this repo:

- `qa\vm\profiles\lisanstudio-msi-smoke.json`
- `qa\vm\Install-LisanStudioQaGuest.ps1`
- `qa\vm\Invoke-LisanStudioVmSmoke.ps1`
- `qa\vm\lisan-studio-qa-plan.md`

Use the LisanStudio-QA lane for workflows such as:

- installing or uninstalling the MSI
- launching the installed app
- collecting logs, screenshots, and release evidence
- validating Arabic and RTL shell behavior
- checking shortcut/install payload behavior
- running modal and project-tree GUI checks that should not interrupt the host

Safety boundaries:

- active WHITEDRAGON is forbidden for GUI, MSI, and destructive validation
- `project-vm` is the required route for installed desktop QA
- project guest provisioning is bounded to `LisanStudio-QA`
- project-owned VM mutation must stay under `C:\CodexRunner` inside the guest
- approval packets must be confirmed before any provisioning or smoke action

Current implementation state:

- guest provisioning is implemented as a bounded, approval-gated PowerShell
  Direct workflow for `LisanStudio-QA`
- guest provisioning is tooling-first: it validates or installs the exact repo
  prerequisites needed by `scripts\build.ps1`, `scripts\package.ps1`, and
  `scripts\release-evidence.ps1`
- MSI/GUI smoke is implemented for real isolated execution in
  `LisanStudio-QA`; it copies the active repo plus `apython` into
  `C:\CodexRunner\work`, requires a logged-on `LISAN-QA\codexqa` desktop
  session, registers a one-shot interactive scheduled task, runs
  `scripts\validate.ps1` first and then `scripts\release-evidence.ps1`, and
  copies logs and evidence back to the host. The smoke task records raw
  stdout, raw stderr, combined logs, and exit-code JSON for both validation
  steps before reporting failure.

No VM creation, deletion, checkpoint, rollback, MSI install, GUI automation, or
desktop mutation should happen without explicit later approval.
