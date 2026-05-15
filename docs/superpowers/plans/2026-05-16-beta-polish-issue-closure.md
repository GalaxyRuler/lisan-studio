# Beta Polish Issue Closure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix every currently recorded non-blocking Lisan Studio private beta issue and replace the "ready with known issues" status with fresh evidence.

**Architecture:** Treat each issue as a separate root-cause fix with a failing test first, then make the smallest targeted production change. Keep GUI/MSI/installed-app validation inside `LisanStudio-QA` through Homelab; local work is limited to source edits, static checks, and normal Qt test validation.

**Tech Stack:** C++17, Qt 6 Widgets/Test, CMake, PowerShell QA tests, WiX MSI packaging, Homelab `desktopMsiSmoke` route.

---

## Scope

This plan fixes the four issues currently recorded in `docs/BETA_VALIDATION.md` and `docs/RELEASE_NOTES.md`:

- Launch: a command window appears at normal app launch.
- Editing: right-click Undo/Redo menu actions are not clickable while keyboard shortcuts work.
- Search: result text appears far from the number column.
- Problems panel: diagnostic subtext appears far left.

It also updates release documentation only after local and isolated VM validation confirms the fixes.

## Evidence Gathered

- `CMakeLists.txt` currently declares the product app with `qt_add_executable(LisanStudio ...)` and does not pass the Windows GUI subsystem flag for the app target.
- `src/EditorSurface.cpp` uses the default `QPlainTextEdit` context menu and does not expose testable, Lisan-owned context-menu actions.
- `src/MainWindow.cpp` builds search result rows in `renderSearchResults()` with separate file and line labels in an expanding horizontal layout.
- `src/MainWindow.cpp` builds Problems rows in `addProblem()` with the message label added directly to the row layout instead of inside a right-anchored message line.
- Existing `tests/TestMainWindow.cpp` verifies basic search and Problems row content, but not the specific visual gap that the manual reviewer noticed.

## File Structure

- Modify: `CMakeLists.txt`
  - Product app subsystem only. Do not make Qt test binaries `WIN32`; test binaries should keep normal console/test behavior.
- Modify: `src/EditorSurface.h`
  - Add a public or protected context-menu factory that tests can inspect.
  - Add `contextMenuEvent()` override if needed.
- Modify: `src/EditorSurface.cpp`
  - Build Lisan-owned editor context-menu actions with stable object names and direct Undo/Redo wiring.
- Modify: `tests/TestEditorSurface.cpp`
  - Add regression coverage for enabled and triggerable right-click Undo/Redo actions.
- Modify: `src/MainWindow.cpp`
  - Tighten search result metadata layout.
  - Tighten Problems row message alignment.
- Modify: `tests/TestMainWindow.cpp`
  - Add geometry assertions for search line/file adjacency and Problems message right anchoring.
- Create: `qa/tests/Test-WindowsGuiSubsystem.ps1`
  - Static and optional built-binary guard proving the product target is configured as a Windows GUI executable.
- Modify: `docs/BETA_VALIDATION.md`
  - Move the four notes from active known issues to resolved notes after validation.
- Modify: `docs/RELEASE_NOTES.md`
  - Update validation status after validation, not before.

## Task 1: Guard the Windows GUI Subsystem

**Files:**
- Create: `qa/tests/Test-WindowsGuiSubsystem.ps1`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add the failing static QA test**

Create `qa/tests/Test-WindowsGuiSubsystem.ps1`:

```powershell
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cmakePath = Join-Path $repoRoot 'CMakeLists.txt'
$cmake = Get-Content -LiteralPath $cmakePath -Raw

if ($cmake -notmatch '(?s)qt_add_executable\s*\(\s*LisanStudio\s+WIN32\b') {
    throw 'LisanStudio must be declared as qt_add_executable(LisanStudio WIN32 ...) so normal app launch does not open a console window.'
}

$exePath = Join-Path $repoRoot 'build\LisanStudio.exe'
if (Test-Path -LiteralPath $exePath) {
    $bytes = [System.IO.File]::ReadAllBytes($exePath)
    if ($bytes.Length -lt 0x100) {
        throw "Built executable is too small to inspect: $exePath"
    }

    $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
    $optionalHeaderOffset = $peOffset + 24
    $magic = [BitConverter]::ToUInt16($bytes, $optionalHeaderOffset)
    $subsystemOffset = if ($magic -eq 0x20B) { $optionalHeaderOffset + 0x5C } else { $optionalHeaderOffset + 0x44 }
    $subsystem = [BitConverter]::ToUInt16($bytes, $subsystemOffset)

    if ($subsystem -ne 2) {
        throw "Expected Windows GUI subsystem 2 for $exePath, got subsystem $subsystem."
    }
}

'Windows GUI subsystem metadata check passed.'
```

