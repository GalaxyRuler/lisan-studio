# Contributing to Lisan Studio

Lisan Studio is a native Qt/C++ Windows IDE for Arabic-first `.apy`
development. Contributions should preserve the Arabic-first RTL experience,
Windows installer safety, and data-safety guarantees.

## Before You Start

1. Fork or branch from `main`.
2. Initialize submodules:

   ```powershell
   git submodule update --init --recursive
   ```

3. Read the relevant docs:
   - [README.md](README.md)
   - [docs/INSTALLATION.md](docs/INSTALLATION.md)
   - [docs/ROADMAP-V3.md](docs/ROADMAP-V3.md)
   - [docs/adr/](docs/adr/)

## Branches

Use short descriptive branches:

```text
codex/<feature-or-fix>
docs/<topic>
fix/<bug>
```

Keep pull requests focused. Avoid mixing source changes, release artifacts,
format churn, and unrelated docs edits.

## Development Checks

For normal source changes:

```powershell
.\scripts\validate.ps1
```

For packaging changes, also run or arrange the appropriate MSI validation in an
isolated Windows QA environment:

```powershell
.\scripts\package.ps1 -ProductVersion <version> -ApythonRoot "<path>" -PythonRoot "<path>"
gh workflow run msi-tests.yml -f scenario=full
```

Do not run MSI install, uninstall, upgrade, registry-mutating, or GUI automation
checks on an active work desktop unless that is explicitly intended for the
current validation pass.

## Public Repository Hygiene

Do not commit:

- `.env*`
- private keys, certificates, signing material, tokens, or credentials
- local machine paths that are not portable setup examples
- generated build output under `build/`, `stage/`, `artifacts/`, or `out/`
- private planning notes, vault exports, or agent scratch files

If a change touches packaging or release automation, check that public docs do
not claim an installer, signature, release asset, or validation run exists
unless it has been verified.

## Code Style

- C++17.
- Qt 6 APIs and idioms.
- Keep UI text and layout Arabic-first / RTL-aware.
- Prefer existing controllers and services before adding new abstractions.
- Add tests for behavior changes.
- Keep comments short and useful.

## ADRs

Architecture decisions live in `docs/adr/`. Add a new ADR when a change affects
long-term architecture, release policy, installer behavior, runtime ownership,
or public security posture.

## Pull Request Checklist

- Scope is focused and described.
- Relevant tests or validation commands were run.
- Public docs were updated when behavior, installation, packaging, or release
  expectations changed.
- No secrets or private environment details are included.
- MSI and GUI validation, when required, ran in an isolated Windows QA
  environment.
