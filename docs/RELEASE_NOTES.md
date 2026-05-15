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
