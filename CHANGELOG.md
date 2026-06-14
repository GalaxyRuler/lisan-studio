# Changelog

## 2026-05-22

### Changed

- **Test pipeline migrated to GitHub Actions.** MSI install and upgrade testing now runs in `.github/workflows/msi-tests.yml` via a self-hosted Windows runner, triggered via `workflow_dispatch`. Replaces the previous local Hyper-V harness. Per-step logs and step-level timeouts replace the old 60-minute global-timeout failure mode.

### Added

- **Cross-repo apython integration in CI.** Workflow checks out the apython runtime (`GalaxyRuler/lughat-althuban`, pinned ref) into a sibling directory and passes `-ApythonRoot` to `scripts/package.ps1` explicitly. Previously the build relied on a maintainer-local apython checkout; now the dependency is declared in version control.

### Fixed

- **MSI upgrade lane regression chain.** The upgrade-test path that failed four consecutive times under the old harness now runs green under GHA. Failures were across multiple harness layers (orchestrator hang, wrapper hang, Defender-vs-pip slowness, missing PATH entries); each surfaced cleanly in per-step logs under the new pipeline.
