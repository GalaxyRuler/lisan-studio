# Public Release Readiness

Status date: 2026-06-14

This checklist tracks the gap between a source-ready repository and a public
installer release.

## Current Candidate

- Repository: `GalaxyRuler/lisan-studio`
- Default branch: `main`
- Current source release: `v1.0.0-rc1`
- Release page: <https://github.com/GalaxyRuler/lisan-studio/releases/tag/v1.0.0-rc1>
- GitHub visibility at last check: private
- `v1.0.0-rc1` release assets at last check: none
- Self-hosted MSI runner at last check: offline

## Source-Public Gate

These gates must pass before switching repository visibility to public:

| Gate | Status | Evidence / Next action |
|---|---|---|
| README explains product, install, build, use, validation, and security | Done | README updated for public readers |
| Install docs do not rely on private beta packets | Done | `docs/INSTALLATION.md` rewritten for public setup |
| Internal agent planning files removed from public tree | Done | Removed private workflow notes and local runner metadata from current tree |
| Current-tree secret and private-marker scan | Done | High-confidence secret regexes: 0 hits; private-marker scan: 0 hits |
| Git history high-confidence secret scan | Done | Private key, GitHub token, OpenAI key, AWS key, Google API key, Slack token patterns: 0 hits |
| Git history private-marker scan | Blocked for direct public visibility | Old commits still contain private paths, machine names, and internal planning terms; use history rewrite or a fresh public mirror before changing visibility |
| Build/test gate | Done | `.\scripts\validate.ps1` passed 14/14 CTest tests |
| PowerShell QA gate | Done | 18/18 `qa/tests/*.ps1` scripts passed |
| GitHub repo state verified | Done | Repo is private; `v1.0.0-rc1` release exists with no MSI assets; self-hosted MSI runner is offline |

## Installer-Public Gate

These gates must pass before claiming that public users can install a release
MSI directly from GitHub:

| Gate | Status | Evidence / Next action |
|---|---|---|
| MSI artifact exists for the release | Blocked | `v1.0.0-rc1` currently has no release assets |
| MSI install smoke passed | Blocked | Requires online self-hosted Windows runner or approved isolated QA machine |
| MSI upgrade smoke passed | Blocked | Requires online self-hosted Windows runner or approved isolated QA machine |
| Signing decision is explicit | Open | Current ADR allows unsigned beta/RC builds; public GA should revisit signing |
| Release notes name installer limitations | Open | Update when an MSI asset is attached |

## Public Security Checks

Run these checks before making the repository public or attaching installer
assets:

```powershell
git status -sb
git ls-files | rg -n '(^|/)\.env|id_rsa|\.pem$|\.pfx$|\.key$|secret|token|credential|private'
rg -n '<private-machine-name>|<absolute-private-path>|<private-runner-name>|<agent-planning-folder>' .
.\scripts\validate.ps1
```

If available locally, also run a dedicated secret scanner such as Gitleaks or
TruffleHog against both the working tree and Git history.

## Visibility Change

Changing repository visibility is an owner action. Do not switch the repository
to public until:

1. source-public gates are complete,
2. current-tree scans are clean or documented,
3. Git history private-marker residue is removed, or a fresh public mirror is
   created from the cleaned current tree, and
4. the maintainer accepts any remaining unsigned-installer or missing-asset
   limitations.
