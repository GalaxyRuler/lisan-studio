#include <QtTest/QtTest>

#include "DocumentChangePoller.h"
#include "DocumentRegistry.h"
#include "UnsavedChangesGuard.h"
#include "WorkbenchState.h"
#include "WorkbenchTheme.h"

#include <algorithm>
#include <QUrl>

class TestWorkbenchState : public QObject
{
    Q_OBJECT

private slots:
    void searchGenerationsAdvanceAndRejectStaleResults();
    void projectRootIsStoredWithQtSeparators();
    void editorSessionsTrackCurrentPathAndDirtyState();
    void editorSessionsFindExistingFilesWithNormalizedPaths();
    void documentRegistryOpensTracksAndFindsDocuments();
    void documentRegistryDetectsExternalModifyAndDelete();
    void documentRegistryRefreshesAllExternalStates();
    void documentRegistryListsExternallyChangedDocuments();
    void documentRegistryReloadsExternalChangesFromDisk();
    void documentRegistryKeepsCurrentTextAfterExternalChanges();
    void documentRegistryAssignsStableUris();
    void documentRegistryVersionsDocumentsForStaleEditDetection();
    void documentRegistryAppliesVersionedTextEdits();
    void documentRegistryAppliesVersionedTextEditBatches();
    void documentRegistryRejectsStaleOrInvalidTextEdits();
    void documentChangePollerReportsRegistryExternalChanges();
    void unsavedChangesGuardRequiresSaveDiscardOrCancelForDirtyDocuments();
    void workbenchThemeProvidesDarkAndLightStyleSheets();
};

void TestWorkbenchState::searchGenerationsAdvanceAndRejectStaleResults()
{
    WorkbenchState state;
    QCOMPARE(state.currentSearchGeneration(), 0);

    const int first = state.nextSearchGeneration();
    QCOMPARE(first, 1);
    QVERIFY(state.isCurrentSearchGeneration(first));

    const int second = state.nextSearchGeneration();
    QCOMPARE(second, 2);
    QVERIFY(state.isCurrentSearchGeneration(second));
    QVERIFY(!state.isCurrentSearchGeneration(first));
}

void TestWorkbenchState::projectRootIsStoredWithQtSeparators()
{
    WorkbenchState state;
    state.setProjectRoot(QStringLiteral("C:\\Users\\Admin\\مشروع"));
    QCOMPARE(state.projectRoot(), QStringLiteral("C:/Users/Admin/مشروع"));

    state.setProjectRoot(QString());
    QCOMPARE(state.projectRoot(), QString());
}

void TestWorkbenchState::editorSessionsTrackCurrentPathAndDirtyState()
{
    WorkbenchState state;

    const int first = state.addEditorSession(QString());
    const int second = state.addEditorSession(QStringLiteral("C:\\Users\\Admin\\مشروع\\main.apy"));

    QCOMPARE(first, 0);
    QCOMPARE(second, 1);
    QCOMPARE(state.editorSessionCount(), 2);
    QCOMPARE(state.currentEditorSessionIndex(), 1);
    QCOMPARE(state.currentEditorPath(), QStringLiteral("C:/Users/Admin/مشروع/main.apy"));

    state.setCurrentEditorSessionIndex(0);
    state.setEditorSessionDirty(0, true);
    QVERIFY(state.isCurrentEditorSessionDirty());
    QCOMPARE(state.currentEditorPath(), QString());

    state.setEditorSessionPath(0, QStringLiteral("C:\\Users\\Admin\\مشروع\\draft.apy"));
    QCOMPARE(state.currentEditorPath(), QStringLiteral("C:/Users/Admin/مشروع/draft.apy"));
}

