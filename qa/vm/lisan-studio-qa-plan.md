# Lisan Studio QA VM Plan

This is a project-owned QA lane for the active Lisan Studio native Qt / WiX /
MSI repository at `C:\Users\Admin\arabic-code-studio-qt`.

Homelab provides generic runner capability only. Project-specific MSI, GUI,
Arabic, RTL, screenshot, log, and release-validation details stay in this repo.

`LisanStudio-QA` is the intended QA VM for installed desktop validation. MSI
install/uninstall, GUI launch, Arabic/RTL smoke checks, logs, screenshots, and
release evidence should happen in that QA VM, not on the active WHITEDRAGON
desktop.

## Project script flow

The current project-owned validation flow is:

- `scripts\build.ps1`
- `scripts\package.ps1`
- `scripts\installed-smoke.ps1`
- `scripts\msi-smoke.ps1`
- `scripts\release-evidence.ps1`

`scripts\package.ps1` already invokes repo validation before staging and MSI
creation. Future isolated runtime execution should reuse these project scripts
instead of moving product-specific logic into Homelab core.

## Guest readiness provisioning

`qa\vm\Install-LisanStudioQaGuest.ps1` is the project-owned guest provisioning
entry point for the `LisanStudio-QA` lane.

The script requires a Homelab VM approval packet, confirms packet integrity
through Homelab, refuses stale packets, refuses active WHITEDRAGON targets, and
defaults to dry-run. With bounded runtime approval it connects to
`LisanStudio-QA` through PowerShell Direct, creates only approved
`C:\CodexRunner` guest work, artifact, log, and bootstrap directories, then
performs tooling-first guest provisioning for the exact repo prerequisites:
PowerShell 7, Git, Python 3.13, MSYS2, WiX CLI, and the UCRT64 GCC/CMake/Ninja
/ Qt package set. It records readiness and tooling reports as JSON and does not
perform unrelated system mutation.

## Real isolated MSI/GUI smoke

`qa\vm\Invoke-LisanStudioVmSmoke.ps1` is the project-owned MSI/GUI/Arabic smoke
entry point for the `LisanStudio-QA` lane. It supports real isolated execution
after the approval gate is crossed.

The scaffold requires a Homelab VM approval packet, confirms packet integrity
through Homelab, refuses stale packets, refuses active WHITEDRAGON targets, and
writes a dry-run smoke intent artifact when used in planning mode. In real
isolated execution it copies the active `arabic-code-studio-qt` workspace and
`C:\Users\Admin\apython` into `C:\CodexRunner\work`, requires a logged-on
`LISAN-QA\codexqa` desktop session, then registers a one-shot interactive
scheduled task for the GUI-sensitive work. That task runs
`scripts\validate.ps1` first and then `scripts\release-evidence.ps1` with the
guest-specific Bash, Python, WiX, and Qt license paths, while PowerShell Direct
continues to handle packet checks, staging, polling, and artifact copy-back.
That flow reuses `scripts\package.ps1`, `scripts\installed-smoke.ps1`, and
`scripts\msi-smoke.ps1`, plus the manual installed-app checks documented in
`docs\BETA_VALIDATION.md`. The interactive task writes raw stdout, raw stderr,
combined logs, and exit-code JSON for both `validate` and `release-evidence`
before the wrapper reports pass or fail.

## Planned artifact expectations

The LisanStudio-QA lane should eventually collect:

- install and uninstall logs
- launch and runtime smoke logs
- `validate` and `release-evidence` stdout, stderr, combined logs, and
  per-step result JSON
- Arabic and RTL screenshots
- release evidence summaries and checksums
- guest readiness and smoke summary JSON

No VM creation, deletion, snapshot/checkpoint, rollback, MSI install, GUI
automation, or desktop mutation should happen without explicit approval.
