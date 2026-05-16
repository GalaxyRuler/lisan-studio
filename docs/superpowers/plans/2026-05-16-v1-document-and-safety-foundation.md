# V1 Document And Safety Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a document/session safety layer before find/replace so V1 editing features cannot silently lose unsaved or externally modified work.

**Architecture:** Introduce document services in `acs_core` and migrate durable file IO out of `EditorSurface` in small steps. `MainWindow` remains the Qt shell owner, but save, close, run, project-switch, and later replace workflows must route through a document registry and unsaved-change guard. The first implementation is intentionally conservative: one editor widget still edits one document, but document identity, dirty state, file metadata, atomic writes, and external-change detection live outside the widget.

**Tech Stack:** C++17, Qt 6 Core/Widgets/Test, CMake, Ninja, PowerShell validation.

---

## Scope

This plan implements the prerequisite safety layer requested by the external plan review. It must land before `v1-in-file-find-replace`.

Included:

- `DocumentRegistry` for document IDs, paths, text, dirty state, file identity, encoding, line ending, and external-change metadata.
- `DocumentFileIO` for UTF-8 read and atomic write-temp-then-rename saves.
- `UnsavedChangesGuard` as the non-UI decision engine for save/discard/cancel prompts.
- Minimal file state refresh using filesystem metadata; full asynchronous watcher expansion remains later in V1.
- Initial command IDs for document lifecycle commands.
- `EditorSurface` migration so file read/write policy is no longer owned by the widget.
- `MainWindow` integration tests for close, run, and project switch safety paths.
- A named editor torture test target for mixed-direction editing risks before multi-cursor or column-selection scope is accepted.

Excluded:

- In-file find/replace.
- Project-wide replace.
- Full session restore.
- Full workspace trust UI.
- Terminal execution.
- Language service diagnostics.

## File Structure

- Create: `src/DocumentRegistry.h`
- Create: `src/DocumentRegistry.cpp`
  - Owns document metadata and text snapshots.
  - Provides stable `DocumentId` values.
  - Detects duplicate opens by normalized path.
  - Tracks last-known file identity from size and mtime.
  - Reports external modified/deleted state.
- Create: `src/DocumentFileIO.h`
- Create: `src/DocumentFileIO.cpp`
  - Reads UTF-8 text.
  - Detects BOM/UTF-8 policy.
  - Writes atomically by writing a sibling temp file and renaming it into place.
- Create: `src/UnsavedChangesGuard.h`
- Create: `src/UnsavedChangesGuard.cpp`
  - Pure decision layer for Save/Discard/Cancel outcomes.
  - Does not show Qt dialogs.
- Modify: `src/EditorSurface.h`
- Modify: `src/EditorSurface.cpp`
  - Keep the `QPlainTextEdit` behavior.
  - Replace direct file read/write with document IO helpers.
- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
  - Add `DocumentRegistry`.
  - Route save, close, run materialization, and project switch through safety checks.
  - Keep existing object names and visible RTL behavior.
- Modify: `src/CommandRegistry.cpp`
  - Add document command IDs.
- Modify: `CMakeLists.txt`
  - Add new core files and the `acs_editor_torture_tests` target in Task 7.
- Modify: `tests/TestProjectSearchRuntime.cpp`
  - Add document service tests if keeping service tests consolidated.
- Modify: `tests/TestMainWindow.cpp`
  - Add visible safety integration tests.

---

## Task 1: Document File Identity And Atomic UTF-8 IO

**Files:**

- Create: `src/DocumentFileIO.h`
- Create: `src/DocumentFileIO.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/TestProjectSearchRuntime.cpp`

- [ ] **Step 1: Write the failing service tests**

Add the include:

```cpp
#include "DocumentFileIO.h"
```

Add private slots:

```cpp
void documentFileIoReadsUtf8AndRecordsIdentity();
void documentFileIoWritesAtomicallyAndPreservesUtf8();
void documentFileIoRejectsInvalidUtf8ByPolicy();
```

Add tests:

```cpp
void TestProjectSearchRuntime::documentFileIoReadsUtf8AndRecordsIdentity()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = writeFile(QDir(temp.path()), QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    QString error;
    const DocumentLoadResult result = DocumentFileIO::loadUtf8(path, &error);

    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.text, QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));
    QCOMPARE(result.identity.path, QFileInfo(path).absoluteFilePath());
    QVERIFY(result.identity.sizeBytes > 0);
    QVERIFY(result.identity.lastModifiedUtc.isValid());
    QCOMPARE(result.encoding, DocumentEncoding::Utf8);
    QCOMPARE(result.lineEnding, DocumentLineEnding::Lf);
}

void TestProjectSearchRuntime::documentFileIoWritesAtomicallyAndPreservesUtf8()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = QDir(temp.path()).filePath(QStringLiteral("main.apy"));

    QString error;
    const bool saved = DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("اطبع(\"مرحبا\")\n"), &error);

    QVERIFY2(saved, qPrintable(error));
    QVERIFY(QFileInfo::exists(path));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(file.readAll()), QString::fromUtf8("اطبع(\"مرحبا\")\n"));
    QVERIFY(QDir(temp.path()).entryList(QStringList(QStringLiteral("*.tmp")), QDir::Files).isEmpty());
}

void TestProjectSearchRuntime::documentFileIoRejectsInvalidUtf8ByPolicy()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = QDir(temp.path()).filePath(QStringLiteral("bad.apy"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray::fromHex("fffe4100"));
    file.close();

    QString error;
    const DocumentLoadResult result = DocumentFileIO::loadUtf8(path, &error);

    QVERIFY(result.text.isEmpty());
    QVERIFY(error.contains(QString::fromUtf8("UTF-8")));
}
```

