#include <QtTest/QtTest>

#include "MainWindow.h"

#include <QClipboard>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QMenuBar>
#include <QMessageBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTextCursor>

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
    void commandPaletteExposesRegisteredWorkbenchCommands();
    void coreCommandSurfacesDeclareRegisteredCommandIds();
    void commandPaletteIncludesInFileFindCommand();
    void commandPaletteIncludesSnippetCommand();
    void commandPaletteIncludesVisibleWhitespaceCommand();
    void commandPaletteFiltersAndExecutesSelectedCommand();
    void insertPrintSnippetPlacesCursorInsideQuotes();
    void toggleVisibleWhitespaceUpdatesActiveEditor();
    void inFileFindPanelNavigatesAndReplacesActiveEditor();
    void newFileClearsCurrentPathAndEditorText();
    void newFileCreatesANewEditorTab();
    void dirtyBufferCancelPreventsNewFile();
    void dirtyBufferCancelPreventsProjectSwitch();
    void openingMultipleFilesKeepsEachDocumentInATab();
    void projectTreeShowsOnlyFileNames();
    void projectTreeExposesRtlContextActions();
    void projectTreeOpenActionOpensSelectedFile();
    void projectTreeCopyPathActionCopiesSelectedPath();
    void projectSearchShowsClickableResultRows();
    void projectSearchFindsCurrentUnsavedEditorImmediately();
    void projectReplacePreviewRendersRowsWithoutWritingFile();
    void projectReplacePreviewRowCheckboxesTrackAcceptedState();
    void projectReplacePreviewFileButtonsToggleRowsForThatFile();
    void projectReplaceApplyWritesCheckedRowsOnly();
    void projectReplaceApplyRefusesDirtyOpenBuffers();
    void projectSearchResultRowsFillRtlViewport();
    void projectSearchResultRowsHaveReadableHeight();
    void problemsPanelShowsHiddenBidiWarnings();
    void problemsPanelOpensHiddenBidiDiagnosticLine();
    void problemsPanelShowsRuntimeFailureRows();
    void outputPanelIsVisibleForRunFeedback();
    void outputPlaceholderPaintsFromRight();
    void untitledEditorBufferMaterializesForRunWithoutSaveDialog();
    void runCurrentDirtySavedFileSavesBeforeRuntime();
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
        QStringLiteral("topMenuToolsButton"),
        QStringLiteral("topMenuHelpButton"),
    };
    const QStringList menuObjectNames = {
        QStringLiteral("fileMenu"),
        QStringLiteral("editMenu"),
        QStringLiteral("viewMenu"),
        QStringLiteral("toolsMenu"),
        QStringLiteral("helpMenu"),
    };
    const QStringList menuButtonTexts = {
        QString::fromUtf8("ملف"),
        QString::fromUtf8("تحرير"),
        QString::fromUtf8("عرض"),
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
    QCOMPARE(runButton->defaultAction()->shortcut(), QKeySequence(QStringLiteral("F5")));
    QVERIFY(!runButton->icon().isNull());
    const QImage runIcon = runButton->icon().pixmap(24, 24).toImage();
    bool hasReadablePlayPixel = false;
    for (int y = 0; y < runIcon.height() && !hasReadablePlayPixel; ++y) {
        for (int x = 0; x < runIcon.width(); ++x) {
            const QColor color = runIcon.pixelColor(x, y);
            if (color.alpha() > 0 && color.lightness() > 150) {
                hasReadablePlayPixel = true;
                break;
            }
        }
    }
    QVERIFY(hasReadablePlayPixel);

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
        QStringLiteral("topMenuRunButton"),
        QStringLiteral("topMenuSearchButton"),
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

void TestMainWindow::commandPaletteExposesRegisteredWorkbenchCommands()
{
    MainWindow window;
    QStringList commandIds;

    QTimer::singleShot(0, this, [&]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }

        auto *commands = dialog->findChild<QListWidget *>(QStringLiteral("commandPaletteResults"));
        if (!commands) {
            dialog->reject();
            return;
        }

        for (int row = 0; row < commands->count(); ++row) {
            commandIds.append(commands->item(row)->data(Qt::UserRole).toString());
        }
        dialog->reject();
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "openCommandPalette", Qt::DirectConnection));

    commandIds.sort();
    const QStringList expectedIds = {
        QStringLiteral("command-palette"),
        QStringLiteral("document.closeWithPrompt"),
        QStringLiteral("document.revert"),
        QStringLiteral("document.save"),
        QStringLiteral("document.saveAll"),
        QStringLiteral("editor.toggleVisibleWhitespace"),
        QStringLiteral("find-in-file"),
        QStringLiteral("format-current-file"),
        QStringLiteral("lint-current-file"),
        QStringLiteral("new-file"),
        QStringLiteral("open-file"),
        QStringLiteral("open-project"),
        QStringLiteral("project.file.new"),
        QStringLiteral("project.folder.new"),
        QStringLiteral("project.item.copyPath"),
        QStringLiteral("project.item.deleteWithPrompt"),
        QStringLiteral("project.item.open"),
        QStringLiteral("project.item.openContainingFolder"),
        QStringLiteral("project.item.rename"),
        QStringLiteral("project.item.reveal"),
        QStringLiteral("project.refresh"),
        QStringLiteral("replace-in-project"),
        QStringLiteral("replace-in-project.applyAccepted"),
        QStringLiteral("run-current-file"),
        QStringLiteral("save-as"),
        QStringLiteral("save-file"),
        QStringLiteral("search-project"),
        QStringLiteral("settings"),
        QStringLiteral("snippet.insertPrint"),
        QStringLiteral("stop-run"),
    };
    QCOMPARE(commandIds, expectedIds);
}

