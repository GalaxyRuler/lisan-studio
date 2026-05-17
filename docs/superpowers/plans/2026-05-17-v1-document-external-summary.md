# V1 Document External Summary Slice

## Goal

Give later watcher, replace, and session-restore work a registry-level way to refresh and inspect external file changes across all tracked documents.

## Scope

- Add tests for batch external-state refresh across clean, modified, deleted, and untitled documents.
- Add tests for querying only documents whose backing files changed externally.
- Implement the smallest `DocumentRegistry` API needed by later safety flows.

## Validation

- Focused `acs_workbench_state_tests` red before implementation.
- Focused `acs_workbench_state_tests` green after implementation.
- Full `.\scripts\validate.ps1`.
- `git diff --check`.

## Homelab

This is a headless/unit-test slice. No GUI automation, MSI validation, installer mutation, or active WHITEDRAGON desktop validation is required.
