# Homelab Runner Metadata

This directory documents how the active Lisan Studio native Qt / WiX / MSI repo
describes itself to the central Codex Homelab isolated runner system.

The active installable desktop app repo is:

```text
C:\Users\Admin\arabic-code-studio-qt
```

Do not treat `C:\Users\Admin\VS Code Arabic` as the active project-owned source
of truth for installer, MSI, Arabic/RTL GUI smoke, screenshot, log, or release
evidence workflows.

Homelab core owns generic runner infrastructure only: discovery, registration,
route resolution, VM planning, approval gates, and artifact plumbing. Lisan
Studio owns its build, package, installed-smoke, MSI-smoke, release-evidence,
Arabic/RTL QA, screenshot, log, and project profile details in this repo.

Project runner config:

```text
.codex/homelab-runner.json
```

Project-owned VM artifacts:

```text
qa\vm\
qa\vm\profiles\
```

Project-owned container/headless artifacts:

```text
qa\homelab\lisanstudio-metadata-checks.json
```

Selected runner classes:

- `container`: safe CLI validation such as metadata parsing and repo-local checks.
- `headless`: non-interactive inspection when GUI is unnecessary.
- `windows-gui`: remote desktop GUI validation outside the active WHITEDRAGON desktop.
- `desktop-msi`: installed-app and MSI validation routed to `project-vm`.
- `vm-gui`: LisanStudio-QA GUI validation for the installed app.

The project intentionally does not enable `vm-mutating` in route metadata.
Mutation is approval-gated through the Homelab VM plan and packet lane.

The `metadataChecks` profile is intentionally dry-run-only for live container
execution. This repo's real build and packaging flow currently depends on the
Windows/MSYS2/Qt/WiX toolchain, so a project-owned container image must be
chosen or built before Homelab runs live container commands for this repo.

Safe metadata-only checks from this repo root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "C:\Users\Admin\Documents\Codex\Homelab\codex-isolated-test-runners\tools\codex-runner\Resolve-CodexRunnerRoute.ps1" -ProjectPath "C:\Users\Admin\arabic-code-studio-qt" -Class container -Json
powershell -NoProfile -ExecutionPolicy Bypass -File "C:\Users\Admin\Documents\Codex\Homelab\codex-isolated-test-runners\tools\codex-runner\Resolve-CodexRunnerRoute.ps1" -ProjectPath "C:\Users\Admin\arabic-code-studio-qt" -Class headless -Json
powershell -NoProfile -ExecutionPolicy Bypass -File "C:\Users\Admin\Documents\Codex\Homelab\codex-isolated-test-runners\tools\codex-runner\Resolve-CodexRunnerRoute.ps1" -ProjectPath "C:\Users\Admin\arabic-code-studio-qt" -Class windows-gui -Json
powershell -NoProfile -ExecutionPolicy Bypass -File "C:\Users\Admin\Documents\Codex\Homelab\codex-isolated-test-runners\tools\codex-runner\Resolve-CodexRunnerRoute.ps1" -ProjectPath "C:\Users\Admin\arabic-code-studio-qt" -Class desktop-msi -Json
```

Those commands resolve metadata only. They do not launch the app, install MSI
packages, mutate a VM, or run GUI automation.
