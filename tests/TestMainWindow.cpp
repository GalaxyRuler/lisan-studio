#include <QtTest/QtTest>

#include "MainWindow.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QMenuBar>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextOption>
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
    void projectSearchShowsClickableResultRows();
    void projectSearchFindsCurrentUnsavedEditorImmediately();
    void projectSearchResultRowsFillRtlViewport();
    void problemsPanelShowsHiddenBidiWarnings();
    void outputPanelIsVisibleForRunFeedback();
    void outputPlaceholderPaintsFromRight();
    void untitledEditorBufferMaterializesForRunWithoutSaveDialog();
    void runUsesUntitledBufferWithoutOpeningSaveDialog();
    void runToolProvidesCancelableStructuredFeedback();
    void settingsDialogExposesCategoriesAndRuntimeDiagnostics();
    void settingsDialogAppliesEditorFontVisibly();
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
    QVERIFY(window.styleSheet().contains(QStringLiteral("QToolButton[role=\"topMenu\"]::menu-indicator")));
    QVERIFY(window.styleSheet().contains(QStringLiteral("image: none")));
    QVERIFY(window.styleSheet().contains(QStringLiteral("QFrame[role=\"menuPopup\"]")));
    QVERIFY(window.styleSheet().contains(QStringLiteral("QPushButton[role=\"menuRow\"]")));
    QVERIFY(window.styleSheet().contains(QStringLiteral("QToolButton[role=\"topMenu\"]:hover")));
    QVERIFY(window.styleSheet().contains(QStringLiteral("border-bottom: 2px solid #4C8DFF")));
    QVERIFY(!window.windowIcon().isNull());

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
    const QStringList menuObjectNames = {
        QStringLiteral("fileMenu"),
        QStringLiteral("editMenu"),
        QStringLiteral("viewMenu"),
        QStringLiteral("runMenu"),
        QStringLiteral("searchMenu"),
        QStringLiteral("toolsMenu"),
        QStringLiteral("helpMenu"),
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
        QCOMPARE(button->cursor().shape(), Qt::PointingHandCursor);
        QVERIFY(button->menu() == nullptr);
        QCOMPARE(button->property("attachedMenuName").toString(), menuObjectNames.at(i));

        auto *menu = window.findChild<QFrame *>(menuObjectNames.at(i));
        QVERIFY(menu != nullptr);
        QCOMPARE(menu->layoutDirection(), Qt::RightToLeft);
        QCOMPARE(menu->property("role").toString(), QStringLiteral("menuPopup"));
        const auto rows = menu->findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly);
        QVERIFY(!rows.isEmpty());
        for (auto *row : rows) {
            QVERIFY(row != nullptr);
            QCOMPARE(row->property("role").toString(), QStringLiteral("menuRow"));
            QVERIFY(row->icon().isNull());
            QVERIFY(row->isFlat());
        }

        button->click();
        QVERIFY(menu->isVisible());
        QVERIFY(button->property("active").toBool());
        menu->hide();
        QVERIFY(!button->isChecked());
        QVERIFY(!button->property("active").toBool());
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
    QCOMPARE(tabs->count(), 5);
    QCOMPARE(tabs->tabText(0), QString::fromUtf8("الطرفية"));
    QCOMPARE(tabs->tabText(1), QString::fromUtf8("الإخراج"));
    QCOMPARE(tabs->tabText(2), QString::fromUtf8("المشاكل"));
    QCOMPARE(tabs->tabText(3), QString::fromUtf8("نتائج البحث"));
    QCOMPARE(tabs->tabText(4), QString::fromUtf8("التصحيح"));
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

    auto *problemsPanel = window.findChild<QListWidget *>(QStringLiteral("problemsPanel"));
    QVERIFY(problemsPanel != nullptr);
    QCOMPARE(problemsPanel->layoutDirection(), Qt::RightToLeft);

    auto *searchResultsPanel = window.findChild<QListWidget *>(QStringLiteral("searchResultsPanel"));
    QVERIFY(searchResultsPanel != nullptr);
    QCOMPARE(searchResultsPanel->layoutDirection(), Qt::RightToLeft);

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

    auto *settingsAction = window.findChild<QAction *>(QStringLiteral("settingsAction"));
    QVERIFY(settingsAction != nullptr);
    QCOMPARE(settingsAction->text(), QString::fromUtf8("الإعدادات"));
    QVERIFY(settingsAction->icon().isNull());
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

