#include <QtTest/QtTest>

#include "MainWindow.h"

#include <QMenuBar>
#include <QLabel>
#include <QLineEdit>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void opensProjectAndFileFromPath();
    void usesSingleRtlTopCommandBarWithMenuButtons();
    void exposesLisanLogoAssetInShell();
    void exposesPremiumFutureBottomPanelTabs();
    void enforcesRtlDirectionAcrossShellContainers();
    void exposesCommandPaletteAction();
    void newFileClearsCurrentPathAndEditorText();
    void newFileCreatesANewEditorTab();
    void openingMultipleFilesKeepsEachDocumentInATab();
    void projectTreeShowsOnlyFileNames();
    void outputPanelIsVisibleForRunFeedback();
    void outputPlaceholderPaintsFromRight();
    void untitledEditorBufferMaterializesForRunWithoutSaveDialog();
    void runUsesUntitledBufferWithoutOpeningSaveDialog();
    void runToolProvidesCancelableStructuredFeedback();
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

void TestMainWindow::opensProjectAndFileFromPath()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("src/main.apy"), QString::fromUtf8("اطبع(\"مرحبا\")\n"));

    MainWindow window;
    QVERIFY(window.openPath(root.absolutePath()));
    QCOMPARE(window.currentProjectRoot(), root.absolutePath());

    QVERIFY(window.openPath(filePath));
    QCOMPARE(QDir::toNativeSeparators(window.currentEditorPath()), QDir::toNativeSeparators(filePath));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QVERIFY(editor->toPlainText().contains(QString::fromUtf8("مرحبا")));
}

void TestMainWindow::usesSingleRtlTopCommandBarWithMenuButtons()
{
    MainWindow window;
    QVERIFY(window.windowTitle().contains(QString::fromUtf8("استوديو لسان")));

    QVERIFY(window.findChild<QMenuBar *>(QStringLiteral("mainMenuBar")) == nullptr);
    QVERIFY(window.findChild<QToolBar *>() == nullptr);

    auto *topShell = window.findChild<QWidget *>(QStringLiteral("topShell"));
    QVERIFY(topShell != nullptr);
    QCOMPARE(topShell->layoutDirection(), Qt::RightToLeft);

    auto *topMenuRow = window.findChild<QWidget *>(QStringLiteral("topMenuRow"));
    QVERIFY(topMenuRow != nullptr);
    QCOMPARE(topMenuRow->layoutDirection(), Qt::RightToLeft);

    auto *brandBlock = window.findChild<QWidget *>(QStringLiteral("brandBlock"));
    QVERIFY(brandBlock != nullptr);
    QCOMPARE(brandBlock->layoutDirection(), Qt::LeftToRight);

    auto *brandText = window.findChild<QLabel *>(QStringLiteral("brandTextLabel"));
    QVERIFY(brandText != nullptr);
    QCOMPARE(brandText->text(), QStringLiteral("Lisan Studio"));

    QVERIFY(window.findChild<QWidget *>(QStringLiteral("topActionRow")) == nullptr);

    const QStringList menuButtonNames = {
        QStringLiteral("topMenuFileButton"),
        QStringLiteral("topMenuEditButton"),
        QStringLiteral("topMenuViewButton"),
        QStringLiteral("topMenuRunButton"),
        QStringLiteral("topMenuSearchButton"),
        QStringLiteral("topMenuToolsButton"),
        QStringLiteral("topMenuHelpButton"),
    };
    const QStringList menuButtonTexts = {
        QString::fromUtf8("ملف"),
        QString::fromUtf8("تحرير"),
        QString::fromUtf8("عرض"),
        QString::fromUtf8("تشغيل"),
        QString::fromUtf8("بحث"),
        QString::fromUtf8("أدوات"),
        QString::fromUtf8("مساعدة"),
    };

    for (int i = 0; i < menuButtonNames.size(); ++i) {
        auto *button = window.findChild<QToolButton *>(menuButtonNames.at(i));
        QVERIFY(button != nullptr);
        QCOMPARE(button->layoutDirection(), Qt::RightToLeft);
        QCOMPARE(button->text(), menuButtonTexts.at(i));
        QVERIFY(button->menu() != nullptr);
        QVERIFY(!button->menu()->actions().isEmpty());
    }

    auto *runButton = window.findChild<QToolButton *>(QStringLiteral("topRunButton"));
    QVERIFY(runButton != nullptr);
    QCOMPARE(runButton->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
    QCOMPARE(runButton->text(), QString::fromUtf8("تشغيل"));

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    QVERIFY(commandBox != nullptr);
    QCOMPARE(commandBox->layoutDirection(), Qt::RightToLeft);
    QCOMPARE(commandBox->placeholderText(), QString::fromUtf8("ابحث في الأوامر والملفات..."));

    const QStringList retiredToolbarButtonNames = {
        QStringLiteral("topNewFileButton"),
        QStringLiteral("topOpenFileButton"),
        QStringLiteral("topOpenProjectButton"),
        QStringLiteral("topSaveAsButton"),
        QStringLiteral("topLintButton"),
        QStringLiteral("topFormatButton"),
        QStringLiteral("topStopButton"),
        QStringLiteral("topSaveButton"),
        QStringLiteral("topSearchButton"),
        QStringLiteral("topCommandPaletteButton"),
        QStringLiteral("topSettingsButton"),
    };
    for (const QString &buttonName : retiredToolbarButtonNames) {
        QVERIFY2(window.findChild<QToolButton *>(buttonName) == nullptr, qPrintable(buttonName));
    }
}

void TestMainWindow::exposesLisanLogoAssetInShell()
{
    MainWindow window;

    auto *logo = window.findChild<QLabel *>(QStringLiteral("brandLogoLabel"));
    QVERIFY(logo != nullptr);
    QVERIFY(!logo->pixmap().isNull());
    QVERIFY(!QPixmap(QStringLiteral(":/branding/lisan-logo.png")).isNull());
}

void TestMainWindow::exposesPremiumFutureBottomPanelTabs()
{
    MainWindow window;

    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("bottomPanelTabs"));
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->layoutDirection(), Qt::RightToLeft);
    QCOMPARE(tabs->count(), 4);
    QCOMPARE(tabs->tabText(0), QString::fromUtf8("الطرفية"));
    QCOMPARE(tabs->tabText(1), QString::fromUtf8("الإخراج"));
    QCOMPARE(tabs->tabText(2), QString::fromUtf8("المشاكل"));
    QCOMPARE(tabs->tabText(3), QString::fromUtf8("التصحيح"));
}

