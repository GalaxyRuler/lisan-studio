#include <QtTest/QtTest>

#include "DocumentRegistry.h"
#include "UnsavedChangesGuard.h"
#include "WorkbenchState.h"

#include <algorithm>

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
    void unsavedChangesGuardRequiresSaveDiscardOrCancelForDirtyDocuments();
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

QTEST_MAIN(TestWorkbenchState)
#include "TestWorkbenchState.moc"