void TestWorkbenchState::editorSessionsFindExistingFilesWithNormalizedPaths()
{
    WorkbenchState state;
    state.addEditorSession(QStringLiteral("C:\\Users\\Admin\\مشروع\\first.apy"));
    state.addEditorSession(QStringLiteral("C:/Users/Admin/مشروع/second.apy"));

    QCOMPARE(state.findEditorSessionByPath(QStringLiteral("C:/Users/Admin/مشروع/first.apy")), 0);
    QCOMPARE(state.findEditorSessionByPath(QStringLiteral("C:\\Users\\Admin\\مشروع\\second.apy")), 1);
    QCOMPARE(state.findEditorSessionByPath(QStringLiteral("C:/Users/Admin/مشروع/missing.apy")), -1);

    state.removeEditorSession(0);
    QCOMPARE(state.editorSessionCount(), 1);
    QCOMPARE(state.currentEditorSessionIndex(), 0);
    QCOMPARE(state.currentEditorPath(), QStringLiteral("C:/Users/Admin/مشروع/second.apy"));
}

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

void TestWorkbenchState::documentRegistryRefreshesAllExternalStates()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString modifiedPath = QDir(temp.path()).filePath(QStringLiteral("modified.apy"));
    const QString deletedPath = QDir(temp.path()).filePath(QStringLiteral("deleted.apy"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(modifiedPath, QString::fromUtf8("عدد = 1\n")));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(deletedPath, QString::fromUtf8("اسم = \"سارة\"\n")));

    DocumentRegistry registry;
    QString error;
    const DocumentId modifiedId = registry.openPath(modifiedPath, &error);
    QVERIFY2(modifiedId.isValid(), qPrintable(error));
    const DocumentId deletedId = registry.openPath(deletedPath, &error);
    QVERIFY2(deletedId.isValid(), qPrintable(error));
    const DocumentId untitledId = registry.createUntitled(QString::fromUtf8("مسودة\n"));
    QVERIFY(untitledId.isValid());

    QTest::qWait(1100);
    QVERIFY(DocumentFileIO::saveUtf8Atomically(modifiedPath, QString::fromUtf8("عدد = 2\n")));
    QVERIFY(QFile::remove(deletedPath));

    registry.refreshAllFileStates();

    QCOMPARE(registry.document(modifiedId).externalState, DocumentExternalState::Modified);
    QCOMPARE(registry.document(deletedId).externalState, DocumentExternalState::Deleted);
    QCOMPARE(registry.document(untitledId).externalState, DocumentExternalState::Unchanged);
}

void TestWorkbenchState::documentRegistryListsExternallyChangedDocuments()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString unchangedPath = QDir(temp.path()).filePath(QStringLiteral("clean.apy"));
    const QString modifiedPath = QDir(temp.path()).filePath(QStringLiteral("modified.apy"));
    const QString deletedPath = QDir(temp.path()).filePath(QStringLiteral("deleted.apy"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(unchangedPath, QString::fromUtf8("ثابت = 1\n")));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(modifiedPath, QString::fromUtf8("عدد = 1\n")));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(deletedPath, QString::fromUtf8("اسم = \"سارة\"\n")));

    DocumentRegistry registry;
    QString error;
    const DocumentId unchangedId = registry.openPath(unchangedPath, &error);
    QVERIFY2(unchangedId.isValid(), qPrintable(error));
    const DocumentId modifiedId = registry.openPath(modifiedPath, &error);
    QVERIFY2(modifiedId.isValid(), qPrintable(error));
    const DocumentId deletedId = registry.openPath(deletedPath, &error);
    QVERIFY2(deletedId.isValid(), qPrintable(error));
    const DocumentId untitledId = registry.createUntitled(QString::fromUtf8("مسودة\n"));
    QVERIFY(untitledId.isValid());

    QTest::qWait(1100);
    QVERIFY(DocumentFileIO::saveUtf8Atomically(modifiedPath, QString::fromUtf8("عدد = 2\n")));
    QVERIFY(QFile::remove(deletedPath));

    registry.refreshAllFileStates();
    const QVector<DocumentRecord> changed = registry.externallyChangedDocuments();

    QCOMPARE(changed.size(), 2);
    QVERIFY(std::any_of(changed.cbegin(), changed.cend(), [modifiedId](const DocumentRecord &record) {
        return record.id == modifiedId && record.externalState == DocumentExternalState::Modified;
    }));
    QVERIFY(std::any_of(changed.cbegin(), changed.cend(), [deletedId](const DocumentRecord &record) {
        return record.id == deletedId && record.externalState == DocumentExternalState::Deleted;
    }));
    QVERIFY(std::none_of(changed.cbegin(), changed.cend(), [unchangedId, untitledId](const DocumentRecord &record) {
        return record.id == unchangedId || record.id == untitledId;
    }));
}