void TestMainWindow::enforcesRtlDirectionAcrossShellContainers()
{
    MainWindow window;

    QCOMPARE(window.layoutDirection(), Qt::RightToLeft);

    QVERIFY(window.findChild<QMenuBar *>(QStringLiteral("mainMenuBar")) == nullptr);
    QVERIFY(window.findChild<QToolBar *>() == nullptr);

    auto *topShell = window.findChild<QWidget *>(QStringLiteral("topShell"));
    QVERIFY(topShell != nullptr);
    QCOMPARE(topShell->layoutDirection(), Qt::RightToLeft);

    auto *projectTree = window.findChild<QTreeView *>(QStringLiteral("projectTree"));
    QVERIFY(projectTree != nullptr);
    QCOMPARE(projectTree->layoutDirection(), Qt::RightToLeft);

    auto *editorTabs = window.findChild<QTabWidget *>(QStringLiteral("editorTabs"));
    QVERIFY(editorTabs != nullptr);
    QCOMPARE(editorTabs->layoutDirection(), Qt::RightToLeft);

    auto *bottomTabs = window.findChild<QTabWidget *>(QStringLiteral("bottomPanelTabs"));
    QVERIFY(bottomTabs != nullptr);
    QCOMPARE(bottomTabs->layoutDirection(), Qt::RightToLeft);

    auto *outputPanel = window.findChild<QPlainTextEdit *>(QStringLiteral("outputPanel"));
    QVERIFY(outputPanel != nullptr);
    QCOMPARE(outputPanel->layoutDirection(), Qt::RightToLeft);

    auto *statusBar = window.statusBar();
    QVERIFY(statusBar != nullptr);
    QCOMPARE(statusBar->layoutDirection(), Qt::RightToLeft);
}