- [ ] **Step 2: Run the focused test and verify it fails**

Run:

```powershell
$ErrorActionPreference='Stop'
$bashPath = 'C:\msys64\usr\bin\bash.exe'
$worktreePath = (Resolve-Path -LiteralPath '.').Path
$drive = $worktreePath.Substring(0, 1).ToLowerInvariant()
$rest = $worktreePath.Substring(2).Replace('\', '/')
$worktreeUnix = "/$drive$rest"
& $bashPath -lc "set -euo pipefail; export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH; cd '$worktreeUnix'; cmake --build build --target acs_project_runtime_tests; ./build/acs_project_runtime_tests.exe"
```

Expected: compile failure because `DocumentFileIO.h` does not exist.

- [ ] **Step 3: Add `DocumentFileIO` types**

Create `src/DocumentFileIO.h`:

```cpp
#pragma once

#include <QDateTime>
#include <QString>

enum class DocumentEncoding
{
    Utf8
};

enum class DocumentLineEnding
{
    Lf,
    Crlf,
    Mixed,
    None
};

struct DocumentFileIdentity
{
    QString path;
    qint64 sizeBytes = -1;
    QDateTime lastModifiedUtc;
};

struct DocumentLoadResult
{
    QString text;
    DocumentFileIdentity identity;
    DocumentEncoding encoding = DocumentEncoding::Utf8;
    DocumentLineEnding lineEnding = DocumentLineEnding::None;
};

class DocumentFileIO final
{
public:
    static DocumentLoadResult loadUtf8(const QString &path, QString *error = nullptr);
    static bool saveUtf8Atomically(const QString &path, const QString &text, QString *error = nullptr);
    static DocumentFileIdentity identityForPath(const QString &path);
    static DocumentLineEnding detectLineEnding(const QString &text);
};
```

- [ ] **Step 4: Implement minimal UTF-8 load and atomic save**

Create `src/DocumentFileIO.cpp`:

```cpp
#include "DocumentFileIO.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringDecoder>

DocumentLoadResult DocumentFileIO::loadUtf8(const QString &path, QString *error)
{
    if (error) {
        error->clear();
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return {};
    }

    const QByteArray bytes = file.readAll();
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString text = decoder.decode(bytes);
    if (decoder.hasError()) {
        if (error) {
            *error = QString::fromUtf8("الملف ليس UTF-8 صالحا: %1").arg(QDir::toNativeSeparators(path));
        }
        return {};
    }

    DocumentLoadResult result;
    result.text = text;
    result.identity = identityForPath(path);
    result.lineEnding = detectLineEnding(text);
    return result;
}

bool DocumentFileIO::saveUtf8Atomically(const QString &path, const QString &text, QString *error)
{
    if (error) {
        error->clear();
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(text.toUtf8());
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

DocumentFileIdentity DocumentFileIO::identityForPath(const QString &path)
{
    const QFileInfo info(path);
    DocumentFileIdentity identity;
    identity.path = info.absoluteFilePath();
    identity.sizeBytes = info.exists() ? info.size() : -1;
    identity.lastModifiedUtc = info.exists() ? info.lastModified().toUTC() : QDateTime();
    return identity;
}

DocumentLineEnding DocumentFileIO::detectLineEnding(const QString &text)
{
    const bool hasCrlf = text.contains(QStringLiteral("\r\n"));
    QString withoutCrlf = text;
    withoutCrlf.replace(QStringLiteral("\r\n"), QString());
    const bool hasLf = withoutCrlf.contains(QLatin1Char('\n'));
    if (hasCrlf && hasLf) {
        return DocumentLineEnding::Mixed;
    }
    if (hasCrlf) {
        return DocumentLineEnding::Crlf;
    }
    if (hasLf) {
        return DocumentLineEnding::Lf;
    }
    return DocumentLineEnding::None;
}
```

- [ ] **Step 5: Add files to CMake**

In `CMakeLists.txt`, add to `acs_core`:

```cmake
    src/DocumentFileIO.h
    src/DocumentFileIO.cpp
```

- [ ] **Step 6: Run the focused test and verify it passes**

Run the same `acs_project_runtime_tests` command from Step 2.

Expected: tests pass.