void TestMainWindow::projectSearchShowsClickableResultRows()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(
        root,
        QStringLiteral("src/main.apy"),
        QString::fromUtf8("س = 1\nاطبع(س)\nاكتب(\"بعيد\")\n"));

    MainWindow window;
    window.resize(1000, 700);
    QVERIFY(window.openPath(root.absolutePath()));

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    QVERIFY(commandBox != nullptr);
    commandBox->setText(QString::fromUtf8("اطبع"));

    QVERIFY(QMetaObject::invokeMethod(&window, "findInProject", Qt::DirectConnection));

    auto *results = window.findChild<QListWidget *>(QStringLiteral("searchResultsPanel"));
    QVERIFY(results != nullptr);
    QCOMPARE(results->count(), 0);
    QTRY_COMPARE(results->count(), 1);
    auto *resultRow = results->itemWidget(results->item(0));
    QVERIFY(resultRow != nullptr);
    QCOMPARE(resultRow->layoutDirection(), Qt::RightToLeft);
    auto *fileLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultFileLabel"));
    auto *lineLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultLineLabel"));
    auto *previewLabel = resultRow->findChild<QPlainTextEdit *>(QStringLiteral("searchResultPreviewText"));
    QVERIFY(fileLabel != nullptr);
    QVERIFY(lineLabel != nullptr);
    QVERIFY(previewLabel != nullptr);
    QCOMPARE(fileLabel->text(), QStringLiteral("main.apy"));
    QCOMPARE(lineLabel->text(), QString::fromUtf8("السطر 2"));
    QVERIFY(previewLabel->toPlainText().contains(QString::fromUtf8("اطبع")));
    QCOMPARE(fileLabel->alignment() & Qt::AlignRight, Qt::AlignRight);
    QCOMPARE(previewLabel->document()->defaultTextOption().textDirection(), Qt::RightToLeft);
    QCOMPARE(previewLabel->document()->defaultTextOption().alignment() & Qt::AlignRight, Qt::AlignRight);
    QCOMPARE(QDir::toNativeSeparators(results->item(0)->data(Qt::UserRole).toString()), QDir::toNativeSeparators(filePath));
    QCOMPARE(results->item(0)->data(Qt::UserRole + 1).toInt(), 2);

    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("bottomPanelTabs"));
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->currentWidget(), results);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    results->setFocus();
    results->setCurrentRow(0);
    QTest::keyClick(results, Qt::Key_Return);

    QCOMPARE(QDir::toNativeSeparators(window.currentEditorPath()), QDir::toNativeSeparators(filePath));
    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->textCursor().blockNumber(), 1);
}

void TestMainWindow::projectSearchFindsCurrentUnsavedEditorImmediately()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    MainWindow window;
    QVERIFY(window.openPath(temp.path()));
    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("العمر = 20\nاذا العمر >= 18:\n    اطبع(\"adult\")\n"));

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    QVERIFY(commandBox != nullptr);
    commandBox->setText(QStringLiteral("adult"));

    QVERIFY(QMetaObject::invokeMethod(&window, "findInProject", Qt::DirectConnection));

    auto *results = window.findChild<QListWidget *>(QStringLiteral("searchResultsPanel"));
    QVERIFY(results != nullptr);
    QCOMPARE(results->count(), 1);
    auto *resultRow = results->itemWidget(results->item(0));
    QVERIFY(resultRow != nullptr);
    auto *fileLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultFileLabel"));
    auto *lineLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultLineLabel"));
    auto *previewLabel = resultRow->findChild<QPlainTextEdit *>(QStringLiteral("searchResultPreviewText"));
    QVERIFY(fileLabel != nullptr);
    QVERIFY(lineLabel != nullptr);
    QVERIFY(previewLabel != nullptr);
    QCOMPARE(fileLabel->text(), QString::fromUtf8("المحرر الحالي"));
    QCOMPARE(lineLabel->text(), QString::fromUtf8("السطر 3"));
    QVERIFY(previewLabel->toPlainText().contains(QStringLiteral("adult")));
    QCOMPARE(results->item(0)->data(Qt::UserRole).toString(), QString());
    QCOMPARE(results->item(0)->data(Qt::UserRole + 1).toInt(), 3);
}

