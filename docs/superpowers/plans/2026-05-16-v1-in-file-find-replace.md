# V1 In-File Find/Replace Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add dependable active-editor find/replace with Arabic, English, mixed-direction, and hidden-BiDi-adjacent text coverage.

**Architecture:** Keep durable document safety in the existing document services and make find/replace a small active-document layer. Add a pure `EditorFindService` for match and replacement calculations, then expose minimal `EditorSurface` methods for match highlighting/navigation and text mutation, and finally add a compact MainWindow find/replace panel wired through command IDs.

**Tech Stack:** C++17, Qt 6 Core/Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

Included:

- Active editor find matches for Arabic, English, mixed-direction paths/code, and hidden-BiDi-adjacent strings.
- Next/previous navigation with visible selection.
- Highlight all current matches through `QPlainTextEdit::ExtraSelection`.
- Replace current selected match.
- Replace all matches in the active editor.
- Command registry IDs and a compact in-file find/replace panel.

Excluded:

- Project-wide replace.
- Regex search.
- Case-sensitive UI controls.
- Whole-word search.
- Persistent find history.

## File Structure

- Create: `src/EditorFindService.h`
- Create: `src/EditorFindService.cpp`
  - Pure text matching and replacement helpers.
- Modify: `src/EditorSurface.h`
- Modify: `src/EditorSurface.cpp`
  - Store current find query/matches, update extra selections, navigate matches, replace current/all.
- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
  - Add find/replace panel widgets and command registration.
- Modify: `CMakeLists.txt`
  - Add service files to `acs_core`.
- Modify: `tests/TestProjectSearchRuntime.cpp`
  - Service tests.
- Modify: `tests/TestEditorSurface.cpp`
  - Editor navigation/highlight/replace tests.
- Modify: `tests/TestMainWindow.cpp`
  - Command/UI integration tests.

---

## Task 1: Pure Editor Find Service

**Files:**

- Create: `src/EditorFindService.h`
- Create: `src/EditorFindService.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/TestProjectSearchRuntime.cpp`

- [ ] **Step 1: Write failing service tests**

Add:

```cpp
#include "EditorFindService.h"
```

Add private slots:

```cpp
void editorFindServiceFindsArabicEnglishAndMixedMatches();
void editorFindServiceReplacesCurrentAndAllMatches();
```

Add tests:

```cpp
void TestProjectSearchRuntime::editorFindServiceFindsArabicEnglishAndMixedMatches()
{
    const QString text = QString::fromUtf8(
        "عدد = 1\n"
        "path = \"C:/Users/Admin/مشروع/main.apy\"\n"
        "اطبع(عدد)\n"
        "اسم = \"سارة\"\u202E\n");

    const auto arabic = EditorFindService::findAll(text, QString::fromUtf8("عدد"));
    QCOMPARE(arabic.size(), 2);
    QCOMPARE(arabic.first().start, 0);
    QCOMPARE(arabic.first().length, QString::fromUtf8("عدد").size());

    const auto english = EditorFindService::findAll(text, QStringLiteral("path"));
    QCOMPARE(english.size(), 1);

    const auto mixed = EditorFindService::findAll(text, QString::fromUtf8("مشروع/main.apy"));
    QCOMPARE(mixed.size(), 1);

    const auto hiddenAdjacent = EditorFindService::findAll(text, QString::fromUtf8("سارة"));
    QCOMPARE(hiddenAdjacent.size(), 1);
}

void TestProjectSearchRuntime::editorFindServiceReplacesCurrentAndAllMatches()
{
    QString text = QString::fromUtf8("عدد = 1\nاطبع(عدد)\n");
    const auto matches = EditorFindService::findAll(text, QString::fromUtf8("عدد"));
    QCOMPARE(matches.size(), 2);

    QVERIFY(EditorFindService::replaceAt(&text, matches.first(), QString::fromUtf8("قيمة")));
    QCOMPARE(text, QString::fromUtf8("قيمة = 1\nاطبع(عدد)\n"));

    int replaced = 0;
    text = EditorFindService::replaceAll(text, QString::fromUtf8("عدد"), QString::fromUtf8("قيمة"), &replaced);
    QCOMPARE(replaced, 1);
    QCOMPARE(text, QString::fromUtf8("قيمة = 1\nاطبع(قيمة)\n"));
}
```

- [ ] **Step 2: Run focused test and verify red**

Run:

