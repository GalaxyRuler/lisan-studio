# V1 Diagnostics Bundle Slice

## Goal

Add a headless crash/log diagnostics bundle script that gathers non-secret support evidence without launching the app, installing MSI packages, or touching active WHITEDRAGON GUI state.

## Scope

- Add a QA test for diagnostics bundle generation.
- Add `scripts/collect-diagnostics.ps1`.
- Collect environment, git status, and safe log artifacts.
- Exclude secret-like files such as `.env`, tokens, credentials, keys, and certificates.
- Produce a zip plus JSON manifest.

## Validation

- Run `qa/tests/Test-DiagnosticsBundle.ps1` red before implementation.
- Run the same QA test green after implementation.
- Parse touched PowerShell scripts.
- Run full `.\scripts\validate.ps1`.
- Run `git diff --check`.

## Homelab

This is inspection-only and headless. No GUI automation, MSI install/uninstall, registry mutation, or active WHITEDRAGON desktop validation is required.