void TestMainWindow::projectSearchResultRowsFillRtlViewport()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    MainWindow window;
    window.resize(1200, 800);
    QVERIFY(window.openPath(temp.path()));
    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("العمر = 20\nاطبع(\"adult\")\n"));

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    QVERIFY(commandBox != nullptr);
    commandBox->setText(QStringLiteral("adult"));
    QVERIFY(QMetaObject::invokeMethod(&window, "findInProject", Qt::DirectConnection));

    auto *results = window.findChild<QListWidget *>(QStringLiteral("searchResultsPanel"));
    QVERIFY(results != nullptr);
    QCOMPARE(results->count(), 1);
    QCoreApplication::processEvents();

    auto *resultRow = results->itemWidget(results->item(0));
    QVERIFY(resultRow != nullptr);
    auto *fileLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultFileLabel"));
    QVERIFY(fileLabel != nullptr);
    auto *previewLabel = resultRow->findChild<QPlainTextEdit *>(QStringLiteral("searchResultPreviewText"));
    QVERIFY(previewLabel != nullptr);

    const QRect rowRect = resultRow->geometry();
    const int viewportWidth = results->viewport()->width();
    QVERIFY2(rowRect.width() > viewportWidth * 0.9,
        qPrintable(QStringLiteral("search result row should fill viewport width. row=%1 viewport=%2")
            .arg(rowRect.width())
            .arg(viewportWidth)));

    const QRect labelRect(fileLabel->mapTo(results->viewport(), QPoint(0, 0)), fileLabel->size());
    QVERIFY2(labelRect.right() > viewportWidth - 260,
        qPrintable(QStringLiteral("RTL search result title should sit near the right edge. labelRight=%1 viewport=%2")
            .arg(labelRect.right())
            .arg(viewportWidth)));

    QCOMPARE(previewLabel->document()->defaultTextOption().textDirection(), Qt::RightToLeft);
    QCOMPARE(previewLabel->document()->defaultTextOption().alignment() & Qt::AlignRight, Qt::AlignRight);
    QCOMPARE(previewLabel->frameShape(), QFrame::NoFrame);
    QCOMPARE(previewLabel->focusPolicy(), Qt::NoFocus);
}

void TestMainWindow::problemsPanelShowsHiddenBidiWarnings()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(
        root,
        QStringLiteral("main.apy"),
        QString::fromUtf8("اطبع(\"سليم\")\n") + QChar(0x202E) + QString::fromUtf8("اطبع(\"مخفي\")\n"));

    MainWindow window;
    QVERIFY(window.openPath(filePath));

    auto *problems = window.findChild<QListWidget *>(QStringLiteral("problemsPanel"));
    QVERIFY(problems != nullptr);
    QCOMPARE(problems->count(), 1);
    QVERIFY(problems->item(0)->text().contains(QString::fromUtf8("تحكم اتجاه مخفي")));
    QVERIFY(problems->item(0)->text().contains(QStringLiteral("RIGHT-TO-LEFT OVERRIDE")));
    QCOMPARE(QDir::toNativeSeparators(problems->item(0)->data(Qt::UserRole).toString()), QDir::toNativeSeparators(filePath));
    QCOMPARE(problems->item(0)->data(Qt::UserRole + 1).toInt(), 2);
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