- [ ] **Step 2: Run the test and verify it fails before the CMake change**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\qa\tests\Test-WindowsGuiSubsystem.ps1"
```

Expected: failure with `LisanStudio must be declared as qt_add_executable(LisanStudio WIN32 ...)`.

- [ ] **Step 3: Change only the product app target**

Change `CMakeLists.txt` from:

```cmake
qt_add_executable(LisanStudio
    src/main.cpp
```

to:

```cmake
qt_add_executable(LisanStudio WIN32
    src/main.cpp
```

Leave `acs_editor_tests`, `acs_project_runtime_tests`, and `acs_main_window_tests` unchanged.

- [ ] **Step 4: Run the static QA test again**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\qa\tests\Test-WindowsGuiSubsystem.ps1"
```

Expected: pass with `Windows GUI subsystem metadata check passed.`

- [ ] **Step 5: Rebuild and inspect the built executable subsystem**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\build.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -File ".\qa\tests\Test-WindowsGuiSubsystem.ps1"
```

Expected: both pass. The second command inspects `build\LisanStudio.exe` and confirms subsystem `2`.

## Task 2: Make Editor Right-Click Undo/Redo Owned and Testable

**Files:**
- Modify: `src/EditorSurface.h`
- Modify: `src/EditorSurface.cpp`
- Modify: `tests/TestEditorSurface.cpp`

- [ ] **Step 1: Add the failing editor context-menu test declaration**

In `tests/TestEditorSurface.cpp`, add a private slot:

```cpp
void contextMenuUndoRedoActionsAreEnabledAndTriggerEditorCommands();
```

- [ ] **Step 2: Add the failing test body**

Add this test body near the existing undo/redo editor tests:

```cpp
void TestEditorSurface::contextMenuUndoRedoActionsAreEnabledAndTriggerEditorCommands()
{
    EditorSurface editor;
    editor.resize(640, 360);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    editor.setPlainText(QString::fromUtf8("س = 1"));
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    QTest::keyClicks(&editor, QString::fromUtf8("\nاطبع(س)"));

    const QString editedText = editor.toPlainText();
    QVERIFY(editedText.contains(QString::fromUtf8("اطبع")));

    std::unique_ptr<QMenu> undoMenu(editor.createEditorContextMenu());
    QVERIFY(undoMenu != nullptr);
    auto *undoAction = undoMenu->findChild<QAction *>(QStringLiteral("editorContextUndoAction"));
    QVERIFY(undoAction != nullptr);
    QVERIFY(undoAction->isEnabled());
    undoAction->trigger();

    QVERIFY(editor.toPlainText().size() < editedText.size());

    std::unique_ptr<QMenu> redoMenu(editor.createEditorContextMenu());
    QVERIFY(redoMenu != nullptr);
    auto *redoAction = redoMenu->findChild<QAction *>(QStringLiteral("editorContextRedoAction"));
    QVERIFY(redoAction != nullptr);
    QVERIFY(redoAction->isEnabled());
    redoAction->trigger();

    QCOMPARE(editor.toPlainText(), editedText);
}
```

This will fail until `EditorSurface` exposes `createEditorContextMenu()` and gives Undo/Redo stable object names.

- [ ] **Step 3: Run the focused test and verify it fails**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected: C++ compile failure because `EditorSurface::createEditorContextMenu()` does not exist yet, or a test failure if a stub was added without action wiring.

- [ ] **Step 4: Add the context-menu API to `EditorSurface.h`**

Add the includes:

```cpp
#include <QContextMenuEvent>
#include <QMenu>
```

Add this public method:

```cpp
QMenu *createEditorContextMenu(QWidget *parent = nullptr);
```

Add this protected override:

```cpp
void contextMenuEvent(QContextMenuEvent *event) override;
```

- [ ] **Step 5: Implement the Lisan-owned context menu**

Add this implementation to `src/EditorSurface.cpp`:

```cpp
QMenu *EditorSurface::createEditorContextMenu(QWidget *parent)
{
    auto *menu = new QMenu(parent ? parent : this);
    menu->setLayoutDirection(Qt::RightToLeft);

    auto *undoAction = menu->addAction(QString::fromUtf8("تراجع"));
    undoAction->setObjectName(QStringLiteral("editorContextUndoAction"));
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(document()->isUndoAvailable());
    connect(undoAction, &QAction::triggered, this, &QPlainTextEdit::undo);

    auto *redoAction = menu->addAction(QString::fromUtf8("إعادة"));
    redoAction->setObjectName(QStringLiteral("editorContextRedoAction"));
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setEnabled(document()->isRedoAvailable());
    connect(redoAction, &QAction::triggered, this, &QPlainTextEdit::redo);

    menu->addSeparator();

    auto *cutAction = menu->addAction(QString::fromUtf8("قص"));
    cutAction->setObjectName(QStringLiteral("editorContextCutAction"));
    cutAction->setShortcut(QKeySequence::Cut);
    cutAction->setEnabled(textCursor().hasSelection());
    connect(cutAction, &QAction::triggered, this, &QPlainTextEdit::cut);

    auto *copyAction = menu->addAction(QString::fromUtf8("نسخ"));
    copyAction->setObjectName(QStringLiteral("editorContextCopyAction"));
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(textCursor().hasSelection());
    connect(copyAction, &QAction::triggered, this, &QPlainTextEdit::copy);

    auto *pasteAction = menu->addAction(QString::fromUtf8("لصق"));
    pasteAction->setObjectName(QStringLiteral("editorContextPasteAction"));
    pasteAction->setShortcut(QKeySequence::Paste);
    pasteAction->setEnabled(canPaste());
    connect(pasteAction, &QAction::triggered, this, &QPlainTextEdit::paste);

    menu->addSeparator();

    auto *selectAllAction = menu->addAction(QString::fromUtf8("تحديد الكل"));
    selectAllAction->setObjectName(QStringLiteral("editorContextSelectAllAction"));
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    selectAllAction->setEnabled(!document()->isEmpty());
    connect(selectAllAction, &QAction::triggered, this, &QPlainTextEdit::selectAll);

    return menu;
}

void EditorSurface::contextMenuEvent(QContextMenuEvent *event)
{
    std::unique_ptr<QMenu> menu(createEditorContextMenu(this));
    menu->exec(event->globalPos());
}
```

If the compiler reports a missing include for `QKeySequence`, add:

```cpp
#include <QKeySequence>
```

- [ ] **Step 6: Run the focused validation**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected: `acs_editor_tests` passes, including the new context-menu test.

## Task 3: Tighten Search Result Metadata Spacing

**Files:**
- Modify: `tests/TestMainWindow.cpp`
- Modify: `src/MainWindow.cpp`

- [ ] **Step 1: Add a geometry helper to `tests/TestMainWindow.cpp`**

Near the existing local helper functions, add:

```cpp
static int horizontalGap(const QRect &a, const QRect &b)
{
    if (a.right() < b.left()) {
        return b.left() - a.right();
    }
    if (b.right() < a.left()) {
        return a.left() - b.right();
    }
    return 0;
}
```

- [ ] **Step 2: Add failing adjacency assertions to `projectSearchResultRowsFillRtlViewport()`**

After the existing `fileLabel` and `detailLabel` lookup, also fetch the line label:

```cpp
auto *lineLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultLineLabel"));
QVERIFY(lineLabel != nullptr);
```

After `labelRect` is computed, add:

```cpp
const QRect lineRect(lineLabel->mapTo(results->viewport(), QPoint(0, 0)), lineLabel->size());
const int metadataGap = horizontalGap(labelRect, lineRect);
QVERIFY2(metadataGap <= 24,
    qPrintable(QStringLiteral("search result file text should sit near the line column. gap=%1 file=[%2,%3] line=[%4,%5]")
        .arg(metadataGap)
        .arg(labelRect.left())
        .arg(labelRect.right())
        .arg(lineRect.left())
        .arg(lineRect.right())));
```

Expected before the layout fix: failure when the manual issue is reproduced by geometry.

- [ ] **Step 3: Run the focused validation and confirm the search layout test fails**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected: `acs_main_window_tests` fails on the new `metadataGap <= 24` assertion.

- [ ] **Step 4: Rework `renderSearchResults()` metadata into a right-anchored cluster**

In `src/MainWindow.cpp`, replace the current `metaLayout` block in `renderSearchResults()` with a right-anchored cluster:

```cpp
auto *metaOuterLayout = new QHBoxLayout;
metaOuterLayout->setDirection(QBoxLayout::RightToLeft);
metaOuterLayout->setContentsMargins(0, 0, 0, 0);
metaOuterLayout->setSpacing(0);

auto *metadataCluster = new QWidget(rowWidget);
metadataCluster->setObjectName(QStringLiteral("searchResultMetadataCluster"));
metadataCluster->setLayoutDirection(Qt::RightToLeft);
metadataCluster->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
auto *metaLayout = new QHBoxLayout(metadataCluster);
metaLayout->setDirection(QBoxLayout::RightToLeft);
metaLayout->setContentsMargins(0, 0, 0, 0);
metaLayout->setSpacing(8);
```

Keep the existing `fileLabel` and `lineLabel` creation, but set the file label to content-sized instead of row-expanding:

```cpp
fileLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
fileLabel->setMinimumWidth(qMin(searchResultsPanel->viewport()->width() * 2 / 3, 760));
```

Then add the widgets to the cluster and anchor the cluster to the right:

```cpp
metaLayout->addWidget(lineLabel);
metaLayout->addWidget(fileLabel);
metaOuterLayout->addWidget(metadataCluster);
metaOuterLayout->addStretch(1);
```

Finally change:

```cpp
rowLayout->addLayout(metaLayout);
```

to:

```cpp
rowLayout->addLayout(metaOuterLayout);
```

- [ ] **Step 5: Run validation and confirm the search spacing assertion passes**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected: `projectSearchResultRowsFillRtlViewport()` passes and existing search-result tests still pass.

## Task 4: Tighten Problems Panel Diagnostic Subtext Alignment

**Files:**
- Modify: `tests/TestMainWindow.cpp`
- Modify: `src/MainWindow.cpp`

- [ ] **Step 1: Add geometry assertions for the hidden-BiDi Problems row**

In `problemsPanelShowsHiddenBidiWarnings()`, set the window size and show the window before inspecting row geometry:

```cpp
window.resize(1000, 700);
window.show();
QVERIFY(QTest::qWaitForWindowExposed(&window));
QCoreApplication::processEvents();
```

After `message` is found, add:

```cpp
const int viewportWidth = problems->viewport()->width();
const QRect locationRect(location->mapTo(problems->viewport(), QPoint(0, 0)), location->size());
const QRect messageRect(message->mapTo(problems->viewport(), QPoint(0, 0)), message->size());
QVERIFY2(messageRect.right() > viewportWidth - 260,
    qPrintable(QStringLiteral("problem message should stay near the right edge. messageRight=%1 viewport=%2")
        .arg(messageRect.right())
        .arg(viewportWidth)));
QVERIFY2(messageRect.left() >= locationRect.left() - 32,
    qPrintable(QStringLiteral("problem message should align under the right-side location text. messageLeft=%1 locationLeft=%2")
        .arg(messageRect.left())
        .arg(locationRect.left())));
```

- [ ] **Step 2: Add the same geometry check to runtime failure rows**

In `problemsPanelShowsRuntimeFailureRows()`, after `message` is found, add:

```cpp
const int viewportWidth = problems->viewport()->width();
const QRect locationRect(location->mapTo(problems->viewport(), QPoint(0, 0)), location->size());
const QRect messageRect(message->mapTo(problems->viewport(), QPoint(0, 0)), message->size());
QVERIFY2(messageRect.right() > viewportWidth - 260,
    qPrintable(QStringLiteral("runtime problem message should stay near the right edge. messageRight=%1 viewport=%2")
        .arg(messageRect.right())
        .arg(viewportWidth)));
QVERIFY2(messageRect.left() >= locationRect.left() - 32,
    qPrintable(QStringLiteral("runtime problem message should align under the right-side location text. messageLeft=%1 locationLeft=%2")
        .arg(messageRect.left())
        .arg(locationRect.left())));
```

- [ ] **Step 3: Run validation and confirm the Problems layout test fails**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected: `acs_main_window_tests` fails on the new Problems row alignment assertions if the manual issue is still present.

- [ ] **Step 4: Wrap the Problems message label in a right-anchored message line**

In `src/MainWindow.cpp`, inside `addProblem()`, replace:

```cpp
rowLayout->addWidget(topLine);
rowLayout->addWidget(messageLabel);
```

with:

```cpp
auto *messageLine = new QWidget(row);
messageLine->setObjectName(QStringLiteral("problemMessageLine"));
messageLine->setLayoutDirection(Qt::RightToLeft);
auto *messageLayout = new QHBoxLayout(messageLine);
messageLayout->setDirection(QBoxLayout::RightToLeft);
messageLayout->setContentsMargins(0, 0, 0, 0);
messageLayout->setSpacing(0);
messageLayout->addWidget(messageLabel);
messageLayout->addStretch(1);

rowLayout->addWidget(topLine);
rowLayout->addWidget(messageLine);
```

Also set the message label policy before adding it:

```cpp
messageLabel->setLayoutDirection(Qt::RightToLeft);
messageLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
```

- [ ] **Step 5: Run validation and confirm Problems layout assertions pass**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected: `problemsPanelShowsHiddenBidiWarnings()` and `problemsPanelShowsRuntimeFailureRows()` pass.

## Task 5: Update Beta Status Documentation After Validation

**Files:**
- Modify: `docs/BETA_VALIDATION.md`
- Modify: `docs/RELEASE_NOTES.md`

- [ ] **Step 1: Keep documentation unchanged until Tasks 1-4 pass locally**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -File ".\qa\tests\Test-WindowsGuiSubsystem.ps1"
```

Expected: both pass.

- [ ] **Step 2: Update `docs/BETA_VALIDATION.md`**

Change:

```markdown
Status: ready for private beta handoff with known non-blocking manual QA notes.
```

to:

```markdown
Status: ready for private beta handoff with previously recorded polish notes resolved in the current codebase.
```

Replace the active `Recorded non-blocking reviewer notes:` list with:

```markdown
Resolved reviewer notes:

- Launch: normal app launch is configured as a Windows GUI executable so it does not open a command window.
- Editing: right-click Undo/Redo menu actions are Lisan-owned actions and trigger the same editor commands as keyboard shortcuts.
- Search: result file text and line metadata are kept in a right-anchored cluster.
- Problems panel: diagnostic subtext is right-anchored under the metadata row.
```

- [ ] **Step 3: Update `docs/RELEASE_NOTES.md`**

Change:

```markdown
- Private beta handoff decision: ready with known non-blocking issues.
```

to:

```markdown
- Private beta handoff decision: ready; previously recorded non-blocking polish notes are resolved in the current codebase.
```

Replace the `Known non-blocking manual QA notes:` list with the same resolved-note list used in `docs/BETA_VALIDATION.md`.

## Task 6: Full Local Verification

**Files:**
- All touched files

- [ ] **Step 1: Run all project QA PowerShell tests**

Run:

```powershell
Get-ChildItem ".\qa\tests" -Filter "*.ps1" | Sort-Object Name | ForEach-Object {
    powershell -NoProfile -ExecutionPolicy Bypass -File $_.FullName
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
```

Expected: every `qa/tests/*.ps1` script exits successfully.

- [ ] **Step 2: Parse touched PowerShell scripts**

Run:

```powershell
powershell -NoProfile -Command "$files = @('.\qa\tests\Test-WindowsGuiSubsystem.ps1'); foreach ($f in $files) { $tokens=$null; $errors=$null; [System.Management.Automation.Language.Parser]::ParseFile((Resolve-Path $f), [ref]$tokens, [ref]$errors) | Out-Null; if ($errors.Count) { $errors | Format-List; exit 1 } }; 'PowerShell parse passed.'"
```

Expected: `PowerShell parse passed.`

- [ ] **Step 3: Run full local validation**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected:

- `acs_editor_tests` passes.
- `acs_project_runtime_tests` passes.
- `acs_main_window_tests` passes.

- [ ] **Step 4: Check the diff**

Run:

```powershell
git diff --check
git status --short
```

Expected: no whitespace errors; only intended files are modified or created.

## Task 7: Isolated Homelab Validation

**Files:**
- No source edits in this task unless validation finds a real failure.

- [ ] **Step 1: Run the Homelab route only from the central Homelab repo**

Run from `C:\Users\Admin\Documents\Codex\Homelab\codex-isolated-test-runners`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\tools\codex-runner\Invoke-HomelabRuntimeLane.ps1" -ProjectPath "C:\Users\Admin\arabic-code-studio-qt" -Profile desktopMsiSmoke -Stage FullLisanSmoke -NoDryRun -AllowMutation -IUnderstandThisRunsBoundedRuntime -Json
```

Expected:

- `ok` is `true`.
- `activeWhitedragonUsed` is `false`.
- `vmName` is `LisanStudio-QA`.
- `MsiGuiSmoke` is `true`.
- Release evidence and screenshot artifacts are copied back under the Homelab runtime lane artifact directory.

- [ ] **Step 2: If Homelab validation fails, stop and diagnose the exact failing stage**

Inspect:

```powershell
Get-Content -LiteralPath "<reported-homelab-runtime-lane-report.json>" -Raw | ConvertFrom-Json | ConvertTo-Json -Depth 8
```

Then inspect the stage-specific copied artifacts under:

```text
C:\Users\Admin\Documents\Codex\Homelab\codex-isolated-test-runners\tools\codex-runner\artifacts\<runtime-lane>\msi-gui-smoke\
```

Do not weaken GUI-sensitive tests. Fix the code or packaging root cause and rerun local validation before rerunning Homelab.

## Task 8: Manual Confirmation and Evidence Closure

**Files:**
- Modify: `docs/BETA_VALIDATION.md`
- Modify: `docs/RELEASE_NOTES.md`

- [ ] **Step 1: Generate or update the manual QA Word packet after the passing Homelab lane**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\beta-manual-check.ps1"
```

Expected: a professionally formatted manual QA `.docx` review packet appears under `artifacts\beta-manual-check\`.

- [ ] **Step 2: Perform a quick installed-app manual check inside `LisanStudio-QA`**

Use the installed app or VM desktop MSI inside `LisanStudio-QA` only. Confirm:

- Normal launch does not open a command window.
- Right-click Undo and Redo are clickable after creating undo/redo history.
- Search result file/path text sits next to the line column.
- Problems diagnostic subtext sits under the right-side metadata, not far left.

- [ ] **Step 3: Add the latest Homelab report path to `docs/BETA_VALIDATION.md`**

Add a short line under completed gates:

```markdown
- Latest polish-closure Homelab report: `<report path from Task 7>`.
```

- [ ] **Step 4: Commit the issue closure**

Run:

```powershell
git add CMakeLists.txt src/EditorSurface.h src/EditorSurface.cpp src/MainWindow.cpp tests/TestEditorSurface.cpp tests/TestMainWindow.cpp qa/tests/Test-WindowsGuiSubsystem.ps1 docs/BETA_VALIDATION.md docs/RELEASE_NOTES.md
git commit -m "fix: close beta polish issues"
```

Expected: commit succeeds with only the issue-closure files staged.

## Self-Review

Spec coverage:

- [ ] Launch command window has a CMake subsystem fix and a PowerShell guard test.
- [ ] Right-click Undo/Redo has a failing Qt test and Lisan-owned context-menu implementation.
- [ ] Search spacing has geometry assertions and a right-anchored metadata-cluster fix.
- [ ] Problems subtext alignment has geometry assertions and a right-anchored message-line fix.
- [ ] Documentation is updated only after validation.
- [ ] Homelab validation stays inside `LisanStudio-QA`; active `WHITEDRAGON` is not used for GUI/MSI validation.

Placeholder scan:

- [ ] No unresolved placeholder markers remain.
- [ ] Every task has exact files, commands, and expected outcomes.

Risk check:

- [ ] The app target becomes `WIN32`, but test targets remain normal test executables.
- [ ] The editor context menu preserves cut/copy/paste/select-all behavior while fixing Undo/Redo.
- [ ] Layout fixes preserve RTL shell behavior and LTR islands for Windows paths.
- [ ] Documentation does not claim "no issues" until local validation, Homelab validation, and manual confirmation agree.
