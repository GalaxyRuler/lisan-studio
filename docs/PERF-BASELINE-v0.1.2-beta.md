# Perf Baseline — v0.1.2-beta

Captured: 2026-05-24T19:51:15.4159244+03:00
HEAD SHA: 8cfa01e05d5d1d13d747ce9d4dc5b441e1b61552
Reference environment: local dev WHITEDRAGON

## Methodology

Each metric is gated by an inline `QVERIFY2(... budget ...)` assertion in `tests/TestEditorTorture.cpp`. The slice that produced this baseline added a parseable `qInfo` emission of the same elapsed value immediately before the budget check. To re-capture:

1. Run `.\scripts\validate.ps1`.
2. From the `acs_editor_torture_tests` output, extract lines matching `PERF metric=NAME elapsed=NUM budget=NUM`.
3. Each elapsed value is the measured wall-clock time for that path.

Values are hardware-dependent and will vary across runners. The `budget` columns are the upper bounds the test enforces; if a future capture exceeds the budget, the test fails and the slice that caused the regression must address it before merging.

On local Windows runs, Qt logging may need stderr forcing for `qInfo` messages to appear in CTest's captured output:

```powershell
$env:QT_FORCE_STDERR_LOGGING = "1"
.\scripts\validate.ps1
Select-String -Pattern "PERF metric=([^ ]+) elapsed=([0-9]+) budget=([0-9]+)" -Path build\Testing\Temporary\LastTest.log
```

## Snapshot

| Metric | Elapsed (ms) | Budget (ms) | Headroom |
|---|---:|---:|---:|
| large_file_open | 312 | 2000 | 84% |
| undo_redo_storm_100k | 66 | 500 | 87% |
| find_replace_storm_100k | 673 | 1000 | 33% |
| arabic_50k_line_render | 106 | 5000 | 98% |
| alternating_undo_redo_storm | 84 | 500 | 83% |
| find_replace_storm_300 | 46 | 1000 | 95% |
| indent_guide_paint_10k | 31 | 5000 | 99% |

Headroom = (budget - elapsed) / budget, rounded to whole percent.

## When to re-capture

- Before any V2 slice that touches the editor core (multi-cursor, column selection, syntax engine swap, etc.).
- When upgrading Qt major version.
- When the reference runner hardware changes.
- As part of each beta release-evidence packet (future).