void TestMainWindow::coreCommandSurfacesDeclareRegisteredCommandIds()
{
    MainWindow window;
    QStringList paletteIds;

    QTimer::singleShot(0, this, [&]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }

        auto *commands = dialog->findChild<QListWidget *>(QStringLiteral("commandPaletteResults"));
        if (!commands) {
            dialog->reject();
            return;
        }

        for (int row = 0; row < commands->count(); ++row) {
            paletteIds.append(commands->item(row)->data(Qt::UserRole).toString());
        }
        dialog->reject();
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "openCommandPalette", Qt::DirectConnection));

    QStringList surfaceIds;
    for (auto *action : window.findChildren<QAction *>()) {
        const QString commandId = action->property("commandId").toString();
        if (!commandId.isEmpty() && !surfaceIds.contains(commandId)) {
            surfaceIds.append(commandId);
        }
    }

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    QVERIFY(commandBox != nullptr);
    surfaceIds.append(commandBox->property("commandId").toString());
    auto *projectReplaceInput = window.findChild<QLineEdit *>(QStringLiteral("projectReplaceInput"));
    QVERIFY(projectReplaceInput != nullptr);
    surfaceIds.append(projectReplaceInput->property("commandId").toString());
    auto *projectReplaceApplyButton = window.findChild<QPushButton *>(QStringLiteral("projectReplaceApplyButton"));
    QVERIFY(projectReplaceApplyButton != nullptr);
    surfaceIds.append(projectReplaceApplyButton->property("commandId").toString());
    surfaceIds.removeDuplicates();
    surfaceIds.sort();

    const QStringList expectedSurfaceIds = {
        QStringLiteral("command-palette"),
        QStringLiteral("editor.toggleVisibleWhitespace"),
        QStringLiteral("find-in-file"),
        QStringLiteral("format-current-file"),
        QStringLiteral("lint-current-file"),
        QStringLiteral("new-file"),
        QStringLiteral("open-file"),
        QStringLiteral("open-project"),
        QStringLiteral("project.file.new"),
        QStringLiteral("project.folder.new"),
        QStringLiteral("project.item.copyPath"),
        QStringLiteral("project.item.deleteWithPrompt"),
        QStringLiteral("project.item.open"),
        QStringLiteral("project.item.openContainingFolder"),
        QStringLiteral("project.item.rename"),
        QStringLiteral("project.item.reveal"),
        QStringLiteral("project.refresh"),
        QStringLiteral("replace-in-project"),
        QStringLiteral("replace-in-project.applyAccepted"),
        QStringLiteral("run-current-file"),
        QStringLiteral("save-as"),
        QStringLiteral("save-file"),
        QStringLiteral("search-project"),
        QStringLiteral("settings"),
        QStringLiteral("snippet.insertPrint"),
        QStringLiteral("stop-run"),
    };
    QCOMPARE(surfaceIds, expectedSurfaceIds);

    for (const QString &surfaceId : surfaceIds) {
        QVERIFY2(paletteIds.contains(surfaceId), qPrintable(QStringLiteral("Missing command registry entry for %1").arg(surfaceId)));
    }
}

void TestMainWindow::commandPaletteIncludesInFileFindCommand()
{
    MainWindow window;
    QStringList commandIds;

    QTimer::singleShot(0, this, [&]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }

        auto *commands = dialog->findChild<QListWidget *>(QStringLiteral("commandPaletteResults"));
        if (!commands) {
            dialog->reject();
            return;
        }

        for (int row = 0; row < commands->count(); ++row) {
            commandIds.append(commands->item(row)->data(Qt::UserRole).toString());
        }
        dialog->reject();
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "openCommandPalette", Qt::DirectConnection));
    QVERIFY(commandIds.contains(QStringLiteral("find-in-file")));
}

void TestMainWindow::commandPaletteIncludesSnippetCommand()
{
    MainWindow window;
    QStringList commandIds;

    QTimer::singleShot(0, this, [&]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }

        auto *commands = dialog->findChild<QListWidget *>(QStringLiteral("commandPaletteResults"));
        if (!commands) {
            dialog->reject();
            return;
        }

        for (int row = 0; row < commands->count(); ++row) {
            commandIds.append(commands->item(row)->data(Qt::UserRole).toString());
        }
        dialog->reject();
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "openCommandPalette", Qt::DirectConnection));
    QVERIFY(commandIds.contains(QStringLiteral("snippet.insertPrint")));
}

void TestMainWindow::commandPaletteIncludesVisibleWhitespaceCommand()
{
    MainWindow window;
    QStringList commandIds;

    QTimer::singleShot(0, this, [&]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }

        auto *commands = dialog->findChild<QListWidget *>(QStringLiteral("commandPaletteResults"));
        if (!commands) {
            dialog->reject();
            return;
        }

        for (int row = 0; row < commands->count(); ++row) {
            commandIds.append(commands->item(row)->data(Qt::UserRole).toString());
        }
        dialog->reject();
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "openCommandPalette", Qt::DirectConnection));
    QVERIFY(commandIds.contains(QStringLiteral("editor.toggleVisibleWhitespace")));
}