```powershell
$ErrorActionPreference='Stop'
$bashPath = 'C:\msys64\usr\bin\bash.exe'
$worktreePath = (Resolve-Path -LiteralPath '.').Path
$driveLetter = $worktreePath.Substring(0, 1).ToLowerInvariant()
$pathRest = $worktreePath.Substring(2).Replace('\', '/')
$worktreeUnix = "/$driveLetter$pathRest"
& $bashPath -lc "set -euo pipefail; export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH; cd '$worktreeUnix'; cmake --build build --target acs_project_runtime_tests; ./build/acs_project_runtime_tests.exe -o -,txt"
```

Expected: compile failure because `EditorFindService.h` does not exist.

- [ ] **Step 3: Implement service**

Create `src/EditorFindService.h`:

```cpp
#pragma once

#include <QString>
#include <QVector>

struct EditorFindMatch
{
    int start = -1;
    int length = 0;
};

class EditorFindService final
{
public:
    static QVector<EditorFindMatch> findAll(
        const QString &text,
        const QString &query,
        Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive);
    static bool replaceAt(QString *text, const EditorFindMatch &match, const QString &replacement);
    static QString replaceAll(
        const QString &text,
        const QString &query,
        const QString &replacement,
        int *replacedCount = nullptr,
        Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive);
};
```

Create `src/EditorFindService.cpp`:

```cpp
#include "EditorFindService.h"

QVector<EditorFindMatch> EditorFindService::findAll(
    const QString &text,
    const QString &query,
    Qt::CaseSensitivity caseSensitivity)
{
    QVector<EditorFindMatch> matches;
    if (query.isEmpty()) {
        return matches;
    }

    int start = 0;
    while (start <= text.size()) {
        const int index = text.indexOf(query, start, caseSensitivity);
        if (index < 0) {
            break;
        }
        matches.push_back({index, query.size()});
        start = index + qMax(1, query.size());
    }
    return matches;
}

bool EditorFindService::replaceAt(QString *text, const EditorFindMatch &match, const QString &replacement)
{
    if (!text || match.start < 0 || match.length < 0 || match.start + match.length > text->size()) {
        return false;
    }

    text->replace(match.start, match.length, replacement);
    return true;
}

QString EditorFindService::replaceAll(
    const QString &text,
    const QString &query,
    const QString &replacement,
    int *replacedCount,
    Qt::CaseSensitivity caseSensitivity)
{
    if (replacedCount) {
        *replacedCount = 0;
    }
    if (query.isEmpty()) {
        return text;
    }

    QString result;
    int cursor = 0;
    const auto matches = findAll(text, query, caseSensitivity);
    for (const EditorFindMatch &match : matches) {
        result += text.mid(cursor, match.start - cursor);
        result += replacement;
        cursor = match.start + match.length;
    }
    result += text.mid(cursor);
    if (replacedCount) {
        *replacedCount = matches.size();
    }
    return result;
}
```

Add files to `acs_core` in `CMakeLists.txt`.

- [ ] **Step 4: Validate and commit**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git add CMakeLists.txt src\EditorFindService.h src\EditorFindService.cpp tests\TestProjectSearchRuntime.cpp
git commit -m "feat: add editor find service"
```

Expected: validation passes.

---

## Task 2: Editor Surface Find/Replace

**Files:**

- Modify: `src/EditorSurface.h`
- Modify: `src/EditorSurface.cpp`
- Test: `tests/TestEditorSurface.cpp`

- [ ] **Step 1: Write failing editor tests**

Add private slots:

```cpp
void findNavigationSelectsArabicMatchesAndHighlightsAll();
void replaceCurrentAndAllUseActiveFindMatches();
```

Add tests:

```cpp
void TestEditorSurface::findNavigationSelectsArabicMatchesAndHighlightsAll()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    QCOMPARE(editor.setFindQuery(QString::fromUtf8("عدد")), 2);
    QCOMPARE(editor.findMatchCount(), 2);
    QCOMPARE(editor.currentFindMatchIndex(), 0);
    QCOMPARE(editor.textCursor().selectedText(), QString::fromUtf8("عدد"));
    QVERIFY(editor.findHighlightSelectionCountForTest() >= 2);

    QVERIFY(editor.selectNextFindMatch());
    QCOMPARE(editor.currentFindMatchIndex(), 1);
    QCOMPARE(editor.textCursor().selectedText(), QString::fromUtf8("عدد"));
}

