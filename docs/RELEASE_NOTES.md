# Lisan Studio 0.1.0 Beta Notes

## Included

- Native Qt 6 desktop shell.
- Lisan Studio app branding and bundled logo resource.
- Premium dark visual system based on the accepted Claude design.
- Custom RTL Claude-design single top shell with integrated menus, primary run action, and unified command/search field.
- Integrated dropdown menus for file, edit, view, run, search, tools, and help.
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
- WiX MSI packaging path for `LisanStudio-0.1.0-beta.msi`.

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
- Hidden BiDi controls inserted by the editor.
- Any return of a native `QMenuBar` or `QToolBar` shell surface.
- Left-to-right shell regression in top command bar menus, project/sidebar, tabs, status bar, or bottom panel.