void TestMainWindow::commandPaletteFiltersAndExecutesSelectedCommand()
{
    MainWindow window;
    window.show();

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("اطبع(\"قبل\")\n"));

    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("editorTabs"));
    QVERIFY(tabs != nullptr);
    const int beforeTabCount = tabs->count();

    bool sawDialog = false;
    bool sawRtlDialog = false;
    bool sawFocusedInput = false;
    bool sawCommandMetadata = true;
    bool sawCommandWidgets = true;
    int commandCount = 0;
    int visibleRows = 0;
    QString visibleCommandId;

    QTimer::singleShot(0, this, [&]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        sawDialog = dialog != nullptr;
        if (!dialog) {
            return;
        }
        sawRtlDialog = dialog->layoutDirection() == Qt::RightToLeft;

        auto *input = dialog->findChild<QLineEdit *>(QStringLiteral("commandPaletteInput"));
        auto *commands = dialog->findChild<QListWidget *>(QStringLiteral("commandPaletteResults"));
        if (!input || !commands) {
            dialog->reject();
            return;
        }

        sawFocusedInput = dialog->focusWidget() == input || input->hasFocus();
        commandCount = commands->count();

        for (int row = 0; row < commands->count(); ++row) {
            auto *item = commands->item(row);
            sawCommandMetadata = sawCommandMetadata
                && item
                && !item->data(Qt::UserRole).toString().isEmpty()
                && !item->data(Qt::UserRole + 1).toString().isEmpty();
            sawCommandWidgets = sawCommandWidgets && item && commands->itemWidget(item) != nullptr;
        }

        input->setText(QKeySequence(QKeySequence::New).toString(QKeySequence::NativeText));
        QCoreApplication::processEvents();

        int visibleRow = -1;
        for (int row = 0; row < commands->count(); ++row) {
            if (!commands->item(row)->isHidden()) {
                ++visibleRows;
                visibleRow = row;
            }
        }
        if (visibleRow >= 0) {
            visibleCommandId = commands->item(visibleRow)->data(Qt::UserRole).toString();
        }

        commands->setCurrentRow(visibleRow);
        QTest::keyClick(input, Qt::Key_Return);
        if (dialog->isVisible()) {
            dialog->reject();
        }
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "openCommandPalette", Qt::DirectConnection));

    QVERIFY(sawDialog);
    QVERIFY(sawRtlDialog);
    QVERIFY(sawFocusedInput);
    QVERIFY(commandCount >= 9);
    QVERIFY(sawCommandMetadata);
    QVERIFY(sawCommandWidgets);
    QCOMPARE(visibleRows, 1);
    QCOMPARE(visibleCommandId, QStringLiteral("new-file"));
    QCOMPARE(tabs->count(), beforeTabCount + 1);
    QCOMPARE(window.currentEditorPath(), QString());
    auto *currentEditor = qobject_cast<EditorSurface *>(tabs->currentWidget());
    QVERIFY(currentEditor != nullptr);
    QCOMPARE(currentEditor->toPlainText(), QString());

    int doubleClickVisibleRows = 0;
    QString doubleClickCommandId;

    QTimer::singleShot(0, this, [&]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }

        auto *input = dialog->findChild<QLineEdit *>(QStringLiteral("commandPaletteInput"));
        auto *commands = dialog->findChild<QListWidget *>(QStringLiteral("commandPaletteResults"));
        if (!input || !commands) {
            dialog->reject();
            return;
        }

        input->setText(QKeySequence(QKeySequence::New).toString(QKeySequence::NativeText));
        QCoreApplication::processEvents();

        int visibleRow = -1;
        for (int row = 0; row < commands->count(); ++row) {
            if (!commands->item(row)->isHidden()) {
                ++doubleClickVisibleRows;
                visibleRow = row;
            }
        }
        if (visibleRow >= 0) {
            doubleClickCommandId = commands->item(visibleRow)->data(Qt::UserRole).toString();
            commands->setCurrentRow(visibleRow);
            commands->itemDoubleClicked(commands->item(visibleRow));
        }
        if (dialog->isVisible()) {
            dialog->reject();
        }
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "openCommandPalette", Qt::DirectConnection));

    QCOMPARE(doubleClickVisibleRows, 1);
    QCOMPARE(doubleClickCommandId, QStringLiteral("new-file"));
    QCOMPARE(tabs->count(), beforeTabCount + 2);
}

void TestMainWindow::insertPrintSnippetPlacesCursorInsideQuotes()
{
    MainWindow window;

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("قبل\n"));
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    editor->setTextCursor(cursor);

    const int insertionStart = cursor.position();
    QVERIFY(QMetaObject::invokeMethod(&window, "insertPrintSnippet", Qt::DirectConnection));

    QCOMPARE(editor->toPlainText(), QString::fromUtf8("قبل\nاطبع(\"\")"));
    QCOMPARE(editor->textCursor().position(), insertionStart + QString::fromUtf8("اطبع(\"").size());
}

void TestMainWindow::toggleVisibleWhitespaceUpdatesActiveEditor()
{
    MainWindow window;

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QVERIFY(editor->isVisibleWhitespaceEnabled());

    QVERIFY(QMetaObject::invokeMethod(&window, "toggleVisibleWhitespace", Qt::DirectConnection));
    QVERIFY(!editor->isVisibleWhitespaceEnabled());

    QVERIFY(QMetaObject::invokeMethod(&window, "toggleVisibleWhitespace", Qt::DirectConnection));
    QVERIFY(editor->isVisibleWhitespaceEnabled());
}