void TestEditorSurface::replaceCurrentAndAllUseActiveFindMatches()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    QCOMPARE(editor.setFindQuery(QString::fromUtf8("عدد")), 2);
    QVERIFY(editor.replaceCurrentFindMatch(QString::fromUtf8("قيمة")));
    QCOMPARE(editor.toPlainText(), QString::fromUtf8("قيمة = 1\nاطبع(عدد)\n"));
    QCOMPARE(editor.findMatchCount(), 1);

    QCOMPARE(editor.replaceAllFindMatches(QString::fromUtf8("قيمة")), 1);
    QCOMPARE(editor.toPlainText(), QString::fromUtf8("قيمة = 1\nاطبع(قيمة)\n"));
    QCOMPARE(editor.findMatchCount(), 0);
}
```

- [ ] **Step 2: Run focused test and verify red**

Run `acs_editor_tests`.

Expected: compile failure because methods are not declared.

- [ ] **Step 3: Implement editor methods**

Add to `EditorSurface.h` public methods:

```cpp
int setFindQuery(const QString &query);
QString findQuery() const;
int findMatchCount() const;
int currentFindMatchIndex() const;
bool selectNextFindMatch();
bool selectPreviousFindMatch();
bool replaceCurrentFindMatch(const QString &replacement);
int replaceAllFindMatches(const QString &replacement);
int findHighlightSelectionCountForTest() const;
```

Add private members:

```cpp
QString activeFindQuery;
QVector<EditorFindMatch> activeFindMatches;
int activeFindIndex = -1;
int findHighlightSelectionCount = 0;

void refreshFindMatches(bool selectFirst = true);
void updateFindExtraSelections();
bool selectFindMatch(int index);
```

Implement in `EditorSurface.cpp` using `EditorFindService`, `QTextEdit::ExtraSelection`, and `QTextCursor`.

- [ ] **Step 4: Validate and commit**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git add src\EditorSurface.h src\EditorSurface.cpp tests\TestEditorSurface.cpp
git commit -m "feat: add editor find replace behavior"
```

Expected: validation passes.

---

## Task 3: MainWindow Find/Replace Panel And Commands

**Files:**

- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
- Test: `tests/TestMainWindow.cpp`

- [ ] **Step 1: Write failing integration tests**

Add private slots:

```cpp
void commandPaletteIncludesInFileFindCommand();
void inFileFindPanelNavigatesAndReplacesActiveEditor();
```

Add tests that:

- open command palette and assert `find-in-file` exists;
- invoke `openInFileFind`;
- enter `عدد` into `inFileFindInput`;
- verify status label says `2`;
- click `inFileFindNextButton`;
- enter replacement `قيمة`;
- click `inFileReplaceButton`;
- verify only the selected match changes;
- click `inFileReplaceAllButton`;
- verify remaining matches change.

- [ ] **Step 2: Run focused test and verify red**

Run `acs_main_window_tests`.

Expected: missing command/panel assertions fail.

- [ ] **Step 3: Implement panel and commands**

Add private slots:

```cpp
void openInFileFind();
void updateInFileFindMatches();
void selectNextInFileMatch();
void selectPreviousInFileMatch();
void replaceCurrentInFileMatch();
void replaceAllInFileMatches();
```

Add members:

```cpp
QWidget *inFileFindPanel = nullptr;
QLineEdit *inFileFindInput = nullptr;
QLineEdit *inFileReplaceInput = nullptr;
QLabel *inFileFindStatusLabel = nullptr;
```

Build a compact panel under the editor tabs in `buildUi()`. Register command ID `find-in-file` with shortcut `Ctrl+F`. Wire buttons with object names:

- `inFileFindPanel`
- `inFileFindInput`
- `inFileReplaceInput`
- `inFileFindNextButton`
- `inFileFindPreviousButton`
- `inFileReplaceButton`
- `inFileReplaceAllButton`
- `inFileFindStatusLabel`

- [ ] **Step 4: Validate and commit**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git add src\MainWindow.h src\MainWindow.cpp tests\TestMainWindow.cpp
git commit -m "feat: add in-file find replace panel"
```

Expected: validation passes.

---

## Final Verification

- [ ] Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git status --short
```

Expected: all tests pass, no whitespace errors, clean working tree after commits.

- [ ] Do not run GUI/MSI validation on active `WHITEDRAGON`.
