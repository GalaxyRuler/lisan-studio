# Arabic Code Studio Qt Beta Validation

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
- the bundled Python runtime exists
- `lughat-althuban` runs a mixed Arabic/English `.apy` file
- Arabic output is decoded as UTF-8
- the app can launch with a project path
- the app can launch with a file path
- the bundled runtime has no editable local `apython` markers

## Manual Gate

Use the installed app for these checks:

- launch from the Start Menu
- open a real `.apy` folder
- open, edit, save, close, and reopen a mixed Arabic/English file
- verify cursor movement across Arabic identifiers, English names, numbers, operators, and Windows paths
- verify selection, copy, paste, undo, redo, backspace, and delete near Arabic text
- search the project and open a result
- run the current `.apy` file and confirm stdout/stderr are readable
- change the editor font setting and reopen the app
- uninstall and confirm app payload files are removed

## Blockers

- cursor or selection corruption
- Arabic output mojibake
- save/open data loss
- installer requiring PATH or system Python setup
- hidden BiDi controls inserted by the editor
- visible placeholder UI
- crash on open, save, run, launch, or close