void TestMainWindow::exposesCommandPaletteAction()
{
    MainWindow window;

    auto *action = window.findChild<QAction *>(QStringLiteral("commandPaletteAction"));
    QVERIFY(action != nullptr);
    QCOMPARE(action->text(), QString::fromUtf8("لوحة الأوامر"));
    QVERIFY(action->shortcuts().contains(QKeySequence(QStringLiteral("Ctrl+Shift+P"))));
}

void TestMainWindow::newFileClearsCurrentPathAndEditorText()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("اطبع(\"مرحبا\")\n"));

    MainWindow window;
    QVERIFY(window.openPath(filePath));
    QVERIFY(!window.currentEditorPath().isEmpty());

    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    QCOMPARE(window.currentEditorPath(), QString());
    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->toPlainText(), QString());
}

void TestMainWindow::newFileCreatesANewEditorTab()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("اطبع(\"مرحبا\")\n"));

    MainWindow window;
    QVERIFY(window.openPath(filePath));

    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("editorTabs"));
    QVERIFY(tabs != nullptr);
    const int before = tabs->count();

    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    QCOMPARE(tabs->count(), before + 1);
    QCOMPARE(window.currentEditorPath(), QString());
    QCOMPARE(tabs->tabText(tabs->currentIndex()), QString::fromUtf8("ملف جديد"));
}

void TestMainWindow::openingMultipleFilesKeepsEachDocumentInATab()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString firstPath = writeFile(root, QStringLiteral("first.apy"), QString::fromUtf8("اطبع(\"الأول\")\n"));
    const QString secondPath = writeFile(root, QStringLiteral("second.apy"), QString::fromUtf8("اطبع(\"الثاني\")\n"));

    MainWindow window;
    QVERIFY(window.openPath(firstPath));
    QVERIFY(window.openPath(secondPath));

    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("editorTabs"));
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->tabText(0), QStringLiteral("first.apy"));
    QCOMPARE(tabs->tabText(1), QStringLiteral("second.apy"));
    QCOMPARE(QDir::toNativeSeparators(window.currentEditorPath()), QDir::toNativeSeparators(secondPath));

    tabs->setCurrentIndex(0);
    QCOMPARE(QDir::toNativeSeparators(window.currentEditorPath()), QDir::toNativeSeparators(firstPath));
}

void TestMainWindow::projectTreeShowsOnlyFileNames()
{
    MainWindow window;
    auto *tree = window.findChild<QTreeView *>(QStringLiteral("projectTree"));
    QVERIFY(tree != nullptr);

    QVERIFY(!tree->isColumnHidden(0));
    QVERIFY(tree->isColumnHidden(1));
    QVERIFY(tree->isColumnHidden(2));
    QVERIFY(tree->isColumnHidden(3));
}

void TestMainWindow::outputPanelIsVisibleForRunFeedback()
{
    MainWindow window;
    window.resize(1000, 700);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *outputPanel = window.findChild<QPlainTextEdit *>(QStringLiteral("outputPanel"));
    QVERIFY(outputPanel != nullptr);
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("bottomPanelTabs"));
    QVERIFY(tabs != nullptr);

    QVERIFY(QMetaObject::invokeMethod(&window, "runCurrentFile", Qt::DirectConnection));

    QCOMPARE(tabs->currentWidget(), outputPanel);
    QVERIFY(outputPanel->isVisible());
    QVERIFY(outputPanel->height() >= 140);
}