void TestMainWindow::inFileFindPanelNavigatesAndReplacesActiveEditor()
{
    MainWindow window;
    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    QVERIFY(QMetaObject::invokeMethod(&window, "openInFileFind", Qt::DirectConnection));

    auto *panel = window.findChild<QWidget *>(QStringLiteral("inFileFindPanel"));
    auto *findInput = window.findChild<QLineEdit *>(QStringLiteral("inFileFindInput"));
    auto *replaceInput = window.findChild<QLineEdit *>(QStringLiteral("inFileReplaceInput"));
    auto *nextButton = window.findChild<QPushButton *>(QStringLiteral("inFileFindNextButton"));
    auto *replaceButton = window.findChild<QPushButton *>(QStringLiteral("inFileReplaceButton"));
    auto *replaceAllButton = window.findChild<QPushButton *>(QStringLiteral("inFileReplaceAllButton"));
    auto *status = window.findChild<QLabel *>(QStringLiteral("inFileFindStatusLabel"));

    QVERIFY(panel != nullptr);
    QVERIFY(!panel->isHidden());
    QVERIFY(findInput != nullptr);
    QVERIFY(replaceInput != nullptr);
    QVERIFY(nextButton != nullptr);
    QVERIFY(replaceButton != nullptr);
    QVERIFY(replaceAllButton != nullptr);
    QVERIFY(status != nullptr);

    findInput->setText(QString::fromUtf8("عدد"));
    QVERIFY(status->text().contains(QStringLiteral("2")));

    nextButton->click();
    replaceInput->setText(QString::fromUtf8("قيمة"));
    replaceButton->click();
    QCOMPARE(editor->toPlainText(), QString::fromUtf8("عدد = 1\nاطبع(قيمة)\n"));

    replaceAllButton->click();
    QCOMPARE(editor->toPlainText(), QString::fromUtf8("قيمة = 1\nاطبع(قيمة)\n"));
    QVERIFY(status->text().contains(QStringLiteral("0")));
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

void TestMainWindow::dirtyBufferCancelPreventsNewFile()
{
    MainWindow window;
    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->insertPlainText(QString::fromUtf8("عدد = 1\n"));
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
    editor->insertPlainText(QString::fromUtf8("عدد = 1\n"));
    QVERIFY(editor->isDirty());

    QTimer::singleShot(0, []() {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box != nullptr);
        box->button(QMessageBox::Cancel)->click();
    });

    QVERIFY(!window.openPath(other.path()));
    QCOMPARE(QDir::toNativeSeparators(window.currentProjectRoot()), QDir::toNativeSeparators(temp.path()));
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

void TestMainWindow::projectTreeExposesRtlContextActions()
{
    MainWindow window;

    auto *tree = window.findChild<QTreeView *>(QStringLiteral("projectTree"));
    QVERIFY(tree != nullptr);
    QCOMPARE(tree->contextMenuPolicy(), Qt::CustomContextMenu);
    QCOMPARE(tree->layoutDirection(), Qt::RightToLeft);

    auto *newFile = window.findChild<QAction *>(QStringLiteral("projectTreeNewFileAction"));
    auto *newFolder = window.findChild<QAction *>(QStringLiteral("projectTreeNewFolderAction"));
    auto *open = window.findChild<QAction *>(QStringLiteral("projectTreeOpenAction"));
    auto *rename = window.findChild<QAction *>(QStringLiteral("projectTreeRenameAction"));
    auto *deleteAction = window.findChild<QAction *>(QStringLiteral("projectTreeDeleteAction"));
    auto *reveal = window.findChild<QAction *>(QStringLiteral("projectTreeRevealAction"));
    auto *copyPath = window.findChild<QAction *>(QStringLiteral("projectTreeCopyPathAction"));
    auto *openContaining = window.findChild<QAction *>(QStringLiteral("projectTreeOpenContainingFolderAction"));
    auto *refresh = window.findChild<QAction *>(QStringLiteral("projectTreeRefreshAction"));

    QVERIFY(newFile != nullptr);
    QVERIFY(newFolder != nullptr);
    QVERIFY(open != nullptr);
    QVERIFY(rename != nullptr);
    QVERIFY(deleteAction != nullptr);
    QVERIFY(reveal != nullptr);
    QVERIFY(copyPath != nullptr);
    QVERIFY(openContaining != nullptr);
    QVERIFY(refresh != nullptr);

    QCOMPARE(newFile->text(), QString::fromUtf8("ملف جديد"));
    QCOMPARE(newFolder->text(), QString::fromUtf8("مجلد جديد"));
    QCOMPARE(open->text(), QString::fromUtf8("فتح"));
    QCOMPARE(rename->text(), QString::fromUtf8("إعادة تسمية"));
    QCOMPARE(deleteAction->text(), QString::fromUtf8("حذف"));
    QCOMPARE(reveal->text(), QString::fromUtf8("إظهار في مستكشف الملفات"));
    QCOMPARE(copyPath->text(), QString::fromUtf8("نسخ المسار"));
    QCOMPARE(openContaining->text(), QString::fromUtf8("فتح المجلد الحاوي"));
    QCOMPARE(refresh->text(), QString::fromUtf8("تحديث"));

    QCOMPARE(newFile->property("commandId").toString(), QStringLiteral("project.file.new"));
    QCOMPARE(newFolder->property("commandId").toString(), QStringLiteral("project.folder.new"));
    QCOMPARE(open->property("commandId").toString(), QStringLiteral("project.item.open"));
    QCOMPARE(rename->property("commandId").toString(), QStringLiteral("project.item.rename"));
    QCOMPARE(deleteAction->property("commandId").toString(), QStringLiteral("project.item.deleteWithPrompt"));
    QCOMPARE(reveal->property("commandId").toString(), QStringLiteral("project.item.reveal"));
    QCOMPARE(copyPath->property("commandId").toString(), QStringLiteral("project.item.copyPath"));
    QCOMPARE(openContaining->property("commandId").toString(), QStringLiteral("project.item.openContainingFolder"));
    QCOMPARE(refresh->property("commandId").toString(), QStringLiteral("project.refresh"));
}

void TestMainWindow::projectTreeOpenActionOpensSelectedFile()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("اطبع(\"من الشجرة\")\n"));

    MainWindow window;
    QVERIFY(window.openPath(root.absolutePath()));

    auto *tree = window.findChild<QTreeView *>(QStringLiteral("projectTree"));
    auto *model = qobject_cast<QFileSystemModel *>(tree ? tree->model() : nullptr);
    auto *open = window.findChild<QAction *>(QStringLiteral("projectTreeOpenAction"));
    QVERIFY(tree != nullptr);
    QVERIFY(model != nullptr);
    QVERIFY(open != nullptr);

    QModelIndex fileIndex;
    QTRY_VERIFY((fileIndex = model->index(filePath)).isValid());
    tree->setCurrentIndex(fileIndex);

    open->trigger();

    QCOMPARE(QDir::toNativeSeparators(window.currentEditorPath()), QDir::toNativeSeparators(filePath));
}

