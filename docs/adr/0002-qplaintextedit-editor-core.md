# ADR-0002: QPlainTextEdit-First Editor Core

## Status

Accepted

## Context

Arabic Code Studio needs robust text editing before it needs advanced IDE
features. Owning cursor movement, selection, undo, layout, and BiDi rendering
from scratch would delay release and increase bugs.

## Decision

Use `QPlainTextEdit` as the first editor core. Add product-owned behavior around
it: Arabic defaults, `.apy` highlighting, hidden BiDi detection, file IO, and
runtime commands.

## Alternatives Considered

- Custom `QTextLayout` viewport: fallback if `QPlainTextEdit` fails torture
  tests.
- Scintilla/AvalonEdit/RichEdit: not selected for the Qt-first clean rebuild.

## Consequences

The editor proof gate decides whether Qt remains viable. If cursor, selection,
or mixed-direction editing fails, do not patch around it indefinitely; switch to
the fallback spike.

## Verification

`acs_editor_tests` covers mixed Arabic code preservation, hidden BiDi controls,
highlighting, cursor selection, paste/replace, and undo behavior.