void TestMainWindow::outputPlaceholderPaintsFromRight()
{
    MainWindow window;
    window.resize(1200, 800);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *outputPanel = window.findChild<QPlainTextEdit *>(QStringLiteral("outputPanel"));
    QVERIFY(outputPanel != nullptr);
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("bottomPanelTabs"));
    QVERIFY(tabs != nullptr);
    tabs->setCurrentWidget(outputPanel);
    QCoreApplication::processEvents();
    QVERIFY(outputPanel->isVisible());
    QVERIFY(outputPanel->size().isValid());

    QImage image(outputPanel->size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    outputPanel->render(&image);

    auto countVisibleTextPixels = [&image](const QRect &rect) {
        int count = 0;
        for (int y = rect.top(); y <= rect.bottom(); ++y) {
            for (int x = rect.left(); x <= rect.right(); ++x) {
                const QColor color = image.pixelColor(x, y);
                if (color.lightness() > 110 && color.alpha() > 0) {
                    ++count;
                }
            }
        }
        return count;
    };

    const QRect content = outputPanel->viewport()->geometry();
    const QRect leftBand(content.left() + 12, content.top() + 8, 260, 44);
    const QRect rightBand(content.right() - 320, content.top() + 8, 260, 44);
    const int leftPixels = countVisibleTextPixels(leftBand);
    const int rightPixels = countVisibleTextPixels(rightBand);

    QVERIFY2(rightPixels > leftPixels * 2,
        qPrintable(QStringLiteral("Arabic output placeholder should be painted near the right writing edge. left=%1 right=%2")
            .arg(leftPixels)
            .arg(rightPixels)));
}

void TestMainWindow::untitledEditorBufferMaterializesForRunWithoutSaveDialog()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    MainWindow window;
    QVERIFY(window.openPath(temp.path()));
    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("اطبع(\"من المحرر\")\n"));

    QString error;
    const QString materializedPath = window.materializeRunnableBuffer(&error);

    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!materializedPath.isEmpty());
    QVERIFY(QFileInfo(materializedPath).exists());
    QVERIFY(materializedPath.startsWith(temp.path()));
    QCOMPARE(window.currentEditorPath(), QString());

    QFile materialized(materializedPath);
    QVERIFY(materialized.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(materialized.readAll()), editor->toPlainText());
}

void TestMainWindow::runUsesUntitledBufferWithoutOpeningSaveDialog()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    MainWindow window;
    QVERIFY(window.openPath(temp.path()));
    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("اطبع(\"من زر التشغيل\")\n"));

    QVERIFY(QMetaObject::invokeMethod(&window, "runCurrentFile", Qt::DirectConnection));

    QVERIFY(QApplication::activeModalWidget() == nullptr);
    QCOMPARE(window.currentEditorPath(), QString());
    QVERIFY(QFileInfo(temp.filePath(QStringLiteral(".arabic-code-studio/current-buffer.apy"))).exists());

    auto *outputPanel = window.findChild<QPlainTextEdit *>(QStringLiteral("outputPanel"));
    QVERIFY(outputPanel != nullptr);
    QVERIFY(!outputPanel->toPlainText().isEmpty());
}

void TestMainWindow::runToolProvidesCancelableStructuredFeedback()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    MainWindow window;
    QVERIFY(window.openPath(temp.path()));
    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("اطبع(\"من أداة التشغيل\")\n"));

    auto *cancelAction = window.findChild<QAction *>(QStringLiteral("cancelRunAction"));
    QVERIFY(cancelAction != nullptr);
    QVERIFY(!cancelAction->isEnabled());

    auto *timeoutTimer = window.findChild<QTimer *>(QStringLiteral("runtimeTimeoutTimer"));
    QVERIFY(timeoutTimer != nullptr);
    QCOMPARE(timeoutTimer->interval(), 30000);
    QVERIFY(timeoutTimer->isSingleShot());

    QVERIFY(QMetaObject::invokeMethod(&window, "runCurrentFile", Qt::DirectConnection));
    QTRY_VERIFY_WITH_TIMEOUT(!cancelAction->isEnabled(), 5000);

    auto *outputPanel = window.findChild<QPlainTextEdit *>(QStringLiteral("outputPanel"));
    QVERIFY(outputPanel != nullptr);
    const QString output = outputPanel->toPlainText();
    QVERIFY2(output.contains(QString::fromUtf8("الأمر: تشغيل")), qPrintable(output));
    QVERIFY2(output.contains(QString::fromUtf8("ملف:")), qPrintable(output));
    QVERIFY2(output.contains(QString::fromUtf8("مجلد العمل:")), qPrintable(output));
    QVERIFY2(output.contains(QString::fromUtf8("رمز الخروج:")) || output.contains(QString::fromUtf8("تعذر بدء العملية")), qPrintable(output));
}

QTEST_MAIN(TestMainWindow)
#include "TestMainWindow.moc"
