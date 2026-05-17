# V1 Document Change Poller Slice

## Goal

Provide the minimal polling abstraction requested by the V1 safety plan so later UI can check external file changes without knowing `DocumentRegistry` internals.

## Scope

- Add a pure `DocumentChangePoller` service over `DocumentRegistry`.
- Refresh all tracked file states when polled.
- Return a snapshot containing only externally modified or deleted documents.

## Validation

- Focused `acs_workbench_state_tests` red before implementation.
- Focused `acs_workbench_state_tests` green after implementation.
- Full `.\scripts\validate.ps1`.
- `git diff --check`.

## Homelab

This is a headless core-service slice. No GUI automation, MSI validation, installer mutation, or active WHITEDRAGON desktop validation is required.