- [ ] **Step 7: Run full validation and commit**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git add CMakeLists.txt src\DocumentFileIO.h src\DocumentFileIO.cpp tests\TestProjectSearchRuntime.cpp
git commit -m "feat: add document file io safety"
```

Expected: `5/5` tests pass, no diff-check errors, commit succeeds.

---

## Task 2: Document Registry Metadata

**Files:**

- Create: `src/DocumentRegistry.h`
- Create: `src/DocumentRegistry.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/TestWorkbenchState.cpp` or create `tests/TestDocumentSafety.cpp`

- [ ] **Step 1: Write failing registry tests**

If keeping tests in `tests/TestWorkbenchState.cpp`, add:

```cpp
#include "DocumentRegistry.h"
```

Add private slots:

```cpp
void documentRegistryOpensTracksAndFindsDocuments();
void documentRegistryDetectsExternalModifyAndDelete();
```

Add tests:

```cpp
void TestWorkbenchState::documentRegistryOpensTracksAndFindsDocuments()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = QDir(temp.path()).filePath(QStringLiteral("main.apy"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("عدد = 1\n")));

    DocumentRegistry registry;
    QString error;
    const DocumentId id = registry.openPath(path, &error);

    QVERIFY2(id.isValid(), qPrintable(error));
    QCOMPARE(registry.documentCount(), 1);
    QCOMPARE(registry.document(id).path, QFileInfo(path).absoluteFilePath());
    QCOMPARE(registry.document(id).text, QString::fromUtf8("عدد = 1\n"));
    QVERIFY(!registry.document(id).dirty);
    QCOMPARE(registry.findByPath(path), id);
    QCOMPARE(registry.openPath(path, &error), id);
    QCOMPARE(registry.documentCount(), 1);

    registry.setText(id, QString::fromUtf8("عدد = 2\n"));
    QVERIFY(registry.document(id).dirty);
}

