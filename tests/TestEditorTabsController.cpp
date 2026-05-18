#include <QtTest/QtTest>

#include "DocumentRegistry.h"
#include "EditorSurface.h"
#include "EditorTabsController.h"
#include "WorkbenchState.h"
#include "WorkspaceSettingsStore.h"

#include <QDir>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextCursor>

class TestEditorTabsController : public QObject
{
    Q_OBJECT

private slots:
    void editorTabsControllerOpenFileCreatesTabAndRegistersDocument();
    void editorTabsControllerOpenSamePathTwiceFocusesExistingTab();
    void editorTabsControllerCloseTabClosesRegistryRecord();
    void editorTabsControllerApplyFontReachesAllOpenSurfaces();
};

static QString writeFile(const QDir &root, const QString &relative, const QString &text)
{
    const QFileInfo info(root.filePath(relative));
    QDir().mkpath(info.absolutePath());
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qFatal("could not write test file");
    }
    file.write(text.toUtf8());
    return info.absoluteFilePath();
}

void TestEditorTabsController::editorTabsControllerOpenFileCreatesTabAndRegistersDocument()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\n"));

    QTabWidget tabs;
    DocumentRegistry registry;
    WorkbenchState state;
    EditorTabsController controller(&tabs, registry, state);
    QSignalSpy currentEditorSpy(&controller, &EditorTabsController::currentEditorChanged);

    QString error;
    QVERIFY2(controller.openFile(filePath, &error), qPrintable(error));

    QCOMPARE(tabs.count(), 1);
    EditorSurface *surface = controller.currentSurface();
    QVERIFY(surface != nullptr);
    QCOMPARE(currentEditorSpy.count(), 1);
    QCOMPARE(qvariant_cast<EditorSurface *>(currentEditorSpy.takeFirst().at(0)), surface);

    const DocumentId id = controller.documentIdForSurface(surface);
    QVERIFY(id.isValid());
    QCOMPARE(registry.findByPath(filePath), id);
}

void TestEditorTabsController::editorTabsControllerOpenSamePathTwiceFocusesExistingTab()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\n"));

    QTabWidget tabs;
    DocumentRegistry registry;
    WorkbenchState state;
    EditorTabsController controller(&tabs, registry, state);

    QString error;
    QVERIFY2(controller.openFile(filePath, &error), qPrintable(error));
    EditorSurface *surface = controller.currentSurface();
    QVERIFY(surface != nullptr);

    QSignalSpy currentEditorSpy(&controller, &EditorTabsController::currentEditorChanged);
    QVERIFY2(controller.openFile(filePath, &error), qPrintable(error));

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(controller.currentSurface(), surface);
    QCOMPARE(currentEditorSpy.count(), 1);
    QCOMPARE(qvariant_cast<EditorSurface *>(currentEditorSpy.takeFirst().at(0)), surface);
    QCOMPARE(registry.documents().size(), 1);
}

void TestEditorTabsController::editorTabsControllerCloseTabClosesRegistryRecord()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\n"));

    QTabWidget tabs;
    DocumentRegistry registry;
    WorkbenchState state;
    EditorTabsController controller(&tabs, registry, state);

    QString error;
    QVERIFY2(controller.openFile(filePath, &error), qPrintable(error));

    controller.closeTab(0);

    QCOMPARE(registry.findByPath(filePath), DocumentId());
}

void TestEditorTabsController::editorTabsControllerApplyFontReachesAllOpenSurfaces()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString firstPath = writeFile(root, QStringLiteral("first.apy"), QString::fromUtf8("أ = 1\n"));
    const QString secondPath = writeFile(root, QStringLiteral("second.apy"), QString::fromUtf8("ب = 2\n"));

    QTabWidget tabs;
    DocumentRegistry registry;
    WorkbenchState state;
    EditorTabsController controller(&tabs, registry, state);

    QString error;
    QVERIFY2(controller.openFile(firstPath, &error), qPrintable(error));
    const DocumentId firstId = registry.findByPath(firstPath);
    QVERIFY2(controller.openFile(secondPath, &error), qPrintable(error));
    const DocumentId secondId = registry.findByPath(secondPath);

    QFont font(QStringLiteral("Arial"), 17);
    controller.applyFont(font);

    EditorSurface *firstSurface = controller.surfaceForDocument(firstId);
    EditorSurface *secondSurface = controller.surfaceForDocument(secondId);
    QVERIFY(firstSurface != nullptr);
    QVERIFY(secondSurface != nullptr);
    QCOMPARE(firstSurface->font().family(), QStringLiteral("Arial"));
    QCOMPARE(firstSurface->font().pointSize(), 17);
    QCOMPARE(secondSurface->font().family(), QStringLiteral("Arial"));
    QCOMPARE(secondSurface->font().pointSize(), 17);
}

QTEST_MAIN(TestEditorTabsController)
#include "TestEditorTabsController.moc"
