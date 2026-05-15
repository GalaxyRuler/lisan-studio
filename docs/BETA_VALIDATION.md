# Lisan Studio Beta Validation

This file tracks the private beta gate for the installed Windows application.
Run validation from the installed app, not only from the build tree.

## Current 0.1.0-beta Status

Status: ready for private beta handoff with known non-blocking manual QA notes.

Completed gates:

- Homelab MSI/GUI/release evidence lane passed inside `LisanStudio-QA`.
- Active `WHITEDRAGON` was not used for GUI/MSI validation.
- Human installed-app manual QA checklist was completed.
- Installer reinstall check passed from the VM desktop MSI:
  `C:\Users\Public\Desktop\LisanStudio-0.1.0-beta.msi`.

Recorded non-blocking reviewer notes:

- Launch: a command window appeared at the same time as app launch.
- Editing: right-click Undo/Redo menu actions were not clickable, while keyboard
  shortcuts worked.
- Search: result text appeared far from the number column.
- Problems panel: diagnostic subtext appeared far left.

These notes do not block the 0.1.0 private beta handoff, but they should be
tracked as follow-up polish before a wider release.

## Automated Gate

```powershell
.\scripts\validate.ps1
.\scripts\package.ps1
.\scripts\installed-smoke.ps1
.\scripts\msi-smoke.ps1
.\scripts\release-evidence.ps1
```

The installed smoke script verifies:

- the installed executable exists
- the installed executable is `LisanStudio.exe` under `%LOCALAPPDATA%\LisanStudio`
- the bundled Python runtime exists
- `lughat-althuban` runs a mixed Arabic/English `.apy` file
- Arabic output is decoded as UTF-8
- the app can launch with a project path
- the app can launch with a file path
- the bundled runtime has no editable local `apython` markers

The MSI smoke script verifies:

- silent MSI install from `artifacts\LisanStudio-0.1.0-beta.msi`
- pre-clean of older local Lisan Studio MSI products
- installed payload under `%LOCALAPPDATA%\LisanStudio`
- Start Menu and Desktop shortcuts
- packaged README, release notes, validation notes, and license files
- Qt, Python, and `lughat-althuban` license payloads
- installed runtime smoke through `scripts\installed-smoke.ps1`
- silent MSI uninstall removes the app executable and shortcuts
- same-version beta MSI rebuilds can replace older local beta installs

The release evidence script produces:

- `artifacts\release\0.1.0-beta\VALIDATION_LOG.md`
- `artifacts\release\0.1.0-beta\CHECKSUMS-SHA256.txt`
- `artifacts\release\0.1.0-beta\KNOWN_ISSUES.md`
- `artifacts\release\0.1.0-beta\screenshots\main-window.png`

To generate a human manual QA checklist from an existing release evidence
bundle without launching the app or installing the MSI, run:

```powershell
.\scripts\beta-manual-check.ps1
```

The checklist is written under `artifacts\beta-manual-check\` and starts with
`ManualQaStatus` set to `NotStarted`. Human reviewers should use the generated
professionally formatted `manual-beta-qa.docx` review packet first; the Markdown
and JSON files are kept for diffable traceability. Automated evidence does not
mean the manual installed-app pass is complete.

The automated Qt editor torture tests verify:

- Lisan Studio branding and bundled logo resource
- single custom RTL Claude-design top command bar with integrated menu dropdowns
- no inherited `QMenuBar` or `QToolBar` shell surface
- top shell follows the accepted Claude design: one integrated menu/run/command row
- brand row includes the logo and `Lisan Studio` text as one product block
- primary run is the only visible top-row action; stop, save, search, command palette, settings, lint, and format remain available through menus/shortcuts
- RTL editor tabs, project sidebar, bottom panel tabs, output panel, Problems panel, search results panel, and status bar
- bottom panel tab order: terminal, output, problems, search results, debug
- mixed Arabic/English text preservation
- UTF-8 save and reopen behavior
- hidden BiDi control detection
- syntax spans for Arabic keywords, strings, comments, and numbers
- selection, copy, paste, undo, redo, backspace, and delete around mixed text
- logical cursor traversal across a long mixed-direction line
- line-number gutter visibility and width scaling
- editor tab creation, tab switching, and current-document path tracking
- clickable project search rows open the matching file at the result line
- Problems panel lists hidden BiDi controls with file and line data
- run-tool feedback with command title, file path, working directory, exit code, and cancel action
- Settings opens as a real RTL dialog with editor, runtime, and recent-project categories
- runtime diagnostics report bundled Python path and `lughat-althuban` readiness
- output feedback selects the output tab even though the bottom panel defaults to terminal

## Manual Gate

Use the installed app for these checks. The sample project is:

```text
samples\torture-project
```

- launch from the Start Menu
- open a real `.apy` folder
- verify the shell title and single top command bar match the Claude design direction, with no extra native menu row
- verify Start Menu/Desktop shortcuts launch Lisan Studio, not an old ArabicCodeStudioQt install
- verify the project sidebar, editor tabs, and bottom panel are RTL
- open, edit, save, close, and reopen a mixed Arabic/English file
- open two files and verify each stays available in its own editor tab
- verify cursor movement across Arabic identifiers, English names, numbers, operators, and Windows paths
- verify selection, copy, paste, undo, redo, backspace, and delete near Arabic text
- search the project and open a result from the search-results tab
- insert a hidden BiDi control into a scratch file and confirm the Problems panel reports it
- run the current `.apy` file and confirm stdout/stderr, exit code, elapsed time, and cancel behavior are readable
- open Settings, verify runtime diagnostics, change the editor font setting, and reopen the app
- uninstall and confirm app payload files and shortcuts are removed
- reinstall from the same beta MSI without manual PATH or Python setup
- confirm user settings under the Qt app config location are not treated as MSI payload

## Blockers

- cursor or selection corruption
- Arabic output mojibake
- save/open data loss
- installer requiring PATH or system Python setup
- MSI install/uninstall smoke failure
- missing Qt, Python, or `lughat-althuban` license payloads
- hidden BiDi controls inserted by the editor
- visible placeholder UI
- any return of a native `QMenuBar` or `QToolBar` shell surface
- left-to-right shell regression in top command bar menus, project/sidebar, tabs, status bar, or bottom panel
- crash on open, save, run, launch, or close