void TestMainWindow::projectTreeCopyPathActionCopiesSelectedPath()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("copy-target.apy"), QString::fromUtf8("اطبع(\"مسار\")\n"));

    MainWindow window;
    QVERIFY(window.openPath(root.absolutePath()));

    auto *tree = window.findChild<QTreeView *>(QStringLiteral("projectTree"));
    auto *model = qobject_cast<QFileSystemModel *>(tree ? tree->model() : nullptr);
    auto *copyPath = window.findChild<QAction *>(QStringLiteral("projectTreeCopyPathAction"));
    QVERIFY(tree != nullptr);
    QVERIFY(model != nullptr);
    QVERIFY(copyPath != nullptr);
    QVERIFY(QApplication::clipboard() != nullptr);

    QApplication::clipboard()->clear();

    QModelIndex fileIndex;
    QTRY_VERIFY((fileIndex = model->index(filePath)).isValid());
    tree->setCurrentIndex(fileIndex);

    copyPath->trigger();

    QCOMPARE(QApplication::clipboard()->text(), QDir::toNativeSeparators(filePath));
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
    auto *detailLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultDetailLabel"));
    QVERIFY(fileLabel != nullptr);
    QVERIFY(lineLabel != nullptr);
    QVERIFY(detailLabel != nullptr);
    QCOMPARE(fileLabel->text(), QDir::toNativeSeparators(filePath));
    QCOMPARE(lineLabel->text(), QString::fromUtf8("السطر 2"));
    QCOMPARE(detailLabel->text(), QString::fromUtf8("مطابقة واحدة - انقر للفتح"));
    QCOMPARE(fileLabel->alignment() & Qt::AlignRight, Qt::AlignRight);
    QCOMPARE(detailLabel->alignment() & Qt::AlignRight, Qt::AlignRight);
    QCOMPARE(detailLabel->layoutDirection(), Qt::RightToLeft);
    QVERIFY(resultRow->findChild<QWidget *>(QStringLiteral("searchResultPreviewText")) == nullptr);
    QCOMPARE(QDir::toNativeSeparators(results->item(0)->data(Qt::UserRole).toString()), QDir::toNativeSeparators(filePath));
    QCOMPARE(results->item(0)->data(Qt::UserRole + 1).toInt(), 2);
    QVERIFY(results->item(0)->toolTip().contains(QString::fromUtf8("اطبع")));

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
    auto *metadataCluster = resultRow->findChild<QWidget *>(QStringLiteral("searchResultMetadataCluster"));
    QVERIFY(metadataCluster != nullptr);
    QCOMPARE(metadataCluster->layoutDirection(), Qt::RightToLeft);
    auto *fileLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultFileLabel"));
    auto *lineLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultLineLabel"));
    auto *detailLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultDetailLabel"));
    QVERIFY(fileLabel != nullptr);
    QVERIFY(lineLabel != nullptr);
    QVERIFY(detailLabel != nullptr);
    QCOMPARE(fileLabel->text(), QString::fromUtf8("المحرر الحالي"));
    QCOMPARE(lineLabel->text(), QString::fromUtf8("السطر 3"));
    QCOMPARE(detailLabel->text(), QString::fromUtf8("مطابقة واحدة - انقر للفتح"));
    QVERIFY(resultRow->findChild<QWidget *>(QStringLiteral("searchResultPreviewText")) == nullptr);
    QCOMPARE(results->item(0)->data(Qt::UserRole).toString(), QString());
    QCOMPARE(results->item(0)->data(Qt::UserRole + 1).toInt(), 3);
    QVERIFY(results->item(0)->toolTip().contains(QStringLiteral("adult")));
}

void TestMainWindow::projectReplacePreviewRendersRowsWithoutWritingFile()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    MainWindow window;
    QVERIFY(window.openPath(root.absolutePath()));

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    auto *replaceInput = window.findChild<QLineEdit *>(QStringLiteral("projectReplaceInput"));
    QVERIFY(commandBox != nullptr);
    QVERIFY(replaceInput != nullptr);
    QCOMPARE(replaceInput->property("commandId").toString(), QStringLiteral("replace-in-project"));

    commandBox->setText(QString::fromUtf8("عدد"));
    replaceInput->setText(QString::fromUtf8("قيمة"));

    QVERIFY(QMetaObject::invokeMethod(&window, "previewProjectReplace", Qt::DirectConnection));

    auto *results = window.findChild<QListWidget *>(QStringLiteral("searchResultsPanel"));
    QVERIFY(results != nullptr);
    QCOMPARE(results->count(), 2);

    auto *resultRow = results->itemWidget(results->item(0));
    QVERIFY(resultRow != nullptr);
    auto *detailLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultDetailLabel"));
    auto *beforeLabel = resultRow->findChild<QLabel *>(QStringLiteral("projectReplaceBeforeLabel"));
    auto *afterLabel = resultRow->findChild<QLabel *>(QStringLiteral("projectReplaceAfterLabel"));
    QVERIFY(detailLabel != nullptr);
    QVERIFY(beforeLabel != nullptr);
    QVERIFY(afterLabel != nullptr);
    QCOMPARE(detailLabel->text(), QString::fromUtf8("معاينة استبدال فقط - لن يتم تعديل الملف"));
    QCOMPARE(beforeLabel->text(), QString::fromUtf8("عدد = 1"));
    QCOMPARE(afterLabel->text(), QString::fromUtf8("قيمة = 1"));
    QCOMPARE(QDir::toNativeSeparators(results->item(0)->data(Qt::UserRole).toString()), QDir::toNativeSeparators(filePath));
    QCOMPARE(results->item(0)->data(Qt::UserRole + 1).toInt(), 1);
    QCOMPARE(results->item(0)->data(Qt::UserRole + 3).toString(), QString::fromUtf8("قيمة = 1"));

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString diskText = QString::fromUtf8(file.readAll());
    diskText.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    QCOMPARE(diskText, QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));
}