void TestWorkbenchState::documentRegistryReloadsExternalChangesFromDisk()
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
    QVERIFY(DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("عدد = 2\n")));
    registry.refreshFileState(id);
    QCOMPARE(registry.document(id).externalState, DocumentExternalState::Modified);

    QVERIFY2(registry.reloadFromDisk(id, &error), qPrintable(error));

    const DocumentRecord record = registry.document(id);
    QCOMPARE(record.text, QString::fromUtf8("عدد = 2\n"));
    QVERIFY(!record.dirty);
    QCOMPARE(record.externalState, DocumentExternalState::Unchanged);
    QCOMPARE(record.identity.sizeBytes, DocumentFileIO::identityForPath(path).sizeBytes);
    QCOMPARE(record.identity.lastModifiedUtc, DocumentFileIO::identityForPath(path).lastModifiedUtc);
}

void TestWorkbenchState::documentRegistryKeepsCurrentTextAfterExternalChanges()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString modifiedPath = QDir(temp.path()).filePath(QStringLiteral("modified.apy"));
    const QString deletedPath = QDir(temp.path()).filePath(QStringLiteral("deleted.apy"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(modifiedPath, QString::fromUtf8("عدد = 1\n")));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(deletedPath, QString::fromUtf8("اسم = \"سارة\"\n")));

    DocumentRegistry registry;
    QString error;
    const DocumentId modifiedId = registry.openPath(modifiedPath, &error);
    QVERIFY2(modifiedId.isValid(), qPrintable(error));
    const DocumentId deletedId = registry.openPath(deletedPath, &error);
    QVERIFY2(deletedId.isValid(), qPrintable(error));

    registry.setText(modifiedId, QString::fromUtf8("عدد = 99\n"));
    registry.setText(deletedId, QString::fromUtf8("اسم = \"ليلى\"\n"));

    QTest::qWait(1100);
    QVERIFY(DocumentFileIO::saveUtf8Atomically(modifiedPath, QString::fromUtf8("عدد = 2\n")));
    QVERIFY(QFile::remove(deletedPath));
    registry.refreshAllFileStates();
    QCOMPARE(registry.document(modifiedId).externalState, DocumentExternalState::Modified);
    QCOMPARE(registry.document(deletedId).externalState, DocumentExternalState::Deleted);

    QVERIFY(registry.keepCurrentVersion(modifiedId));
    QVERIFY(registry.keepCurrentVersion(deletedId));

    const DocumentRecord modified = registry.document(modifiedId);
    QCOMPARE(modified.text, QString::fromUtf8("عدد = 99\n"));
    QVERIFY(modified.dirty);
    QCOMPARE(modified.externalState, DocumentExternalState::Unchanged);
    QCOMPARE(modified.identity.sizeBytes, DocumentFileIO::identityForPath(modifiedPath).sizeBytes);

    const DocumentRecord deleted = registry.document(deletedId);
    QCOMPARE(deleted.text, QString::fromUtf8("اسم = \"ليلى\"\n"));
    QVERIFY(deleted.dirty);
    QCOMPARE(deleted.externalState, DocumentExternalState::Unchanged);
    QCOMPARE(deleted.identity.sizeBytes, -1);
    QVERIFY(!deleted.identity.lastModifiedUtc.isValid());
}

void TestWorkbenchState::documentRegistryAssignsStableUris()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = QDir(temp.path()).filePath(QStringLiteral("main.apy"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("عدد = 1\n")));

    DocumentRegistry registry;
    QString error;
    const DocumentId fileId = registry.openPath(path, &error);
    QVERIFY2(fileId.isValid(), qPrintable(error));
    QCOMPARE(registry.document(fileId).uri, QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath()));

    const DocumentId untitledId = registry.createUntitled(QString::fromUtf8("مسودة\n"));
    const QUrl untitledUri = registry.document(untitledId).uri;
    QCOMPARE(untitledUri.scheme(), QStringLiteral("untitled"));
    QVERIFY(untitledUri.toString().contains(QString::number(untitledId.value())));
}

