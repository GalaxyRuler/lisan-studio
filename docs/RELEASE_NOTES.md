# v0.3.0-beta (2026-06-05)

Phase C V2 release: first user-visible LSP foundation for Arabic-first
editing.

## Added

- Lisan Studio now starts and synchronizes a Qt-side LSP client for `.apy`
  documents through the bundled `lughat-althuban-lsp` runtime path.
- The editor can request `textDocument/completion` and show a debounced
  completion popup. Accepting a completion replaces the current identifier
  prefix instead of duplicating the typed text.
- The editor can request `textDocument/hover` on identifier dwell and surface
  the returned markdown in a tooltip.
- `acs_lsp_client_tests` covers initialize, document sync, completion parsing,
  hover parsing, and local round-trip budget checks.

## Changed

- The repository now vendors `lsp-framework` as the first submodule under
  `third_party/lsp-framework`; fresh checkouts must initialize submodules.
- GitHub MSI test checkout steps use recursive submodule checkout so the LSP
  transport is present in CI.

## Known v0.3.0-beta limitations

- Completion and hover depend on the bundled apython LSP runtime being present
  and initializable. If the runtime is missing, LSP UI features stay silent.
- Completion insertion uses the current identifier prefix plus `insertText`.
  LSP `textEdit` replacement ranges are not yet consumed.
- Manual installed-app QA, GitHub Actions MSI runs, tag creation, and release
  publication remain operator-owned steps after this in-repo release prep.

# v0.2.2-beta (2026-06-05)

Phase B V2 release: workbench recovery, search truncation visibility,
and command ID stability.

## Added

- Dirty untitled buffers are autosaved while editing and can be recovered
  after a non-orderly shutdown. Graceful shutdown clears the recovery
  sentinel.
- Project search now reports when a scan hits the configured file cap, so
  users can tell the difference between complete and truncated results.
- Command IDs now use dotted camelCase consistently across registered
  workbench commands, command surfaces, shortcuts, and tests.

## Changed

- Legacy shortcut JSON using V1 hyphenated command IDs is migrated during
  import, preserving user bindings and exporting canonical IDs afterward.
- ADR-0015 documents the public command ID convention for future extension,
  LSP, debugger, terminal, and refactor commands.

## Known v0.2.2-beta limitations

- Draft recovery currently prompts only when the previous session left the
  workbench sentinel uncleared and saved draft payloads exist.
- The search truncation cap is surfaced in status text, but there is not yet
  a full search-index progress UI.
- Push, CI, tag creation, and release publication remain operator-owned
  steps after this in-repo release prep.

# v0.2.1-beta (2026-06-05)

Phase A V2 release: multi-cursor editing now covers the deferred
undo/redo, indentation, and committed IME text paths.

## Added

- Ctrl+Z and Ctrl+Y are pinned by regression tests for multi-cursor
  edits. A single undo removes text inserted at all cursors, and a
  single redo restores it.
- Tab indents every active cursor line by one four-space level.
  Shift+Tab dedents every active cursor line by one level. Each
  operation is one undo step.
- Committed IME text, including Arabic composition commits, inserts at
  the primary cursor and every secondary cursor in one undoable edit.

## Known v0.2.1-beta limitations

- Live IME preedit text is accepted but is not painted as a separate
  preview at every secondary caret. The committed text is dispatched to
  all cursors when the composition is finalized.
- Alt+drag column selection still has no live preview rectangle during
  drag and still operates on logical lines rather than soft-wrapped
  visual rows.
- Other non-trivial editor commands outside Ctrl+Z, Ctrl+Y, Tab,
  Shift+Tab, Return, Backspace, Delete, arrow movement, typing, and
  committed IME text may still apply only to the primary cursor while
  secondaries are active.

# v0.2.0-beta (2026-05-26)

V1.5 milestone-close release. The V1.5 roadmap is archived at
`docs/ROADMAP-V1.5-archive.md`; V2 planning continues in
`docs/ROADMAP-V2.md`.

## Added

- Column / rectangle selection via Alt+drag. Hold Alt and left-drag in
  the editor to generate one cursor per line in the rectangle, each
  with a selection (or zero-width cursor when columns coincide)
  spanning the rectangle's column range. Reuses all of v0.1.3-beta's
  multi-cursor machinery — atomic edit blocks, soft/hard caps, Esc
  collapse — so typing, Backspace, Delete, arrow movement, and Return
  apply across the generated cursors just like Ctrl+Click or Ctrl+D
  cursors.

## V1.5 milestone summary (informational)

V1.5 shipped across two cumulative release tracks during May 2026:

- **v0.1.2-beta** (2026-05-24): search/replace project-scan cap raised
  from 500 to 5000 files; release-evidence trust-audit instrumentation;
  perf baseline established; SmartScreen click-path documented;
  ADR-0010 defers MSI code signing.
- **v0.1.3-beta** (2026-05-24): multi-cursor primary+secondary editing
  keystone (Ctrl+Click, Ctrl+Alt+Up/Down, Ctrl+D, Ctrl+Shift+L, Esc,
  soft cap 100 / hard cap 1000, single-undo invariant).
- **v0.2.0-beta** (2026-05-26, this release): Alt+drag column selection
  closes the V1.5 multi-cursor keystone; milestone closed.

## Known v0.2.0-beta limitations

- Alt+drag column selection has no live preview rectangle during the
  drag. The generated cursors appear on mouse release. Live preview
  is deferred to V2.
- Alt+drag column selection operates on logical lines only. If the
  editor is showing soft-wrapped lines, the drag rectangle treats
  each logical line as a single row rather than each wrapped visual
  row. Soft-wrap-as-visual-row column selection is deferred to V2.

# v0.1.3-beta (2026-05-24)

## Added

- Multi-cursor editing in the editor surface. Hold Ctrl and click to add
  a cursor at the click position. Ctrl+Alt+Up and Ctrl+Alt+Down add
  cursors above/below the primary cursor at the same column. Ctrl+D
  adds a cursor at the next exact match of the current selection.
  Ctrl+Shift+L converts every find-match in the document into a cursor.
  Esc collapses to a single cursor. Typing, Backspace, Delete, arrow
  movement, and Return apply to all cursors atomically with a single
  undo step.
- Soft cap notice when 100 simultaneous cursors are active; hard cap at
  1000 cursors to prevent runaway UI lock-ups from select-all-matches on
  very large documents.
- Five new commands in the command palette and Edit menu, all
  rebindable via the shortcuts JSON: `cursor.addAbove`, `cursor.addBelow`,
  `cursor.addAtNextMatch`, `cursor.selectAllMatches`,
  `cursor.collapseToSingle`.

## Known v0.1.3-beta limitations

- IME composition is refused while secondary cursors are active. The
  editor emits a status-bar notice when this happens. Press Esc to
  collapse to a single cursor first, then use IME.
- Column / rectangle selection is not yet supported. Deferred to a
  future beta.
- Undo/redo (Ctrl+Z, Ctrl+Y), Tab indent, and other complex editor
  commands apply only to the primary cursor in this beta. Multi-cursor
  extensions for these commands are planned.

# v0.1.2-beta (2026-05-24)

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