void TestMainWindow::projectReplacePreviewRowCheckboxesTrackAcceptedState()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    MainWindow window;
    QVERIFY(window.openPath(root.absolutePath()));

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    auto *replaceInput = window.findChild<QLineEdit *>(QStringLiteral("projectReplaceInput"));
    QVERIFY(commandBox != nullptr);
    QVERIFY(replaceInput != nullptr);
    commandBox->setText(QString::fromUtf8("عدد"));
    replaceInput->setText(QString::fromUtf8("قيمة"));

    QVERIFY(QMetaObject::invokeMethod(&window, "previewProjectReplace", Qt::DirectConnection));

    auto *results = window.findChild<QListWidget *>(QStringLiteral("searchResultsPanel"));
    QVERIFY(results != nullptr);
    QCOMPARE(results->count(), 2);

    auto *firstRow = results->itemWidget(results->item(0));
    QVERIFY(firstRow != nullptr);
    auto *accept = firstRow->findChild<QCheckBox *>(QStringLiteral("projectReplaceAcceptCheckBox"));
    QVERIFY(accept != nullptr);
    QVERIFY(accept->isChecked());
    QCOMPARE(results->item(0)->data(Qt::UserRole + 4).toBool(), true);

    accept->setChecked(false);
    QCOMPARE(results->item(0)->data(Qt::UserRole + 4).toBool(), false);

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString diskText = QString::fromUtf8(file.readAll());
    diskText.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    QCOMPARE(diskText, QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));
}

void TestMainWindow::projectReplacePreviewFileButtonsToggleRowsForThatFile()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));
    writeFile(root, QStringLiteral("other.apy"), QString::fromUtf8("اطبع(عدد)\n"));

    MainWindow window;
    QVERIFY(window.openPath(root.absolutePath()));

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    auto *replaceInput = window.findChild<QLineEdit *>(QStringLiteral("projectReplaceInput"));
    QVERIFY(commandBox != nullptr);
    QVERIFY(replaceInput != nullptr);
    commandBox->setText(QString::fromUtf8("عدد"));
    replaceInput->setText(QString::fromUtf8("قيمة"));

    QVERIFY(QMetaObject::invokeMethod(&window, "previewProjectReplace", Qt::DirectConnection));

    auto *results = window.findChild<QListWidget *>(QStringLiteral("searchResultsPanel"));
    QVERIFY(results != nullptr);
    QCOMPARE(results->count(), 3);

    int mainRow = -1;
    int otherRow = -1;
    for (int row = 0; row < results->count(); ++row) {
        const QString path = results->item(row)->data(Qt::UserRole).toString();
        if (path.endsWith(QStringLiteral("main.apy")) && mainRow < 0) {
            mainRow = row;
        }
        if (path.endsWith(QStringLiteral("other.apy"))) {
            otherRow = row;
        }
    }
    QVERIFY(mainRow >= 0);
    QVERIFY(otherRow >= 0);

    results->setCurrentRow(mainRow);
    auto *rowWidget = results->itemWidget(results->item(mainRow));
    QVERIFY(rowWidget != nullptr);
    auto *rejectFile = rowWidget->findChild<QPushButton *>(QStringLiteral("projectReplaceRejectFileButton"));
    auto *acceptFile = rowWidget->findChild<QPushButton *>(QStringLiteral("projectReplaceAcceptFileButton"));
    QVERIFY(rejectFile != nullptr);
    QVERIFY(acceptFile != nullptr);

    rejectFile->click();
    for (int row = 0; row < results->count(); ++row) {
        const QString path = results->item(row)->data(Qt::UserRole).toString();
        if (path.endsWith(QStringLiteral("main.apy"))) {
            QCOMPARE(results->item(row)->data(Qt::UserRole + 4).toBool(), false);
            auto *checkbox = results->itemWidget(results->item(row))->findChild<QCheckBox *>(QStringLiteral("projectReplaceAcceptCheckBox"));
            QVERIFY(checkbox != nullptr);
            QCOMPARE(checkbox->isChecked(), false);
        }
    }
    QCOMPARE(results->item(otherRow)->data(Qt::UserRole + 4).toBool(), true);

    acceptFile->click();
    for (int row = 0; row < results->count(); ++row) {
        const QString path = results->item(row)->data(Qt::UserRole).toString();
        if (path.endsWith(QStringLiteral("main.apy"))) {
            QCOMPARE(results->item(row)->data(Qt::UserRole + 4).toBool(), true);
        }
    }
}

void TestMainWindow::projectReplaceApplyWritesCheckedRowsOnly()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    MainWindow window;
    QVERIFY(window.openPath(root.absolutePath()));

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    auto *replaceInput = window.findChild<QLineEdit *>(QStringLiteral("projectReplaceInput"));
    auto *applyButton = window.findChild<QPushButton *>(QStringLiteral("projectReplaceApplyButton"));
    QVERIFY(commandBox != nullptr);
    QVERIFY(replaceInput != nullptr);
    QVERIFY(applyButton != nullptr);
    QCOMPARE(applyButton->property("commandId").toString(), QStringLiteral("replace-in-project.applyAccepted"));

    commandBox->setText(QString::fromUtf8("عدد"));
    replaceInput->setText(QString::fromUtf8("قيمة"));
    QVERIFY(QMetaObject::invokeMethod(&window, "previewProjectReplace", Qt::DirectConnection));

    auto *results = window.findChild<QListWidget *>(QStringLiteral("searchResultsPanel"));
    QVERIFY(results != nullptr);
    QCOMPARE(results->count(), 2);
    auto *secondRow = results->itemWidget(results->item(1));
    QVERIFY(secondRow != nullptr);
    auto *acceptSecond = secondRow->findChild<QCheckBox *>(QStringLiteral("projectReplaceAcceptCheckBox"));
    QVERIFY(acceptSecond != nullptr);
    acceptSecond->setChecked(false);

    QVERIFY(QMetaObject::invokeMethod(&window, "applyAcceptedProjectReplaceRows", Qt::DirectConnection));

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString diskText = QString::fromUtf8(file.readAll());
    diskText.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    QCOMPARE(diskText, QString::fromUtf8("قيمة = 1\nاطبع(عدد)\n"));
}