void TestWorkbenchState::documentRegistryVersionsDocumentsForStaleEditDetection()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = QDir(temp.path()).filePath(QStringLiteral("main.apy"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("عدد = 1\n")));

    DocumentRegistry registry;
    QString error;
    const DocumentId fileId = registry.openPath(path, &error);
    QVERIFY2(fileId.isValid(), qPrintable(error));
    const DocumentId untitledId = registry.createUntitled(QString::fromUtf8("مسودة\n"));
    QVERIFY(untitledId.isValid());

    QCOMPARE(registry.document(fileId).version, 1);
    QCOMPARE(registry.document(untitledId).version, 1);

    registry.setText(fileId, QString::fromUtf8("عدد = 2\n"));
    QCOMPARE(registry.document(fileId).version, 2);

    registry.markClean(fileId);
    QCOMPARE(registry.document(fileId).version, 2);

    registry.setPathAfterSave(fileId, path);
    QCOMPARE(registry.document(fileId).version, 2);

    QTest::qWait(1100);
    QVERIFY(DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("عدد = 3\n")));
    QVERIFY2(registry.reloadFromDisk(fileId, &error), qPrintable(error));
    QCOMPARE(registry.document(fileId).text, QString::fromUtf8("عدد = 3\n"));
    QCOMPARE(registry.document(fileId).version, 3);
}

void TestWorkbenchState::documentRegistryAppliesVersionedTextEdits()
{
    DocumentRegistry registry;
    const DocumentId id = registry.createUntitled(QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));
    QVERIFY(id.isValid());
    const int start = registry.document(id).text.indexOf(QString::fromUtf8("عدد"));
    QVERIFY(start >= 0);

    DocumentTextEdit edit;
    edit.expectedVersion = registry.document(id).version;
    edit.start = start;
    edit.length = QString::fromUtf8("عدد").size();
    edit.replacement = QString::fromUtf8("قيمة");

    QString error;
    QVERIFY2(registry.applyTextEdit(id, edit, &error), qPrintable(error));

    const DocumentRecord record = registry.document(id);
    QCOMPARE(record.text, QString::fromUtf8("قيمة = 1\nاطبع(عدد)\n"));
    QCOMPARE(record.version, 2);
    QVERIFY(record.dirty);
    QCOMPARE(record.lineEnding, DocumentLineEnding::Lf);
}

void TestWorkbenchState::documentRegistryAppliesVersionedTextEditBatches()
{
    DocumentRegistry registry;
    const DocumentId id = registry.createUntitled(QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));
    QVERIFY(id.isValid());

    const DocumentRecord before = registry.document(id);
    const int secondMatch = before.text.indexOf(QString::fromUtf8("عدد"), 1);
    QVERIFY(secondMatch > 0);
    const int tokenLength = static_cast<int>(QString::fromUtf8("عدد").size());

    QVector<DocumentTextEdit> edits;
    edits.push_back({before.version, 0, tokenLength, QString::fromUtf8("قيمة")});
    edits.push_back({before.version, secondMatch, tokenLength, QString::fromUtf8("قيمة")});

    QString error;
    QVERIFY2(registry.applyTextEdits(id, edits, &error), qPrintable(error));

    const DocumentRecord record = registry.document(id);
    QCOMPARE(record.text, QString::fromUtf8("قيمة = 1\nاطبع(قيمة)\n"));
    QCOMPARE(record.version, before.version + 1);
    QVERIFY(record.dirty);
}

