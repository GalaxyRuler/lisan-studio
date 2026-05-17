# V1 External Change Resolution Slice

## Goal

Add pure document-registry operations for the two choices later exposed by external-change prompts: reload the disk version, or keep the in-memory version dirty for the next save.

## Scope

- Reload a tracked file from disk and clear its external-change marker.
- Keep current in-memory text after a modified or deleted backing file and mark it dirty.
- Preserve UTF-8 and line-ending metadata through the existing `DocumentFileIO` helpers.

## Validation

- Focused `acs_workbench_state_tests` red before implementation.
- Focused `acs_workbench_state_tests` green after implementation.
- Full `.\scripts\validate.ps1`.
- `git diff --check`.

## Homelab

This is a headless document-service slice. It does not run GUI automation, MSI validation, installer mutation, or active WHITEDRAGON desktop validation.