void TestMainWindow::projectReplaceApplyRefusesDirtyOpenBuffers()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    MainWindow window;
    QVERIFY(window.openPath(filePath));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->insertPlainText(QString::fromUtf8("# تعديل غير محفوظ\n"));
    QVERIFY(editor->isDirty());

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    auto *replaceInput = window.findChild<QLineEdit *>(QStringLiteral("projectReplaceInput"));
    QVERIFY(commandBox != nullptr);
    QVERIFY(replaceInput != nullptr);
    commandBox->setText(QString::fromUtf8("عدد"));
    replaceInput->setText(QString::fromUtf8("قيمة"));
    QVERIFY(QMetaObject::invokeMethod(&window, "previewProjectReplace", Qt::DirectConnection));

    QVERIFY(QMetaObject::invokeMethod(&window, "applyAcceptedProjectReplaceRows", Qt::DirectConnection));

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString diskText = QString::fromUtf8(file.readAll());
    diskText.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    QCOMPARE(diskText, QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));
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
    auto *lineLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultLineLabel"));
    auto *detailLabel = resultRow->findChild<QLabel *>(QStringLiteral("searchResultDetailLabel"));
    QVERIFY(fileLabel != nullptr);
    QVERIFY(lineLabel != nullptr);
    QVERIFY(detailLabel != nullptr);
    QVERIFY(resultRow->findChild<QWidget *>(QStringLiteral("searchResultPreviewText")) == nullptr);

    const QRect rowRect = resultRow->geometry();
    const int viewportWidth = results->viewport()->width();
    QVERIFY2(rowRect.width() > viewportWidth * 0.9,
        qPrintable(QStringLiteral("search result row should fill viewport width. row=%1 viewport=%2")
            .arg(rowRect.width())
            .arg(viewportWidth)));

    const QRect labelRect(fileLabel->mapTo(results->viewport(), QPoint(0, 0)), fileLabel->size());
    const QRect lineRect(lineLabel->mapTo(results->viewport(), QPoint(0, 0)), lineLabel->size());
    const int metadataGap = horizontalGap(labelRect, lineRect);
    QVERIFY2(metadataGap <= 24,
        qPrintable(QStringLiteral("search result file text should sit near the line column. gap=%1 file=[%2,%3] line=[%4,%5]")
            .arg(metadataGap)
            .arg(labelRect.left())
            .arg(labelRect.right())
            .arg(lineRect.left())
            .arg(lineRect.right())));
    QVERIFY2(labelRect.right() > viewportWidth - 260,
        qPrintable(QStringLiteral("RTL search result title should sit near the right edge. labelRight=%1 viewport=%2")
            .arg(labelRect.right())
            .arg(viewportWidth)));
    QVERIFY2(fileLabel->width() > viewportWidth * 0.55,
        qPrintable(QStringLiteral("search result file label should use the available row width. labelWidth=%1 viewport=%2")
            .arg(fileLabel->width())
            .arg(viewportWidth)));
    QVERIFY2(fileLabel->text().length() > QStringLiteral("main.apy").length(),
        "search result label should show a wide path-style location, not only a short filename");

    const QRect detailRect(detailLabel->mapTo(results->viewport(), QPoint(0, 0)), detailLabel->size());
    QVERIFY2(detailRect.right() > viewportWidth - 260,
        qPrintable(QStringLiteral("search result detail should sit under the right-side metadata. detailRight=%1 viewport=%2")
            .arg(detailRect.right())
            .arg(viewportWidth)));
    QVERIFY2(detailRect.left() > viewportWidth / 2,
        qPrintable(QStringLiteral("search result detail label should be right-anchored, not full-row left-starting. detailLeft=%1 viewport=%2")
            .arg(detailRect.left())
            .arg(viewportWidth)));

    QVERIFY2(rowRect.height() >= 72,
        qPrintable(QStringLiteral("search result row should have enough height to read comfortably. height=%1")
            .arg(rowRect.height())));
}

