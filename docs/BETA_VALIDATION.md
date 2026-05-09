# Lisan Studio Beta Validation

This file tracks the private beta gate for the installed Windows application.
Run validation from the installed app, not only from the build tree.

## Automated Gate

```powershell
.\scripts\validate.ps1
.\scripts\package.ps1
.\scripts\installed-smoke.ps1
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
- uninstall and confirm app payload files are removed

## Blockers

- cursor or selection corruption
- Arabic output mojibake
- save/open data loss
- installer requiring PATH or system Python setup
- hidden BiDi controls inserted by the editor
- visible placeholder UI
- any return of a native `QMenuBar` or `QToolBar` shell surface
- left-to-right shell regression in top command bar menus, project/sidebar, tabs, status bar, or bottom panel
- crash on open, save, run, launch, or close