void TestWorkbenchState::documentRegistryDetectsExternalModifyAndDelete()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = QDir(temp.path()).filePath(QStringLiteral("main.apy"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("عدد = 1\n")));

    DocumentRegistry registry;
    QString error;
    const DocumentId id = registry.openPath(path, &error);
    QVERIFY2(id.isValid(), qPrintable(error));

    QTest::qWait(1100);
    QVERIFY(DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("عدد = 3\n")));
    registry.refreshFileState(id);
    QCOMPARE(registry.document(id).externalState, DocumentExternalState::Modified);

    QVERIFY(QFile::remove(path));
    registry.refreshFileState(id);
    QCOMPARE(registry.document(id).externalState, DocumentExternalState::Deleted);
}
```

- [ ] **Step 2: Run the focused test and verify it fails**

Run:

```powershell
$ErrorActionPreference='Stop'
$bashPath = 'C:\msys64\usr\bin\bash.exe'
$worktreePath = (Resolve-Path -LiteralPath '.').Path
$drive = $worktreePath.Substring(0, 1).ToLowerInvariant()
$rest = $worktreePath.Substring(2).Replace('\', '/')
$worktreeUnix = "/$drive$rest"
& $bashPath -lc "set -euo pipefail; export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH; cd '$worktreeUnix'; cmake --build build --target acs_workbench_state_tests; ./build/acs_workbench_state_tests.exe"
```

Expected: compile failure because `DocumentRegistry.h` does not exist.

- [ ] **Step 3: Add registry interfaces**

Create `src/DocumentRegistry.h`:

```cpp
#pragma once

#include "DocumentFileIO.h"

#include <QString>
#include <QVector>

class DocumentId final
{
public:
    DocumentId() = default;
    explicit DocumentId(int value);
    bool isValid() const;
    int value() const;
    friend bool operator==(DocumentId left, DocumentId right) { return left.raw == right.raw; }
    friend bool operator!=(DocumentId left, DocumentId right) { return !(left == right); }

private:
    int raw = -1;
};

enum class DocumentExternalState
{
    Unchanged,
    Modified,
    Deleted
};

struct DocumentRecord
{
    DocumentId id;
    QString path;
    QString text;
    bool dirty = false;
    DocumentEncoding encoding = DocumentEncoding::Utf8;
    DocumentLineEnding lineEnding = DocumentLineEnding::None;
    DocumentFileIdentity identity;
    DocumentExternalState externalState = DocumentExternalState::Unchanged;
};

class DocumentRegistry final
{
public:
    DocumentId openPath(const QString &path, QString *error = nullptr);
    DocumentId createUntitled(const QString &text = QString());
    bool close(DocumentId id);
    int documentCount() const;
    DocumentRecord document(DocumentId id) const;
    DocumentId findByPath(const QString &path) const;
    void setText(DocumentId id, const QString &text);
    void markClean(DocumentId id);
    void setPathAfterSave(DocumentId id, const QString &path);
    void refreshFileState(DocumentId id);

private:
    QVector<DocumentRecord> records;
    int nextId = 1;

    int indexOf(DocumentId id) const;
};
```

- [ ] **Step 4: Implement minimal registry**

Create `src/DocumentRegistry.cpp`:

```cpp
#include "DocumentRegistry.h"

#include <QFileInfo>

DocumentId::DocumentId(int value)
    : raw(value)
{
}

bool DocumentId::isValid() const
{
    return raw > 0;
}

int DocumentId::value() const
{
    return raw;
}

DocumentId DocumentRegistry::openPath(const QString &path, QString *error)
{
    const QString normalizedPath = QFileInfo(path).absoluteFilePath();
    const DocumentId existing = findByPath(normalizedPath);
    if (existing.isValid()) {
        if (error) {
            error->clear();
        }
        return existing;
    }

    const DocumentLoadResult loaded = DocumentFileIO::loadUtf8(normalizedPath, error);
    if (loaded.text.isEmpty() && error && !error->isEmpty()) {
        return {};
    }

    DocumentRecord record;
    record.id = DocumentId(nextId++);
    record.path = normalizedPath;
    record.text = loaded.text;
    record.dirty = false;
    record.encoding = loaded.encoding;
    record.lineEnding = loaded.lineEnding;
    record.identity = loaded.identity;
    records.push_back(record);
    return record.id;
}

DocumentId DocumentRegistry::createUntitled(const QString &text)
{
    DocumentRecord record;
    record.id = DocumentId(nextId++);
    record.text = text;
    record.dirty = !text.isEmpty();
    records.push_back(record);
    return record.id;
}

bool DocumentRegistry::close(DocumentId id)
{
    const int index = indexOf(id);
    if (index < 0) {
        return false;
    }
    records.remove(index);
    return true;
}

int DocumentRegistry::documentCount() const
{
    return records.size();
}

DocumentRecord DocumentRegistry::document(DocumentId id) const
{
    const int index = indexOf(id);
    return index >= 0 ? records.at(index) : DocumentRecord {};
}

DocumentId DocumentRegistry::findByPath(const QString &path) const
{
    const QString normalizedPath = QFileInfo(path).absoluteFilePath();
    for (const DocumentRecord &record : records) {
        if (!record.path.isEmpty() && QFileInfo(record.path).absoluteFilePath() == normalizedPath) {
            return record.id;
        }
    }
    return {};
}

void DocumentRegistry::setText(DocumentId id, const QString &text)
{
    const int index = indexOf(id);
    if (index < 0) {
        return;
    }
    records[index].text = text;
    records[index].dirty = true;
}

void DocumentRegistry::markClean(DocumentId id)
{
    const int index = indexOf(id);
    if (index >= 0) {
        records[index].dirty = false;
    }
}

void DocumentRegistry::setPathAfterSave(DocumentId id, const QString &path)
{
    const int index = indexOf(id);
    if (index < 0) {
        return;
    }
    records[index].path = QFileInfo(path).absoluteFilePath();
    records[index].identity = DocumentFileIO::identityForPath(records[index].path);
    records[index].externalState = DocumentExternalState::Unchanged;
    records[index].dirty = false;
}

void DocumentRegistry::refreshFileState(DocumentId id)
{
    const int index = indexOf(id);
    if (index < 0 || records[index].path.isEmpty()) {
        return;
    }

    const DocumentFileIdentity current = DocumentFileIO::identityForPath(records[index].path);
    if (current.sizeBytes < 0) {
        records[index].externalState = DocumentExternalState::Deleted;
    } else if (current.sizeBytes != records[index].identity.sizeBytes
        || current.lastModifiedUtc != records[index].identity.lastModifiedUtc) {
        records[index].externalState = DocumentExternalState::Modified;
    } else {
        records[index].externalState = DocumentExternalState::Unchanged;
    }
}

int DocumentRegistry::indexOf(DocumentId id) const
{
    for (int i = 0; i < records.size(); ++i) {
        if (records.at(i).id == id) {
            return i;
        }
    }
    return -1;
}
```

- [ ] **Step 5: Add files to CMake**

Add:

```cmake
    src/DocumentRegistry.h
    src/DocumentRegistry.cpp
```

- [ ] **Step 6: Run focused and full validation, then commit**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git add CMakeLists.txt src\DocumentRegistry.h src\DocumentRegistry.cpp tests\TestWorkbenchState.cpp
git commit -m "feat: add document registry"
```

Expected: all tests pass, commit succeeds.

---

## Task 3: Unsaved Changes Guard

**Files:**

- Create: `src/UnsavedChangesGuard.h`
- Create: `src/UnsavedChangesGuard.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/TestWorkbenchState.cpp`

- [ ] **Step 1: Write failing guard tests**

Add include:

```cpp
#include "UnsavedChangesGuard.h"
```

Add private slot:

```cpp
void unsavedChangesGuardRequiresSaveDiscardOrCancelForDirtyDocuments();
```

Add test:

```cpp
void TestWorkbenchState::unsavedChangesGuardRequiresSaveDiscardOrCancelForDirtyDocuments()
{
    DocumentRecord clean;
    clean.id = DocumentId(1);
    clean.path = QStringLiteral("C:/project/clean.apy");
    clean.dirty = false;

    DocumentRecord dirty;
    dirty.id = DocumentId(2);
    dirty.path = QStringLiteral("C:/project/dirty.apy");
    dirty.dirty = true;

    const UnsavedChangesRequest none = UnsavedChangesGuard::requestFor({clean}, UnsavedChangesOperation::CloseDocument);
    QCOMPARE(none.required, false);

    const UnsavedChangesRequest request = UnsavedChangesGuard::requestFor({clean, dirty}, UnsavedChangesOperation::ProjectSwitch);
    QVERIFY(request.required);
    QCOMPARE(request.operation, UnsavedChangesOperation::ProjectSwitch);
    QCOMPARE(request.dirtyDocuments.size(), 1);
    QCOMPARE(request.dirtyDocuments.first().id, dirty.id);
    QVERIFY(request.message.contains(QString::fromUtf8("dirty.apy")));

    QCOMPARE(UnsavedChangesGuard::allowsOperation(UnsavedChangesChoice::Cancel), false);
    QCOMPARE(UnsavedChangesGuard::allowsOperation(UnsavedChangesChoice::Discard), true);
    QCOMPARE(UnsavedChangesGuard::allowsOperation(UnsavedChangesChoice::Save), true);
}
```

- [ ] **Step 2: Run focused test and verify it fails**

Run `acs_workbench_state_tests`.

Expected: compile failure because `UnsavedChangesGuard.h` does not exist.

- [ ] **Step 3: Add guard interfaces**

Create `src/UnsavedChangesGuard.h`:

```cpp
#pragma once

#include "DocumentRegistry.h"

#include <QString>
#include <QVector>

enum class UnsavedChangesOperation
{
    CloseDocument,
    OpenFile,
    ProjectSwitch,
    Run,
    Exit
};

enum class UnsavedChangesChoice
{
    Save,
    Discard,
    Cancel
};

struct UnsavedChangesRequest
{
    bool required = false;
    UnsavedChangesOperation operation = UnsavedChangesOperation::CloseDocument;
    QVector<DocumentRecord> dirtyDocuments;
    QString title;
    QString message;
};

class UnsavedChangesGuard final
{
public:
    static UnsavedChangesRequest requestFor(
        const QVector<DocumentRecord> &documents,
        UnsavedChangesOperation operation);
    static bool allowsOperation(UnsavedChangesChoice choice);
};
```

- [ ] **Step 4: Implement the guard**

Create `src/UnsavedChangesGuard.cpp`:

```cpp
#include "UnsavedChangesGuard.h"

#include <QFileInfo>

namespace {
QString operationTitle(UnsavedChangesOperation operation)
{
    switch (operation) {
    case UnsavedChangesOperation::CloseDocument:
        return QString::fromUtf8("حفظ التغييرات");
    case UnsavedChangesOperation::OpenFile:
        return QString::fromUtf8("حفظ قبل فتح ملف");
    case UnsavedChangesOperation::ProjectSwitch:
        return QString::fromUtf8("حفظ قبل تغيير المشروع");
    case UnsavedChangesOperation::Run:
        return QString::fromUtf8("حفظ قبل التشغيل");
    case UnsavedChangesOperation::Exit:
        return QString::fromUtf8("حفظ قبل الخروج");
    }
    return QString::fromUtf8("حفظ التغييرات");
}

QString displayName(const DocumentRecord &document)
{
    if (!document.path.isEmpty()) {
        return QFileInfo(document.path).fileName();
    }
    return QString::fromUtf8("ملف جديد");
}
}

UnsavedChangesRequest UnsavedChangesGuard::requestFor(
    const QVector<DocumentRecord> &documents,
    UnsavedChangesOperation operation)
{
    UnsavedChangesRequest request;
    request.operation = operation;
    request.title = operationTitle(operation);
    for (const DocumentRecord &document : documents) {
        if (document.dirty) {
            request.dirtyDocuments.push_back(document);
        }
    }
    request.required = !request.dirtyDocuments.isEmpty();
    if (request.required) {
        QStringList names;
        for (const DocumentRecord &document : request.dirtyDocuments) {
            names.append(displayName(document));
        }
        request.message = QString::fromUtf8("هناك تغييرات غير محفوظة في: %1").arg(names.join(QStringLiteral(", ")));
    }
    return request;
}

bool UnsavedChangesGuard::allowsOperation(UnsavedChangesChoice choice)
{
    return choice == UnsavedChangesChoice::Save || choice == UnsavedChangesChoice::Discard;
}
```

- [ ] **Step 5: Add files to CMake and validate**

Add to `acs_core`:

```cmake
    src/UnsavedChangesGuard.h
    src/UnsavedChangesGuard.cpp
```

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git add CMakeLists.txt src\UnsavedChangesGuard.h src\UnsavedChangesGuard.cpp tests\TestWorkbenchState.cpp
git commit -m "feat: add unsaved changes guard"
```

Expected: full validation passes and commit succeeds.

---

## Task 4: Migrate EditorSurface File IO To DocumentFileIO

**Files:**

- Modify: `src/EditorSurface.cpp`
- Test: `tests/TestEditorSurface.cpp`

- [ ] **Step 1: Add failing atomic-save behavior test at editor surface level**

Add private slot:

```cpp
void saveUsesDocumentIoAndRejectsInvalidTarget();
```

Add test:

```cpp
void TestEditorSurface::saveUsesDocumentIoAndRejectsInvalidTarget()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = QDir(temp.path()).filePath(QStringLiteral("main.apy"));

    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("اطبع(\"مرحبا\")\n"));
    QVERIFY(editor.saveFileAs(path));
    QVERIFY(!editor.isDirty());
    QVERIFY(QDir(temp.path()).entryList(QStringList(QStringLiteral("*.tmp")), QDir::Files).isEmpty());

    QString error;
    QVERIFY(!editor.saveFileAs(QDir(temp.path()).filePath(QStringLiteral("missing-dir/main.apy")), &error));
    QVERIFY(!error.isEmpty());
}
```

- [ ] **Step 2: Run focused editor test**

Run:

```powershell
$ErrorActionPreference='Stop'
$bashPath = 'C:\msys64\usr\bin\bash.exe'
$worktreePath = (Resolve-Path -LiteralPath '.').Path
$drive = $worktreePath.Substring(0, 1).ToLowerInvariant()
$rest = $worktreePath.Substring(2).Replace('\', '/')
$worktreeUnix = "/$drive$rest"
& $bashPath -lc "set -euo pipefail; export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH; cd '$worktreeUnix'; cmake --build build --target acs_editor_tests; QT_QPA_PLATFORM=offscreen ./build/acs_editor_tests.exe"
```

Expected before migration: the valid save may pass, but this test does not prove `DocumentFileIO` is used. The implementation step still replaces direct `QFile` write with `DocumentFileIO`.

- [ ] **Step 3: Replace direct editor file IO**

In `src/EditorSurface.cpp`, include:

```cpp
#include "DocumentFileIO.h"
```

Replace `openFile()` body with:

```cpp
bool EditorSurface::openFile(const QString &path, QString *error)
{
    const DocumentLoadResult loaded = DocumentFileIO::loadUtf8(path, error);
    if (loaded.text.isEmpty() && error && !error->isEmpty()) {
        return false;
    }

    setPlainText(loaded.text);
    document()->setModified(false);
    setCurrentFilePath(loaded.identity.path);
    return true;
}
```

Replace `saveFileAs()` write logic with:

```cpp
bool EditorSurface::saveFileAs(const QString &path, QString *error)
{
    if (!DocumentFileIO::saveUtf8Atomically(path, toPlainText(), error)) {
        return false;
    }
    document()->setModified(false);
    setCurrentFilePath(DocumentFileIO::identityForPath(path).path);
    return true;
}
```

- [ ] **Step 4: Validate and commit**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git add src\EditorSurface.cpp tests\TestEditorSurface.cpp
git commit -m "refactor: route editor file io through document service"
```

Expected: full validation passes and commit succeeds.

---

## Task 5: MainWindow Guard Integration

**Files:**

- Modify: `src/MainWindow.h`
- Modify: `src/MainWindow.cpp`
- Test: `tests/TestMainWindow.cpp`

- [ ] **Step 1: Add integration tests for guard coverage**

Add private slots:

```cpp
void dirtyBufferCancelPreventsNewFile();
void dirtyBufferCancelPreventsProjectSwitch();
void runCurrentDirtySavedFileSavesBeforeRuntime();
```

The tests should use `QTimer::singleShot` to interact with modal dialogs:

```cpp
void TestMainWindow::dirtyBufferCancelPreventsNewFile()
{
    MainWindow window;
    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("عدد = 1\n"));
    QVERIFY(editor->isDirty());

    QTimer::singleShot(0, []() {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box != nullptr);
        box->button(QMessageBox::Cancel)->click();
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("editorTabs"));
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->count(), 1);
    QVERIFY(editor->toPlainText().contains(QString::fromUtf8("عدد")));
}

void TestMainWindow::dirtyBufferCancelPreventsProjectSwitch()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QTemporaryDir other;
    QVERIFY(other.isValid());

    MainWindow window;
    QVERIFY(window.openPath(temp.path()));
    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("عدد = 1\n"));

    QTimer::singleShot(0, []() {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box != nullptr);
        box->button(QMessageBox::Cancel)->click();
    });

    QVERIFY(!window.openPath(other.path()));
    QCOMPARE(QDir::toNativeSeparators(window.currentProjectRoot()), QDir::toNativeSeparators(temp.path()));
}
```

For `runCurrentDirtySavedFileSavesBeforeRuntime()`, create a temporary `.apy`, open it, modify it, invoke run, and assert the file contents were updated before command launch output appears. If the runtime is unavailable, assert the saved file changed and output contains the launch attempt or missing-runtime message.

- [ ] **Step 2: Run main-window tests and verify failure**

Run:

```powershell
$ErrorActionPreference='Stop'
$bashPath = 'C:\msys64\usr\bin\bash.exe'
$worktreePath = (Resolve-Path -LiteralPath '.').Path
$drive = $worktreePath.Substring(0, 1).ToLowerInvariant()
$rest = $worktreePath.Substring(2).Replace('\', '/')
$worktreeUnix = "/$drive$rest"
& $bashPath -lc "set -euo pipefail; export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH; cd '$worktreeUnix'; cmake --build build --target acs_main_window_tests; QT_QPA_PLATFORM=offscreen ./build/acs_main_window_tests.exe"
```

Expected: project-switch guard test fails because `openPath()` and `loadProject()` currently bypass `confirmSaveIfDirty()`.

- [ ] **Step 3: Add document records from open editors**

Add to `MainWindow.h` private methods:

```cpp
QVector<DocumentRecord> openDocumentRecords() const;
bool confirmUnsavedDocuments(UnsavedChangesOperation operation);
```

Implement in `MainWindow.cpp`:

```cpp
QVector<DocumentRecord> MainWindow::openDocumentRecords() const
{
    QVector<DocumentRecord> records;
    if (!editorTabs) {
        return records;
    }
    for (int i = 0; i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (!surface) {
            continue;
        }
        DocumentRecord record;
        record.id = DocumentId(i + 1);
        record.path = surface->currentFilePath();
        record.text = surface->toPlainText();
        record.dirty = surface->isDirty();
        record.identity = record.path.isEmpty() ? DocumentFileIdentity {} : DocumentFileIO::identityForPath(record.path);
        records.push_back(record);
    }
    return records;
}

bool MainWindow::confirmUnsavedDocuments(UnsavedChangesOperation operation)
{
    const UnsavedChangesRequest request = UnsavedChangesGuard::requestFor(openDocumentRecords(), operation);
    if (!request.required) {
        return true;
    }

    const auto answer = QMessageBox::question(
        this,
        request.title,
        request.message,
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (answer == QMessageBox::Cancel) {
        return false;
    }
    if (answer == QMessageBox::Save) {
        saveFile();
        return editor && !editor->isDirty();
    }
    return true;
}
```

Change `confirmSaveIfDirty()` to delegate:

```cpp
bool MainWindow::confirmSaveIfDirty()
{
    return confirmUnsavedDocuments(UnsavedChangesOperation::CloseDocument);
}
```

At the beginning of `loadProject()` add:

```cpp
    if (!confirmUnsavedDocuments(UnsavedChangesOperation::ProjectSwitch)) {
        return false;
    }
```

Before materializing a saved dirty file in `materializeRunnableBuffer()`, keep the current save behavior but ensure it is reachable through document IO after Task 4.

- [ ] **Step 4: Run main-window and full validation**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git add src\MainWindow.h src\MainWindow.cpp tests\TestMainWindow.cpp
git commit -m "feat: guard dirty documents in workbench flows"
```

Expected: full validation passes and commit succeeds.

---

## Task 6: Document Lifecycle Command IDs

**Files:**

- Modify: `src\CommandRegistry.cpp`
- Modify: `tests\TestCommandRegistry.cpp`
- Modify: `tests\TestMainWindow.cpp`

- [ ] **Step 1: Add command registry tests**

In `tests/TestCommandRegistry.cpp`, add expected command IDs:

```cpp
const QStringList expectedIds = {
    QStringLiteral("document.save"),
    QStringLiteral("document.saveAll"),
    QStringLiteral("document.revert"),
    QStringLiteral("document.closeWithPrompt"),
};
for (const QString &id : expectedIds) {
    QVERIFY2(registry.command(id) != nullptr, qPrintable(id));
}
```

- [ ] **Step 2: Run command registry tests and verify failure**

Run:

```powershell
$ErrorActionPreference='Stop'
$bashPath = 'C:\msys64\usr\bin\bash.exe'
$worktreePath = (Resolve-Path -LiteralPath '.').Path
$drive = $worktreePath.Substring(0, 1).ToLowerInvariant()
$rest = $worktreePath.Substring(2).Replace('\', '/')
$worktreeUnix = "/$drive$rest"
& $bashPath -lc "set -euo pipefail; export PATH=/ucrt64/bin:/usr/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:`$PATH; cd '$worktreeUnix'; cmake --build build --target acs_command_registry_tests; ./build/acs_command_registry_tests.exe"
```

Expected: missing command ID assertions fail.

- [ ] **Step 3: Register document lifecycle commands**

In `MainWindow::registerWorkbenchCommands()`, add:

```cpp
    commandRegistry.registerCommand({
        QStringLiteral("document.save"),
        QString::fromUtf8("حفظ المستند"),
        QString::fromUtf8("المستند"),
        QKeySequence::Save,
        QString::fromUtf8("حفظ المستند الحالي"),
        [this]() { saveFile(); },
        [this]() { return editor != nullptr; },
    });
    commandRegistry.registerCommand({
        QStringLiteral("document.saveAll"),
        QString::fromUtf8("حفظ كل المستندات"),
        QString::fromUtf8("المستند"),
        QKeySequence(),
        QString::fromUtf8("حفظ كل المستندات المفتوحة"),
        [this]() { saveFile(); },
        [this]() { return editorTabs && editorTabs->count() > 0; },
    });
    commandRegistry.registerCommand({
        QStringLiteral("document.revert"),
        QString::fromUtf8("إعادة تحميل المستند"),
        QString::fromUtf8("المستند"),
        QKeySequence(),
        QString::fromUtf8("إعادة تحميل المستند الحالي من القرص"),
        [this]() {
            if (editor && !editor->currentFilePath().isEmpty()) {
                openEditorFile(editor->currentFilePath());
            }
        },
        [this]() { return editor && !editor->currentFilePath().isEmpty(); },
    });
    commandRegistry.registerCommand({
        QStringLiteral("document.closeWithPrompt"),
        QString::fromUtf8("إغلاق المستند"),
        QString::fromUtf8("المستند"),
        QKeySequence::Close,
        QString::fromUtf8("إغلاق المستند الحالي مع حماية التغييرات"),
        [this]() {
            if (editorTabs) {
                closeEditorTab(editorTabs->currentIndex());
            }
        },
        [this]() { return editorTabs && editorTabs->count() > 0; },
    });
```

If duplicate save IDs conflict with existing `save-file`, keep the old ID for compatibility and add these document IDs as aliases in the registry.

- [ ] **Step 4: Run validation and commit**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
git diff --check
git add src\CommandRegistry.cpp src\MainWindow.cpp tests\TestCommandRegistry.cpp tests\TestMainWindow.cpp
git commit -m "feat: add document lifecycle commands"
```

Expected: full validation passes and commit succeeds.

---

## Task 7: Editor Torture Target And Feasibility Gate

**Files:**

- Modify: `CMakeLists.txt`
- Create: `tests/TestEditorTorture.cpp`
- Modify: `docs\superpowers\plans\2026-05-16-lisan-studio-current-state-and-development-plan.md`

- [ ] **Step 1: Create explicit torture target tests**

Create `tests/TestEditorTorture.cpp`:

```cpp
#include <QtTest/QtTest>

#include "EditorSurface.h"

class TestEditorTorture : public QObject
{
    Q_OBJECT

private slots:
    void mixedDirectionCursorSelectionRemainsLogical();
    void hiddenBidiReplacementDoesNotCreateInvisibleControls();
};

void TestEditorTorture::mixedDirectionCursorSelectionRemainsLogical()
{
    EditorSurface editor;
    const QString text = QString::fromUtf8("نتيجة = call_english(اسم, 123, \"C:/Users/Admin/مشروع/main.apy\")\n");
    editor.setPlainText(text);

    QTextCursor cursor = editor.textCursor();
    for (int position = 0; position <= text.size(); ++position) {
        cursor.setPosition(position);
        editor.setTextCursor(cursor);
        QCOMPARE(editor.textCursor().position(), position);
    }
}

void TestEditorTorture::hiddenBidiReplacementDoesNotCreateInvisibleControls()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("اسم = \"سارة\"\n"));
    QTextCursor cursor = editor.document()->find(QString::fromUtf8("سارة"));
    QVERIFY(!cursor.isNull());
    cursor.insertText(QString::fromUtf8("ليلى"));
    QCOMPARE(editor.findHiddenBidiControls(editor.toPlainText()).size(), 0);
}

QTEST_MAIN(TestEditorTorture)
#include "TestEditorTorture.moc"
```

- [ ] **Step 2: Add CMake target**

Add:

```cmake
qt_add_executable(acs_editor_torture_tests
    tests/TestEditorTorture.cpp
)

target_link_libraries(acs_editor_torture_tests PRIVATE acs_core Qt6::Test)
add_test(NAME acs_editor_torture_tests COMMAND acs_editor_torture_tests)
```

- [ ] **Step 3: Run validation**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected: `6/6` tests pass after the new target is added.

- [ ] **Step 4: Commit**

Run:

```powershell
git diff --check
git add CMakeLists.txt tests\TestEditorTorture.cpp docs\superpowers\plans\2026-05-16-lisan-studio-current-state-and-development-plan.md
git commit -m "test: add editor torture gate"
```

Expected: commit succeeds.

---

## Final Verification For This Plan

- [ ] Run local validation:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected: all Qt tests pass. If Task 7 added the torture target, expected count is `6/6`.

- [ ] Run diff checks:

```powershell
git diff --check
git status --short
```

Expected: no whitespace errors and only intended files changed before each commit.

- [ ] Do not run GUI/MSI validation on `WHITEDRAGON`.

- [ ] Do not run the Homelab `desktopMsiSmoke` lane unless preparing release evidence.

---

## Exit Criteria

- Durable document IO uses UTF-8 policy and atomic saves.
- Documents have registry-owned IDs, paths, dirty state, identity, encoding, line endings, and external state.
- Duplicate opens return the same document record.
- Dirty document operations are guarded before close, open, run, project switch, and app exit paths as each path exists.
- `EditorSurface` no longer owns raw durable read/write policy.
- Document lifecycle commands exist in the command registry.
- The next find/replace plan can target `DocumentRegistry` instead of reaching directly into `EditorSurface` for durable state.