void TestMainWindow::projectSearchResultRowsHaveReadableHeight()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    MainWindow window;
    window.resize(1200, 800);
    QVERIFY(window.openPath(temp.path()));
    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QFont largeEditorFont = editor->font();
    largeEditorFont.setPointSize(22);
    editor->setFont(largeEditorFont);
    editor->setPlainText(QString::fromUtf8("اطبع(\"أنت adult\")\n"));

    auto *commandBox = window.findChild<QLineEdit *>(QStringLiteral("commandBox"));
    QVERIFY(commandBox != nullptr);
    commandBox->setText(QStringLiteral("adult"));
    QVERIFY(QMetaObject::invokeMethod(&window, "findInProject", Qt::DirectConnection));

    auto *results = window.findChild<QListWidget *>(QStringLiteral("searchResultsPanel"));
    QVERIFY(results != nullptr);
    QCOMPARE(results->count(), 1);

    auto *resultRow = results->itemWidget(results->item(0));
    QVERIFY(resultRow != nullptr);
    QVERIFY(resultRow->findChild<QWidget *>(QStringLiteral("searchResultPreviewText")) == nullptr);
    QVERIFY2(resultRow->height() >= 72,
        qPrintable(QStringLiteral("search result row should remain readable at larger editor fonts. height=%1")
            .arg(resultRow->height())));
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
    window.resize(1000, 700);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();

    auto *problems = window.findChild<QListWidget *>(QStringLiteral("problemsPanel"));
    QVERIFY(problems != nullptr);
    QCOMPARE(problems->count(), 1);
    QVERIFY(problems->item(0)->text().contains(QString::fromUtf8("تحكم اتجاه مخفي")));
    QVERIFY(problems->item(0)->text().contains(QStringLiteral("RIGHT-TO-LEFT OVERRIDE")));
    QCOMPARE(QDir::toNativeSeparators(problems->item(0)->data(Qt::UserRole).toString()), QDir::toNativeSeparators(filePath));
    QCOMPARE(problems->item(0)->data(Qt::UserRole + 1).toInt(), 2);

    auto *row = problems->itemWidget(problems->item(0));
    QVERIFY(row != nullptr);
    auto *severity = row->findChild<QLabel *>(QStringLiteral("problemSeverityLabel"));
    auto *location = row->findChild<QLabel *>(QStringLiteral("problemLocationLabel"));
    auto *message = row->findChild<QLabel *>(QStringLiteral("problemMessageLabel"));
    auto *messageLine = row->findChild<QWidget *>(QStringLiteral("problemMessageLine"));
    QVERIFY(severity != nullptr);
    QVERIFY(location != nullptr);
    QVERIFY(message != nullptr);
    QVERIFY(messageLine != nullptr);
    QCOMPARE(messageLine->layoutDirection(), Qt::RightToLeft);
    QCOMPARE(severity->text(), QString::fromUtf8("تحذير"));
    QVERIFY(location->text().contains(QStringLiteral("main.apy")));
    QVERIFY(location->text().contains(QString::fromUtf8("السطر 2")));
    QVERIFY(message->text().contains(QStringLiteral("RIGHT-TO-LEFT OVERRIDE")));
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
    QCOMPARE(problems->item(0)->data(Qt::UserRole + 2).toString(), QString::fromUtf8("تحذير"));
    QVERIFY(problems->item(0)->data(Qt::UserRole + 3).toString().contains(QString::fromUtf8("تحكم اتجاه مخفي")));
}

void TestMainWindow::problemsPanelOpensHiddenBidiDiagnosticLine()
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

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    editor->setTextCursor(cursor);
    QCOMPARE(editor->textCursor().blockNumber(), 0);

    problems->itemClicked(problems->item(0));

    QCOMPARE(window.currentEditorPath(), filePath);
    QCOMPARE(editor->textCursor().blockNumber(), 1);
}

void TestMainWindow::problemsPanelShowsRuntimeFailureRows()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    MainWindow window;
    window.resize(1000, 700);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.openPath(temp.path()));
    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->setPlainText(QString::fromUtf8("اطبع(\"فشل تشغيل مقصود\")\n"));

    QVERIFY(QMetaObject::invokeMethod(&window, "runCurrentFile", Qt::DirectConnection));

    auto *problems = window.findChild<QListWidget *>(QStringLiteral("problemsPanel"));
    QVERIFY(problems != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(problems->count() >= 1, 5000);

    auto *item = problems->item(problems->count() - 1);
    QVERIFY(item != nullptr);
    QCOMPARE(item->data(Qt::UserRole + 2).toString(), QString::fromUtf8("خطأ"));
    QVERIFY(item->data(Qt::UserRole + 3).toString().contains(QString::fromUtf8("تعذر بدء")));
    QVERIFY(item->data(Qt::UserRole).toString().endsWith(QStringLiteral("current-buffer.apy")));
    QCOMPARE(item->data(Qt::UserRole + 1).toInt(), 1);

    auto *row = problems->itemWidget(item);
    QVERIFY(row != nullptr);
    auto *severity = row->findChild<QLabel *>(QStringLiteral("problemSeverityLabel"));
    auto *location = row->findChild<QLabel *>(QStringLiteral("problemLocationLabel"));
    auto *message = row->findChild<QLabel *>(QStringLiteral("problemMessageLabel"));
    auto *messageLine = row->findChild<QWidget *>(QStringLiteral("problemMessageLine"));
    QVERIFY(severity != nullptr);
    QVERIFY(location != nullptr);
    QVERIFY(message != nullptr);
    QVERIFY(messageLine != nullptr);
    QCOMPARE(messageLine->layoutDirection(), Qt::RightToLeft);
    QCOMPARE(severity->text(), QString::fromUtf8("خطأ"));
    QVERIFY(location->text().contains(QStringLiteral("current-buffer.apy")));
    QVERIFY(location->text().contains(QString::fromUtf8("السطر 1")));
    QVERIFY(message->text().contains(QString::fromUtf8("تعذر بدء")));
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

    problems->itemClicked(item);

    QVERIFY(window.currentEditorPath().endsWith(QStringLiteral("current-buffer.apy")));
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("editorTabs"));
    QVERIFY(tabs != nullptr);
    auto *currentEditor = qobject_cast<EditorSurface *>(tabs->currentWidget());
    QVERIFY(currentEditor != nullptr);
    QCOMPARE(currentEditor->textCursor().blockNumber(), 0);
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

void TestMainWindow::runCurrentDirtySavedFileSavesBeforeRuntime()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\n"));

    MainWindow window;
    QVERIFY(window.openPath(filePath));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    editor->selectAll();
    editor->insertPlainText(QString::fromUtf8("عدد = 2\n"));
    QVERIFY(editor->isDirty());

    QVERIFY(QMetaObject::invokeMethod(&window, "runCurrentFile", Qt::DirectConnection));

    QFile saved(filePath);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(saved.readAll(), QString::fromUtf8("عدد = 2\n").toUtf8());
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
