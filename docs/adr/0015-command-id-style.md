# ADR-0015: Command ID Style

## Status

Accepted

## Context

Lisan Studio command IDs started with hyphenated V1 names such as
`save-file` and `run-current-file`. Later multi-cursor and workbench
commands used dotted camelCase IDs such as `cursor.addAbove` and
`document.saveAll`. Keeping both styles makes command palette rows,
shortcut JSON, tests, and future extension APIs harder to reason about.

V2 will add LSP, debugger, terminal, and refactor commands. Those features
need a single stable ID convention before more user-visible shortcuts are
introduced.

## Decision

All command IDs use dotted camelCase:

```text
category.action
category.actionDetail
category.subcategory.action
```

The enforced pattern is:

```text
^[a-z]+(\.[a-z][a-zA-Z0-9]*)+$
```

Examples:

- `file.save`
- `run.currentFile`
- `cursor.addAtNextMatch`
- `search.replaceApplyAccepted`

Legacy hyphenated IDs are not registered anymore. Shortcut settings loaded
from older JSON are migrated from old IDs to canonical IDs before validation,
so user bindings are preserved.

## Consequences

New commands must pick a short category and a camelCase action. Tests sweep
the registered command inventory and fail if any ID uses hyphens,
underscores, uppercase category names, or a single unqualified token.

Shortcut export writes only canonical IDs. Shortcut import accepts legacy IDs
only through the explicit migration table in `ShortcutSettingsModel`.

## Verification

Tests verify:

- every registered MainWindow command matches the dotted camelCase pattern;
- legacy shortcut JSON with an old ID migrates to the canonical ID while
  preserving the shortcut binding;
- exported shortcut JSON contains canonical IDs only.