void TestMainWindow::settingsDialogExposesCategoriesAndRuntimeDiagnostics()
{
    MainWindow window;
    bool inspected = false;
    QString failure;

    QTimer::singleShot(0, &window, [&window]() {
        QMetaObject::invokeMethod(&window, "openSettings", Qt::DirectConnection);
    });
    QTimer::singleShot(150, &window, [&inspected, &failure]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            failure = QStringLiteral("settings dialog did not open");
            return;
        }

        auto *categories = dialog->findChild<QListWidget *>(QStringLiteral("settingsCategories"));
        auto *pages = dialog->findChild<QTabWidget *>(QStringLiteral("settingsPages"));
        auto *fontFamily = dialog->findChild<QComboBox *>(QStringLiteral("editorFontFamilyCombo"));
        auto *nativeFontPreview = dialog->findChild<QFontComboBox *>(QStringLiteral("editorFontFamilyCombo"));
        auto *pythonPath = dialog->findChild<QLabel *>(QStringLiteral("runtimePythonPathValue"));
        auto *packageStatus = dialog->findChild<QLabel *>(QStringLiteral("runtimePackageStatusValue"));
        auto *buttons = dialog->findChild<QDialogButtonBox *>();

        inspected = true;
        auto require = [&inspected, &failure](bool condition, const QString &message) {
            if (inspected && !condition) {
                inspected = false;
                failure = message;
            }
        };
        require(dialog->objectName() == QStringLiteral("settingsDialog"), QStringLiteral("wrong settings dialog object name"));
        require(dialog->layoutDirection() == Qt::RightToLeft, QStringLiteral("settings dialog is not RTL"));
        require((dialog->windowFlags() & Qt::FramelessWindowHint), QStringLiteral("settings dialog is not frameless"));
        require(dialog->findChild<QLabel *>(QStringLiteral("settingsHeaderTitle")), QStringLiteral("settings title missing"));
        require(!dialog->findChild<QToolButton *>(QStringLiteral("settingsHeaderCloseButton")), QStringLiteral("redundant settings close button returned"));
        require(buttons, QStringLiteral("settings buttons missing"));
        require(buttons && buttons->button(QDialogButtonBox::Ok)->text() == QString::fromUtf8("تطبيق"), QStringLiteral("settings apply label wrong"));
        require(buttons && buttons->button(QDialogButtonBox::Cancel)->text() == QString::fromUtf8("إلغاء"), QStringLiteral("settings cancel label wrong"));
        require(categories, QStringLiteral("settings categories missing"));
        require(categories && categories->layoutDirection() == Qt::RightToLeft, QStringLiteral("settings categories are not RTL"));
        require(categories && categories->count() == 3, QStringLiteral("settings category count wrong"));
        require(categories && categories->item(0)->text() == QString::fromUtf8("المحرر"), QStringLiteral("editor category missing"));
        require(categories && categories->item(1)->text() == QString::fromUtf8("التشغيل"), QStringLiteral("runtime category missing"));
        require(categories && categories->item(2)->text() == QString::fromUtf8("المشاريع"), QStringLiteral("projects category missing"));
        require(pages, QStringLiteral("settings pages missing"));
        require(pages && pages->layoutDirection() == Qt::RightToLeft, QStringLiteral("settings pages are not RTL"));
        require(dialog->findChild<QWidget *>(QStringLiteral("editorSettingsPage")), QStringLiteral("editor settings page missing"));
        require(fontFamily, QStringLiteral("font family combo missing"));
        require(fontFamily && fontFamily->layoutDirection() == Qt::RightToLeft, QStringLiteral("font family combo is not RTL"));
        require(fontFamily && fontFamily->count() > 0, QStringLiteral("font family combo is empty"));
        require(fontFamily && fontFamily->findText(QStringLiteral("Cascadia Code")) == -1, QStringLiteral("font family combo contains non-Arabic Cascadia Code"));
        require(nativeFontPreview == nullptr, QStringLiteral("native font preview combo returned"));
        require(dialog->findChild<QLineEdit *>(QStringLiteral("editorFontFamilyInput")) == nullptr, QStringLiteral("old font text input returned"));
        require(dialog->findChild<QWidget *>(QStringLiteral("runtimeDiagnosticsPage")), QStringLiteral("runtime diagnostics page missing"));
        require(dialog->findChild<QWidget *>(QStringLiteral("recentProjectsPage")), QStringLiteral("recent projects page missing"));
        require(pythonPath && !pythonPath->text().isEmpty(), QStringLiteral("runtime python path missing"));
        require(packageStatus
            && (packageStatus->text().contains(QString::fromUtf8("جاهز"))
                || packageStatus->text().contains(QString::fromUtf8("غير متوفر"))),
            QStringLiteral("runtime package status missing"));
        if (fontFamily) {
            for (int i = 0; i < fontFamily->count(); ++i) {
                if (!QFontDatabase::writingSystems(fontFamily->itemText(i)).contains(QFontDatabase::Arabic)) {
                    inspected = false;
                    failure = QStringLiteral("font list contains non-Arabic-capable font: %1").arg(fontFamily->itemText(i));
                    break;
                }
            }
        }
        dialog->reject();
    });

    QTRY_VERIFY2(inspected, qPrintable(failure));
}

void TestMainWindow::settingsDialogAppliesEditorFontVisibly()
{
    MainWindow window;
    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);

    QString selectedFamily;
    int selectedSize = 18;

    QTimer::singleShot(0, &window, [&window]() {
        QMetaObject::invokeMethod(&window, "openSettings", Qt::DirectConnection);
    });
    QTimer::singleShot(150, &window, [&selectedFamily, selectedSize]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog != nullptr);

        auto *fontFamily = dialog->findChild<QComboBox *>(QStringLiteral("editorFontFamilyCombo"));
        auto *fontSize = dialog->findChild<QSpinBox *>(QStringLiteral("editorFontSizeInput"));
        auto *buttons = dialog->findChild<QDialogButtonBox *>();
        QVERIFY(fontFamily != nullptr);
        QVERIFY(fontSize != nullptr);
        QVERIFY(buttons != nullptr);

        int targetIndex = qMin(1, fontFamily->count() - 1);
        QVERIFY(targetIndex >= 0);
        fontFamily->setCurrentIndex(targetIndex);
        selectedFamily = fontFamily->currentText();
        QVERIFY(QFontDatabase::writingSystems(selectedFamily).contains(QFontDatabase::Arabic));
        fontSize->setValue(selectedSize);
        buttons->button(QDialogButtonBox::Ok)->click();
    });

    QTRY_VERIFY(!selectedFamily.isEmpty());
    QCOMPARE(editor->font().family(), selectedFamily);
    QCOMPARE(editor->font().pointSize(), selectedSize);
    QVERIFY2(editor->styleSheet().contains(QStringLiteral("font-family")),
        qPrintable(editor->styleSheet()));
    QVERIFY2(editor->styleSheet().contains(selectedFamily),
        qPrintable(editor->styleSheet()));
    QVERIFY2(editor->styleSheet().contains(QStringLiteral("font-size: 18pt")),
        qPrintable(editor->styleSheet()));
}

QTEST_MAIN(TestMainWindow)
#include "TestMainWindow.moc"
