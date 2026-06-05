# Public Release Readiness

Status date: 2026-06-05

This file tracks the remaining gates between the in-repo V2 public-use
candidate and a public release. The canonical slice tracker remains
`docs/V2-EXECUTION-PLAN.md`.

## Current Candidate

- Branch: `codex/v2-public-use`
- Draft PR: <https://github.com/GalaxyRuler/lisan-studio/pull/9>
- Release candidate: `v0.5.0-beta`
- Local MSI: `artifacts/LisanStudio-0.5.0-beta.msi`
- MSI size: 69,198,344 bytes
- MSI SHA256: `982F6A12D8BACC24402163BC177A0FD6CEA0737740ACBF83ACFA22EC2E9AB298`
- Signing status: `AuthenticodeStatus: NotSigned`, signer `None`

## Gate Status

| Gate | Status | Evidence / Next action |
|---|---|---|
| Source branch published | Done | `codex/v2-public-use` pushed to `origin` |
| Draft PR opened | Done | PR #9: <https://github.com/GalaxyRuler/lisan-studio/pull/9> |
| Local validation | Done | `./scripts/validate.ps1` passed 13/13 |
| Local package validation | Done | `./scripts/package.ps1 -ProductVersion 0.5.0 ...` passed package-time validation 13/13 from commit `b206fe2` |
| GitHub Actions MSI evidence | Pending | Run <https://github.com/GalaxyRuler/lisan-studio/actions/runs/27007455061> is queued; `lisanstudio-qa` self-hosted runner is currently offline |
| Installed-app QA | Pending | Must run inside `LisanStudio-QA`; do not run GUI/MSI install or uninstall on active WHITEDRAGON |
| Authenticode signing | Repo hook ready / external key pending | `scripts/package.ps1` supports optional thumbprint-based `signtool.exe` signing via `-SigningCertificateThumbprint` or `LISAN_SIGNING_CERT_THUMBPRINT`. A real OV/EV certificate, key storage, and CI secret/variable configuration remain external release gates |
| Tag and GitHub release | Pending | Wait for PR review/merge, MSI evidence, signing decision, and operator release approval |

## Runner Evidence

`gh api repos/GalaxyRuler/lisan-studio/actions/runners` reported the
`lisanstudio-qa` runner as:

```json
{
  "name": "lisanstudio-qa",
  "status": "offline",
  "busy": false,
  "labels": ["self-hosted", "Windows", "X64", "lisanstudio-qa"]
}
```

The full MSI workflow was dispatched with:

```powershell
gh workflow run msi-tests.yml --repo GalaxyRuler/lisan-studio --ref codex/v2-public-use --field scenario=full
```

Run URL: <https://github.com/GalaxyRuler/lisan-studio/actions/runs/27007455061>

## Operating Boundary

Homelab runner instructions require explicit operator approval before VM
creation, VM start/stop, checkpointing, Hyper-V mutation, MSI install/uninstall,
or GUI automation. Because the self-hosted runner is offline, the queued GitHub
Actions run is the correct non-desktop handoff point until the operator brings
`LisanStudio-QA` online or approves runner lifecycle work.

## Signing Inputs

The package script signs only when a certificate thumbprint is supplied. It
does not accept, print, or commit private-key material.

Expected release-run inputs:

- `LISAN_SIGNING_CERT_THUMBPRINT`: secret containing the installed certificate
  thumbprint.
- `LISAN_SIGNTOOL_PATH`: optional repository/environment variable for a
  non-default `signtool.exe` path.
- `LISAN_TIMESTAMP_URL`: optional repository/environment variable, defaulting
  locally to `http://timestamp.digicert.com`.
