# V1 Document Versioning Slice

## Goal

Give the document model a monotonically increasing text version so later diagnostics, formatting, rename, and replace operations can reject stale edits.

## Scope

- Add a `version` field to `DocumentRecord`.
- Start opened and untitled documents at version 1.
- Increment the version when registry text changes or a document reloads from disk.
- Keep metadata-only clean/save updates from changing the text version.

## Validation

- Focused `acs_workbench_state_tests` red before implementation.
- Focused `acs_workbench_state_tests` green after implementation.
- Full `.\scripts\validate.ps1`.
- `git diff --check`.

## Homelab

This is a headless core-service slice. No GUI automation, MSI validation, installer mutation, or active WHITEDRAGON desktop validation is required.
