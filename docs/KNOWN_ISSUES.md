# Known Issues

- Search/replace project scan covers up to 5000 files (raised from 500 in v0.1.2-beta). The per-file 1 MB ceiling is unchanged. Repos with more than 5000 indexable files still see truncation; streaming search is deferred to V2.
- Untitled buffer contents are persisted to the workbench session each time the buffer transitions between clean and dirty state (typically on the first keystroke after opening a new untitled tab, or after a save). Edits made during the same dirty session — continuous typing between saves — are NOT incrementally persisted until the next transition or until graceful application shutdown. A force-kill, power loss, or crash during continuous editing preserves the content as of the most recent transition, not the latest typed state. Saved files (named, on-disk) are unaffected. Continuous incremental autosave is queued as 'V1.5 backlog: incremental autosave for untitled drafts'.
- Workaround: if you have important untitled-buffer content, use File → Save As to give it a path. Saved-file edits flush to disk on every save and follow the filesystem's durability guarantees.
- Unsigned MSI (see ADR-0010 for the deferred-code-signing decision):
  on first launch of a fresh install, Windows shows a SmartScreen
  dialog reading "Microsoft Defender SmartScreen prevented an
  unrecognized app from starting." Click **More info** and then
  **Run anyway** to proceed. The dialog appears only on first launch
  per install; subsequent launches are silent. Enterprise machines
  with SmartScreen Block enforcement may refuse to launch the
  application outright — if you encounter that, file a beta-tester
  report so we can revisit ADR-0010.
- Multi-cursor + IME limitation: committed IME text now inserts at the
  primary cursor and every secondary cursor in one undoable edit.
  Preedit/live composition text is accepted but is not yet painted as a
  separate live preview at every secondary caret. If a Windows IME shows
  confusing preedit UI with many cursors, commit the composition or press
  Esc to collapse to one cursor before composing.
- Multi-cursor + column selection: Alt+drag column / rectangle
  selection ships in v0.2.0-beta. Two limitations carried into V2:
  the drag has no live preview rectangle (cursors appear on release,
  not during the drag), and the selection operates on logical lines
  only — soft-wrapped visual rows are not yet treated as separate
  rectangle rows.
- Multi-cursor + complex editor commands: Ctrl+Z (undo), Ctrl+Y (redo),
  Tab (indent), and Shift+Tab (dedent) now apply across all active
  cursors in one undoable edit. Other non-trivial editor commands may
  still apply only to the primary cursor when secondaries are present.
  Multi-cursor typing, Backspace, Delete, arrow movement, Return, and
  committed IME text are multi-cursor-aware.
