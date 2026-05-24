# v0.1.2-beta (UNRELEASED)

- Search/replace project-wide scan cap raised from 500 to 5000 files. The per-file 1 MB ceiling and the per-call result limit are unchanged. Truncation status-bar message now dynamically reports the active cap.

# Lisan Studio 0.1.1 Beta Notes

## Fixed (data-safety regressions in 0.1.0-beta)

- Project-wide replace now preserves leading whitespace and per-file line endings. (was: stripped indentation, mixed CRLF/LF on apply)
- Editor save preserves the loaded file's line-ending style. (was: silently converted CRLF to LF)
- MSI install registers as a single entry in HKLM\WOW6432Node\Uninstall instead of a dual HKCU+HKLM registration. (was: two entries per install, polluted Programs and Features)
- Build identifier (git short SHA or ISO UTC timestamp) is captured at HKCU\Software\LisanStudio\BuildId for support diagnostics.

## Added

- Document model: every open file is tracked by a real DocumentRegistry with version-checked text edits, external file change polling, and a Reload/Keep/Compare prompt when files change externally.
- Workspace trust: grant + revoke commands with an audit trail at .lisan-workspace/trust-audit.jsonl, surfaced in the diagnostics bundle.
- Editor torture coverage for V1.5 gate decisions: 100k-line files, undo/redo storms, find/replace storms, IME composition cycles, soft-wrap on mixed RTL/LTR.
- Search/replace cap notice: "تم اقتطاع نتائج البحث عند 500 ملف" appears in status bar when the 500-file scan cap fires.
- Command-registry sweep: every visible command is now generatively asserted to have id/label/category/trigger/uniqueness/shortcut/no-BiDi-controls.

## Changed

- MSI test pipeline migrated to GitHub Actions (.github/workflows/msi-tests.yml). Per-step logs replace the prior custom Hyper-V harness's sparse checkpoint files.
- Editor enforces CRLF on save for new untitled buffers (Windows-default); Mixed-line-ending files are normalized to their dominant style on first save.
- MainWindow extracted into RuntimeOrchestrator, EditorTabsController, ProjectTreeController, BottomPanelController, CommandPaletteController, WorkbenchTheme. Internal refactor - no user-facing behavior change beyond the per-feature fixes above.

## Removed

- Legacy Hyper-V harness entry points (qa/vm/*, the wrapper test scripts). MSI test orchestration now lives in the GHA workflow exclusively.
- AllowSameVersionUpgrades on the WiX MajorUpgrade. Each beta now bumps a patch version; same-version replacement is no longer the upgrade cadence.

## Known limitations (deferred to V1.5+ / V2)

- No multi-cursor or column selection. (gated on QPlainTextEdit cost-model evidence)
- Terminal panel boundary exists but real shell execution is not yet wired. (gated on stronger trust model)
- No language-server protocol features (completion, diagnostics from unsaved buffers, go-to-definition). (V2 scope)
- No debugger. (V2 scope)
- No Git source-control UI. (V3 scope)
- Search and project-replace scan up to the first 500 files; larger projects truncate with a status-bar notice.

## Upgrade from 0.1.0-beta

- Install LisanStudio-0.1.1-beta.msi normally. Windows Installer's MajorUpgrade removes the prior install and installs the new one in a single transaction.
- User settings under HKCU\Software\LisanStudio survive the upgrade.
- The verified upgrade path is the one tested in GHA run 26331615773.

# Lisan Studio 0.1.0 Beta Notes

## Included

- Native Qt 6 desktop shell.
- Lisan Studio app branding and bundled logo resource.
- Premium dark visual system based on the accepted Claude design.
- Custom RTL Claude-design single top shell with integrated menus, primary run action, and unified command/search field.
- Integrated dropdown menus for file, edit, view, tools, and help.
- Primary run and command/search are single top-bar entry points to avoid duplicate run/search surfaces.
- No inherited `QMenuBar` or `QToolBar` shell surface.
- RTL editor tabs, project sidebar, bottom panel tabs, and status bar.
- Arabic-first editor surface based on `QPlainTextEdit`.
- Minimal `.apy` syntax highlighting.
- Hidden Unicode BiDi control detection.
- Project folder tree.
- Editor tabs for multiple open documents.
- Bottom panel tabs for terminal, output, problems, search results, and debug.
- Project text search with clickable file/line result rows.
- Problems panel entries for hidden BiDi controls and runtime failures.
- Run current `.apy` through bundled runtime with live output, exit code, elapsed time, and cancel action.
- RTL settings dialog with editor font controls, runtime diagnostics, and recent projects.
- Runtime diagnostics for bundled Python, `lughat-althuban`, run, lint, and format availability.
- Lisan Studio application metadata, executable name, install folder, and shortcut targets.
- Packaged Qt, Python, and `lughat-althuban` license payloads.
- WiX MSI packaging path for `LisanStudio-0.1.0-beta.msi`.
- MSI install/uninstall smoke script for private beta validation.
- Release evidence script for checksums, validation log, known issues, and screenshot capture.
- Manual beta checklist script for recording the installed-app handoff pass as a professionally formatted Word review packet, with Markdown and JSON traceability files.

## Validation Status

- Automated Homelab MSI/GUI/release evidence lane passed inside `LisanStudio-QA`.
- Manual installed-app QA is complete.
- Private beta handoff decision: ready; previously recorded non-blocking polish
  notes are resolved in the current codebase.

Resolved reviewer notes:

- Normal app launch is configured as a Windows GUI executable so it does not
  open a command window.
- Right-click Undo/Redo menu actions are Lisan-owned actions and trigger the
  same editor commands as keyboard shortcuts.
- Search result file text and line metadata are kept in a guarded RTL metadata
  cluster.
- Problems panel diagnostic subtext is right-anchored under the metadata row.

## Deferred

- Git UI.
- AI panel.
- Public distribution.
- Auto-update.
- Plugin system.

## Release Blockers

- Cursor or selection corruption in mixed Arabic/English code.
- Arabic output mojibake.
- Save/open data loss.
- Installer requiring manual PATH or Python setup.
- MSI install/uninstall smoke failure.
- Missing Qt, Python, or `lughat-althuban` license payloads.
- Hidden BiDi controls inserted by the editor.
- Any return of a native `QMenuBar` or `QToolBar` shell surface.
- Left-to-right shell regression in top command bar menus, project/sidebar, tabs, status bar, or bottom panel.