void TestWorkbenchState::documentRegistryRejectsStaleOrInvalidTextEdits()
{
    DocumentRegistry registry;
    const DocumentId id = registry.createUntitled(QString::fromUtf8("عدد = 1\n"));
    QVERIFY(id.isValid());

    QString error;
    DocumentTextEdit staleEdit;
    staleEdit.expectedVersion = registry.document(id).version - 1;
    staleEdit.start = 0;
    staleEdit.length = 3;
    staleEdit.replacement = QString::fromUtf8("قيمة");
    QVERIFY(!registry.applyTextEdit(id, staleEdit, &error));
    QVERIFY(error.contains(QStringLiteral("stale"), Qt::CaseInsensitive));
    QCOMPARE(registry.document(id).text, QString::fromUtf8("عدد = 1\n"));
    QCOMPARE(registry.document(id).version, 1);

    error.clear();
    DocumentTextEdit invalidRange;
    invalidRange.expectedVersion = registry.document(id).version;
    invalidRange.start = registry.document(id).text.size() + 1;
    invalidRange.length = 1;
    invalidRange.replacement = QString::fromUtf8("قيمة");
    QVERIFY(!registry.applyTextEdit(id, invalidRange, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(registry.document(id).text, QString::fromUtf8("عدد = 1\n"));
    QCOMPARE(registry.document(id).version, 1);
}

void TestWorkbenchState::documentChangePollerReportsRegistryExternalChanges()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString cleanPath = QDir(temp.path()).filePath(QStringLiteral("clean.apy"));
    const QString changedPath = QDir(temp.path()).filePath(QStringLiteral("changed.apy"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(cleanPath, QString::fromUtf8("ثابت = 1\n")));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(changedPath, QString::fromUtf8("عدد = 1\n")));

    DocumentRegistry registry;
    QString error;
    const DocumentId cleanId = registry.openPath(cleanPath, &error);
    QVERIFY2(cleanId.isValid(), qPrintable(error));
    const DocumentId changedId = registry.openPath(changedPath, &error);
    QVERIFY2(changedId.isValid(), qPrintable(error));

    DocumentChangePoller poller(&registry);
    QVERIFY(!poller.poll().hasChanges());

    QTest::qWait(1100);
    QVERIFY(DocumentFileIO::saveUtf8Atomically(changedPath, QString::fromUtf8("عدد = 2\n")));

    const DocumentChangeSnapshot snapshot = poller.poll();
    QVERIFY(snapshot.hasChanges());
    QCOMPARE(snapshot.changedDocuments.size(), 1);
    QCOMPARE(snapshot.changedDocuments.first().id, changedId);
    QCOMPARE(snapshot.changedDocuments.first().externalState, DocumentExternalState::Modified);
    QCOMPARE(registry.document(cleanId).externalState, DocumentExternalState::Unchanged);
}

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
    QVERIFY(request.message.contains(QStringLiteral("dirty.apy")));

    QCOMPARE(UnsavedChangesGuard::allowsOperation(UnsavedChangesChoice::Cancel), false);
    QCOMPARE(UnsavedChangesGuard::allowsOperation(UnsavedChangesChoice::Discard), true);
    QCOMPARE(UnsavedChangesGuard::allowsOperation(UnsavedChangesChoice::Save), true);
}

void TestWorkbenchState::workbenchThemeProvidesDarkAndLightStyleSheets()
{
    const QString dark = WorkbenchTheme::darkStyleSheet();
    const QString light = WorkbenchTheme::lightStyleSheet();

    QVERIFY(dark.contains(QStringLiteral("#0f141a")));
    QVERIFY(dark.contains(QStringLiteral("QToolButton[role=\"topMenu\"]::menu-indicator")));
    QVERIFY(dark.contains(QStringLiteral("QStatusBar")));

    QVERIFY(light.contains(QStringLiteral("#F6F7FB")));
    QVERIFY(light.contains(QStringLiteral("QToolButton[role=\"topMenu\"]::menu-indicator")));
    QVERIFY(light.contains(QStringLiteral("QStatusBar")));
    QVERIFY(!light.contains(QStringLiteral("#0f141a")));
    QVERIFY(dark != light);
}

QTEST_MAIN(TestWorkbenchState)
#include "TestWorkbenchState.moc"
