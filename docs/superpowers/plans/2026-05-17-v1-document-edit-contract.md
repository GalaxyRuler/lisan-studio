# V1 Document Edit Contract Slice

## Goal

Expose a version-checked text edit operation on `DocumentRegistry` so later replace, format, rename, and diagnostics workflows can reject stale edits safely.

## Scope

- Add a small `DocumentTextEdit` value type.
- Apply edits only when the caller's expected document version matches.
- Reject out-of-range edits without mutating text or version.
- Mark successful edits dirty and advance document version.

## Validation

- Focused `acs_workbench_state_tests` red before implementation.
- Focused `acs_workbench_state_tests` green after implementation.
- Full `.\scripts\validate.ps1`.
- `git diff --check`.

## Homelab

This is a headless core-service slice. It does not run GUI automation, MSI validation, installer mutation, or active WHITEDRAGON desktop validation.
