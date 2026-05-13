# Beta Handoff Hardening Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the private beta handoff repeatable by generating a manual QA checklist and improving release evidence text without running GUI/MSI work on WHITEDRAGON.

**Architecture:** Add one project-owned PowerShell script that inspects an existing release evidence bundle and writes a timestamped Markdown/JSON manual QA report. Polish `release-evidence.ps1` output so automated evidence clearly separates validated automation from still-required manual QA.

**Tech Stack:** PowerShell 5-compatible scripts, Markdown release docs, repo-local `qa/tests/*.ps1` guard tests.

---

### Task 1: Manual Beta Checklist Script

**Files:**
- Create: `scripts/beta-manual-check.ps1`
- Test: `qa/tests/Test-BetaManualCheck.ps1`

- [ ] **Step 1: Write the failing test**

Create a QA test that builds a fake release bundle in a temporary directory, runs `scripts/beta-manual-check.ps1`, and asserts the generated Markdown and JSON report include artifact status, manual QA checklist items, and a `ManualQaStatus` value of `NotStarted`.

- [ ] **Step 2: Run the test and verify it fails**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\qa\tests\Test-BetaManualCheck.ps1"
```

Expected: failure because `scripts/beta-manual-check.ps1` does not exist yet.

- [ ] **Step 3: Implement the script**

Create `scripts/beta-manual-check.ps1` with parameters for `ReleaseLabel`, `ReleaseDir`, `MsiPath`, `OutputRoot`, and optional deterministic `RunId`. It must only inspect files and write reports; it must not launch the app, install MSI packages, uninstall MSI packages, mutate VMs, or touch Homelab core.

- [ ] **Step 4: Run the test and verify it passes**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\qa\tests\Test-BetaManualCheck.ps1"
```

Expected: pass.

### Task 2: Release Evidence Handoff Sections

**Files:**
- Modify: `scripts/release-evidence.ps1`
- Test: `qa/tests/Test-ReleaseEvidenceHandoffSections.ps1`

- [ ] **Step 1: Write the failing test**

Create a static QA test that parses `scripts/release-evidence.ps1` and asserts the generated validation and known-issues text includes handoff summary, isolation note, manual QA required, and deferred feature sections.

- [ ] **Step 2: Run the test and verify it fails**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\qa\tests\Test-ReleaseEvidenceHandoffSections.ps1"
```

Expected: failure until `release-evidence.ps1` contains the new handoff language.

- [ ] **Step 3: Add the handoff sections**

Update `release-evidence.ps1` so generated `VALIDATION_LOG.md` and `KNOWN_ISSUES.md` clearly state what automation validated, that manual QA is still required, that Homelab/LisanStudio-QA is the intended GUI/MSI route, and that Git UI, AI panel, public distribution, auto-update, and plugins are deferred.

- [ ] **Step 4: Run the test and verify it passes**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\qa\tests\Test-ReleaseEvidenceHandoffSections.ps1"
```

Expected: pass.

### Task 3: Documentation Alignment

**Files:**
- Modify: `docs/BETA_VALIDATION.md`
- Modify: `docs/RELEASE_NOTES.md`

- [ ] **Step 1: Document the manual checklist flow**

Update beta validation docs with the safe local command for generating a manual QA report from existing release artifacts.

- [ ] **Step 2: Clarify beta handoff status**

Update release notes to say automated release evidence does not replace the manual installed-app pass.

### Task 4: Final Verification

**Files:**
- All touched files

- [ ] **Step 1: Run repo-local QA tests**

Run all `qa/tests/*.ps1`.

- [ ] **Step 2: Parse PowerShell scripts**

Parse touched scripts with the PowerShell parser.

- [ ] **Step 3: Check git diff**

Run `git diff --check` and `git status --short`.
