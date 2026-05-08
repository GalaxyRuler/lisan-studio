#include "MainWindow.h"

#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTextStream>
#include <QToolButton>
#include <QStandardPaths>
#include <QVBoxLayout>

class ArabicPlaceholderPlainTextEdit final : public QPlainTextEdit
{
public:
    explicit ArabicPlaceholderPlainTextEdit(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
    {
    }

    void setArabicPlaceholderText(const QString &text)
    {
        placeholder = text;
        setPlaceholderText(QString());
        viewport()->update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPlainTextEdit::paintEvent(event);

        if (!toPlainText().isEmpty() || placeholder.isEmpty()) {
            return;
        }

        QPainter painter(viewport());
        painter.setPen(QColor(145, 155, 160));
        const QRect textRect = viewport()->rect().adjusted(12, 10, -12, 0);
        const int textWidth = fontMetrics().horizontalAdvance(placeholder);
        const int x = qMax(textRect.left(), textRect.right() - textWidth + 1);
        const int y = textRect.top() + fontMetrics().ascent();
        painter.drawText(x, y, placeholder);
    }

private:
    QString placeholder;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    resize(1280, 820);
    setWindowTitle(QString::fromUtf8("استوديو لسان"));
}

bool MainWindow::openPath(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        return false;
    }
    if (info.isDir()) {
        return loadProject(info.absoluteFilePath());
    }
    if (info.isFile()) {
        return openEditorFile(info.absoluteFilePath());
    }
    return false;
}

QString MainWindow::currentProjectRoot() const
{
    return projectRoot;
}

QString MainWindow::currentEditorPath() const
{
    return editor->currentFilePath();
}

QString MainWindow::materializeRunnableBuffer(QString *error)
{
    if (error) {
        error->clear();
    }

    if (!editor->currentFilePath().isEmpty()) {
        if (editor->isDirty()) {
            QString saveError;
            if (!editor->saveFile(&saveError)) {
                if (error) {
                    *error = saveError;
                }
                return QString();
            }
        }
        return editor->currentFilePath();
    }

    const QString baseDirectory = runtimeWorkingDirectory();
    if (baseDirectory.isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("تعذر تحديد مجلد التشغيل.");
        }
        return QString();
    }

    QDir runDirectory(baseDirectory);
    if (!runDirectory.mkpath(QStringLiteral(".arabic-code-studio"))) {
        if (error) {
            *error = QString::fromUtf8("تعذر إنشاء مجلد التشغيل المؤقت.");
        }
        return QString();
    }

    const QString runFilePath = runDirectory.filePath(QStringLiteral(".arabic-code-studio/current-buffer.apy"));
    QFile file(runFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return QString();
    }

    file.write(editor->toPlainText().toUtf8());
    return runFilePath;
}

void MainWindow::buildUi()
{
    setLayoutDirection(Qt::RightToLeft);

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background: #0f141a; color: #E8ECF2; font-family: 'IBM Plex Sans Arabic', 'Segoe UI'; }"
        "QMenu { background: #171B22; color: #E8ECF2; border: 1px solid #303746; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: #264F78; }"
        "QWidget#topShell { background: #111318; border-bottom: 1px solid #303746; }"
        "QWidget#topMenuRow { background: #111318; border-bottom: 1px solid #303746; }"
        "QWidget#brandBlock { background: transparent; }"
        "QLabel#brandTextLabel { color: #AEC6FF; font-weight: 600; font-size: 18px; }"
        "QToolButton[role=\"topMenu\"] { background: transparent; border: 1px solid transparent; border-radius: 4px; color: #C8D0DD; font-weight: 600; padding: 7px 10px; }"
        "QToolButton[role=\"topMenu\"]:hover { color: #E8ECF2; background: #202633; border-color: #303746; border-bottom: 2px solid #4C8DFF; }"
        "QToolButton[role=\"topMenu\"]:pressed, QToolButton[role=\"topMenu\"]:checked { color: #A7C4FF; background: #13233A; border-color: #4C8DFF; }"
        "QToolButton[role=\"topMenu\"][active=\"true\"] { color: #A7C4FF; border-bottom: 2px solid #4C8DFF; }"
        "QToolButton[role=\"topMenu\"]::menu-indicator { image: none; width: 0px; }"
        "QToolButton[role=\"primaryAction\"] { background: transparent; border: 1px solid #4C8DFF; border-radius: 12px; padding: 4px 12px; color: #7BDFF2; font-weight: 700; }"
        "QToolButton[role=\"primaryAction\"]:hover { background: #13233A; border-color: #7BDFF2; }"
        "QToolButton { background: #202633; border: 1px solid #303746; border-radius: 4px; padding: 6px 10px; color: #E8ECF2; }"
        "QToolButton:hover { border-color: #4C8DFF; background: #262d3b; }"
        "QToolButton:disabled { color: #6F7A8A; background: #171B22; }"
        "QLineEdit { background: #1B2230; border: 1px solid #303746; border-radius: 12px; padding: 5px 12px; color: #E8ECF2; selection-background-color: #264F78; }"
        "QLineEdit:focus { border-color: #4C8DFF; }"
        "QTreeView, QPlainTextEdit, QListWidget { background: #0D1117; border: 1px solid #303746; selection-background-color: #264F78; color: #E8ECF2; }"
        "QTabWidget::pane { border: 1px solid #303746; background: #0D1117; }"
        "QTabBar::tab { background: #171B22; color: #A7B0BE; border: 1px solid #303746; padding: 7px 12px; }"
        "QTabBar::tab:selected { background: #202633; color: #E8ECF2; border-top: 2px solid #4C8DFF; }"
        "QDockWidget::title { background: #111318; padding: 6px; text-align: right; color: #E8ECF2; }"
        "QStatusBar { background: #111318; color: #A7B0BE; border-top: 1px solid #303746; }"));

    auto *topShell = new QWidget(this);
    topShell->setObjectName(QStringLiteral("topShell"));
    topShell->setLayoutDirection(Qt::RightToLeft);
    topShell->setFixedHeight(44);

    auto *topMenuRow = new QWidget(topShell);
    topMenuRow->setObjectName(QStringLiteral("topMenuRow"));
    topMenuRow->setLayoutDirection(Qt::RightToLeft);
    topMenuRow->setFixedHeight(44);
    auto *topShellLayout = new QHBoxLayout(topShell);
    topShellLayout->setContentsMargins(0, 0, 0, 0);
    topShellLayout->setSpacing(0);
    topShellLayout->addWidget(topMenuRow);
    auto *menuLayout = new QHBoxLayout(topMenuRow);
    menuLayout->setContentsMargins(12, 4, 12, 4);
    menuLayout->setSpacing(10);
    setMenuWidget(topShell);

    auto *brandBlock = new QWidget(topMenuRow);
    brandBlock->setObjectName(QStringLiteral("brandBlock"));
    brandBlock->setLayoutDirection(Qt::LeftToRight);
    auto *brandLayout = new QHBoxLayout(brandBlock);
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(8);

    brandLogoLabel = new QLabel(brandBlock);
    brandLogoLabel->setObjectName(QStringLiteral("brandLogoLabel"));
    brandLogoLabel->setPixmap(QPixmap(QStringLiteral(":/branding/lisan-logo.png")).scaled(
        30,
        30,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation));
    brandLogoLabel->setToolTip(QString::fromUtf8("استوديو لسان"));
    auto *brandTextLabel = new QLabel(QStringLiteral("Lisan Studio"), brandBlock);
    brandTextLabel->setObjectName(QStringLiteral("brandTextLabel"));
    brandTextLabel->setToolTip(QString::fromUtf8("استوديو لسان"));
    brandLayout->addWidget(brandLogoLabel);
    brandLayout->addWidget(brandTextLabel);

    auto *fileMenu = new QMenu(QString::fromUtf8("ملف"), this);
    auto *editMenu = new QMenu(QString::fromUtf8("تحرير"), this);
    auto *viewMenu = new QMenu(QString::fromUtf8("عرض"), this);
    auto *runMenu = new QMenu(QString::fromUtf8("تشغيل"), this);
    auto *searchMenu = new QMenu(QString::fromUtf8("بحث"), this);
    auto *toolsMenu = new QMenu(QString::fromUtf8("أدوات"), this);
    auto *helpMenu = new QMenu(QString::fromUtf8("مساعدة"), this);
    for (auto *menu : {fileMenu, editMenu, viewMenu, runMenu, searchMenu, toolsMenu, helpMenu}) {
        menu->setLayoutDirection(Qt::RightToLeft);
    }

    auto addMenuButton = [&](const QString &objectName, const QString &label, QMenu *menu) {
        auto *button = new QToolButton(topMenuRow);
        button->setObjectName(objectName);
        button->setProperty("role", "topMenu");
        button->setText(label);
        button->setMenu(menu);
        button->setPopupMode(QToolButton::InstantPopup);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setLayoutDirection(Qt::RightToLeft);
        button->setCursor(Qt::PointingHandCursor);
        menuLayout->addWidget(button);
        return button;
    };

    addMenuButton(QStringLiteral("topMenuFileButton"), QString::fromUtf8("ملف"), fileMenu);
    addMenuButton(QStringLiteral("topMenuEditButton"), QString::fromUtf8("تحرير"), editMenu);
    addMenuButton(QStringLiteral("topMenuViewButton"), QString::fromUtf8("عرض"), viewMenu);
    addMenuButton(QStringLiteral("topMenuRunButton"), QString::fromUtf8("تشغيل"), runMenu);
    addMenuButton(QStringLiteral("topMenuSearchButton"), QString::fromUtf8("بحث"), searchMenu);
    addMenuButton(QStringLiteral("topMenuToolsButton"), QString::fromUtf8("أدوات"), toolsMenu);
    addMenuButton(QStringLiteral("topMenuHelpButton"), QString::fromUtf8("مساعدة"), helpMenu);

    auto makeAction = [&](const QIcon &icon, const QString &label, auto slot) {
        auto *action = new QAction(icon, label, this);
        action->setToolTip(label);
        connect(action, &QAction::triggered, this, slot);
        addAction(action);
        return action;
    };

    auto addTopButton = [&](const QString &objectName, QAction *action, const char *role, Qt::ToolButtonStyle buttonStyle) {
        auto *button = new QToolButton(topMenuRow);
        button->setObjectName(objectName);
        button->setProperty("role", role);
        button->setDefaultAction(action);
        button->setToolButtonStyle(buttonStyle);
        button->setLayoutDirection(Qt::RightToLeft);
        if (buttonStyle == Qt::ToolButtonIconOnly) {
            button->setToolTip(action->text());
        }
        menuLayout->addWidget(button);
        return button;
    };

    auto *saveAction = makeAction(style()->standardIcon(QStyle::SP_DialogSaveButton), QString::fromUtf8("حفظ"), &MainWindow::saveFile);
    auto *openProjectAction = makeAction(style()->standardIcon(QStyle::SP_DirOpenIcon), QString::fromUtf8("فتح مشروع"), &MainWindow::openFolder);
    runAction = makeAction(style()->standardIcon(QStyle::SP_MediaPlay), QString::fromUtf8("تشغيل"), &MainWindow::runCurrentFile);
    runAction->setObjectName(QStringLiteral("runAction"));
    cancelRunAction = makeAction(style()->standardIcon(QStyle::SP_MediaStop), QString::fromUtf8("إيقاف"), &MainWindow::cancelRuntimeProcess);
    cancelRunAction->setObjectName(QStringLiteral("cancelRunAction"));
    cancelRunAction->setEnabled(false);
    lintAction = makeAction(style()->standardIcon(QStyle::SP_MessageBoxInformation), QString::fromUtf8("فحص"), &MainWindow::lintCurrentFile);
    lintAction->setObjectName(QStringLiteral("lintAction"));
    formatAction = makeAction(style()->standardIcon(QStyle::SP_BrowserReload), QString::fromUtf8("تنسيق"), &MainWindow::formatCurrentFile);
    formatAction->setObjectName(QStringLiteral("formatAction"));
    commandPaletteAction = makeAction(style()->standardIcon(QStyle::SP_FileDialogListView), QString::fromUtf8("لوحة الأوامر"), &MainWindow::openCommandPalette);
    commandPaletteAction->setObjectName(QStringLiteral("commandPaletteAction"));
    commandPaletteAction->setShortcuts({QKeySequence(QStringLiteral("Ctrl+Shift+P"))});
    auto *settingsAction = makeAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), QString::fromUtf8("الإعدادات"), &MainWindow::openSettings);
    auto *searchAction = makeAction(style()->standardIcon(QStyle::SP_FileDialogContentsView), QString::fromUtf8("بحث"), &MainWindow::findInProject);

    addTopButton(QStringLiteral("topRunButton"), runAction, "primaryAction", Qt::ToolButtonTextBesideIcon);

    fileMenu->addAction(QString::fromUtf8("ملف جديد"), QKeySequence::New, this, &MainWindow::newFile);
    fileMenu->addAction(QString::fromUtf8("فتح ملف"), QKeySequence::Open, this, &MainWindow::openFile);
    fileMenu->addAction(QString::fromUtf8("فتح مشروع"), this, &MainWindow::openFolder);
    fileMenu->addAction(QString::fromUtf8("حفظ"), QKeySequence::Save, this, &MainWindow::saveFile);
    fileMenu->addAction(QString::fromUtf8("حفظ باسم"), QKeySequence::SaveAs, this, &MainWindow::saveFileAs);
    editMenu->addAction(QString::fromUtf8("تراجع"), QKeySequence::Undo, this, [this]() {
        if (editor) {
            editor->undo();
        }
    });
    editMenu->addAction(QString::fromUtf8("إعادة"), QKeySequence::Redo, this, [this]() {
        if (editor) {
            editor->redo();
        }
    });
    viewMenu->addAction(commandPaletteAction);
    searchMenu->addAction(QString::fromUtf8("بحث في المشروع"), this, &MainWindow::findInProject);
    toolsMenu->addAction(QString::fromUtf8("فحص"), this, &MainWindow::lintCurrentFile);
    toolsMenu->addAction(QString::fromUtf8("تنسيق"), this, &MainWindow::formatCurrentFile);
    toolsMenu->addAction(settingsAction);
    runMenu->addAction(QString::fromUtf8("تشغيل الملف الحالي"), QKeySequence(QStringLiteral("F5")), this, &MainWindow::runCurrentFile);
    runMenu->addAction(QString::fromUtf8("إيقاف التشغيل"), QKeySequence(QStringLiteral("Shift+F5")), this, &MainWindow::cancelRuntimeProcess);
    helpMenu->addAction(QString::fromUtf8("عن استوديو لسان"), this, [this]() {
        QMessageBox::information(this, QString::fromUtf8("عن استوديو لسان"), QString::fromUtf8("استوديو لسان\nبيئة عربية أصلية لملفات .apy"));
    });

    runtimeTimeoutTimer = new QTimer(this);
    runtimeTimeoutTimer->setObjectName(QStringLiteral("runtimeTimeoutTimer"));
    runtimeTimeoutTimer->setSingleShot(true);
    runtimeTimeoutTimer->setInterval(30000);
    connect(runtimeTimeoutTimer, &QTimer::timeout, this, &MainWindow::handleRuntimeTimeout);

    commandBox = new QLineEdit(this);
    commandBox->setObjectName(QStringLiteral("commandBox"));
    commandBox->setPlaceholderText(QString::fromUtf8("ابحث في الأوامر والملفات..."));
    commandBox->setLayoutDirection(Qt::RightToLeft);
    commandBox->setFixedWidth(440);
    connect(commandBox, &QLineEdit::returnPressed, this, &MainWindow::findInProject);
    menuLayout->addStretch(1);
    menuLayout->addWidget(commandBox);
    menuLayout->addStretch(1);
    menuLayout->addWidget(brandBlock);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setLayoutDirection(Qt::RightToLeft);

    fileSystemModel = new QFileSystemModel(this);
    fileSystemModel->setNameFilters({QStringLiteral("*.apy"), QStringLiteral("*.py"), QStringLiteral("*.md"), QStringLiteral("*.txt")});
    fileSystemModel->setNameFilterDisables(false);

    projectTree = new QTreeView(splitter);
    projectTree->setObjectName(QStringLiteral("projectTree"));
    projectTree->setHeaderHidden(true);
    projectTree->setModel(fileSystemModel);
    projectTree->hideColumn(1);
    projectTree->hideColumn(2);
    projectTree->hideColumn(3);
    projectTree->setMinimumWidth(240);
    connect(projectTree, &QTreeView::doubleClicked, this, &MainWindow::openSelectedProjectFile);

    editorTabs = new QTabWidget(splitter);
    editorTabs->setObjectName(QStringLiteral("editorTabs"));
    editorTabs->setDocumentMode(true);
    editorTabs->setTabsClosable(true);
    editorTabs->setLayoutDirection(Qt::RightToLeft);
    connect(editorTabs, &QTabWidget::currentChanged, this, [this](int index) {
        setCurrentEditor(qobject_cast<EditorSurface *>(editorTabs->widget(index)));
    });
    connect(editorTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeEditorTab);
    createEditorTab(QString::fromUtf8("ملف جديد"));

    splitter->addWidget(projectTree);
    splitter->addWidget(editorTabs);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    bottomPanelTabs = new QTabWidget(this);
    bottomPanelTabs->setObjectName(QStringLiteral("bottomPanelTabs"));
    bottomPanelTabs->setLayoutDirection(Qt::RightToLeft);

    auto *arabicOutputPanel = new ArabicPlaceholderPlainTextEdit(bottomPanelTabs);
    outputPanel = arabicOutputPanel;
    outputPanel->setObjectName(QStringLiteral("outputPanel"));
    outputPanel->setReadOnly(true);
    outputPanel->setLayoutDirection(Qt::RightToLeft);
    QTextOption outputOption = outputPanel->document()->defaultTextOption();
    outputOption.setTextDirection(Qt::RightToLeft);
    outputOption.setAlignment(Qt::AlignRight);
    outputPanel->document()->setDefaultTextOption(outputOption);
    outputPanel->setMinimumHeight(160);
    arabicOutputPanel->setArabicPlaceholderText(QString::fromUtf8("المخرجات ستظهر هنا"));

    auto *arabicTerminalPanel = new ArabicPlaceholderPlainTextEdit(bottomPanelTabs);
    terminalPanel = arabicTerminalPanel;
    terminalPanel->setObjectName(QStringLiteral("terminalPanel"));
    terminalPanel->setReadOnly(true);
    terminalPanel->setLayoutDirection(Qt::RightToLeft);
    arabicTerminalPanel->setArabicPlaceholderText(QString::fromUtf8("الطرفية ستظهر هنا"));

    auto *arabicProblemsPanel = new ArabicPlaceholderPlainTextEdit(bottomPanelTabs);
    problemsPanel = arabicProblemsPanel;
    problemsPanel->setObjectName(QStringLiteral("problemsPanel"));
    problemsPanel->setReadOnly(true);
    problemsPanel->setLayoutDirection(Qt::RightToLeft);
    arabicProblemsPanel->setArabicPlaceholderText(QString::fromUtf8("لا توجد مشاكل حاليا"));

    auto *arabicDebugPanel = new ArabicPlaceholderPlainTextEdit(bottomPanelTabs);
    debugPanel = arabicDebugPanel;
    debugPanel->setObjectName(QStringLiteral("debugPanel"));
    debugPanel->setReadOnly(true);
    debugPanel->setLayoutDirection(Qt::RightToLeft);
    arabicDebugPanel->setArabicPlaceholderText(QString::fromUtf8("بيانات التصحيح ستظهر هنا"));

    bottomPanelTabs->addTab(terminalPanel, QString::fromUtf8("الطرفية"));
    bottomPanelTabs->addTab(outputPanel, QString::fromUtf8("الإخراج"));
    bottomPanelTabs->addTab(problemsPanel, QString::fromUtf8("المشاكل"));
    bottomPanelTabs->addTab(debugPanel, QString::fromUtf8("التصحيح"));

    outputDock = new QDockWidget(QString::fromUtf8("اللوحة السفلية"), this);
    outputDock->setObjectName(QStringLiteral("outputDock"));
    outputDock->setWidget(bottomPanelTabs);
    outputDock->setMinimumHeight(180);
    outputDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea, outputDock);
    resizeDocks({outputDock}, {190}, Qt::Vertical);

    statusLabel = new QLabel(QString::fromUtf8("جاهز"), this);
    statusBar()->addPermanentWidget(statusLabel, 1);
}

void MainWindow::newFile()
{
    if (!confirmSaveIfDirty()) {
        return;
    }
    createEditorTab(QString::fromUtf8("ملف جديد"));
    outputPanel->clear();
    setStatus(QString::fromUtf8("ملف جديد"));
}

void MainWindow::openFile()
{
    if (!confirmSaveIfDirty()) {
        return;
    }
    const QString path = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("فتح ملف"),
        projectRoot.isEmpty() ? QDir::homePath() : projectRoot,
        QString::fromUtf8("ملفات الكود (*.apy *.py *.md *.txt)"));
    if (!path.isEmpty()) {
        openEditorFile(path);
    }
}

void MainWindow::saveFile()
{
    QString error;
    if (!editor->saveFile(&error)) {
        saveFileAs();
        return;
    }
    setStatus(QString::fromUtf8("تم الحفظ"));
}

void MainWindow::saveFileAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        QString::fromUtf8("حفظ باسم"),
        projectRoot.isEmpty() ? QDir::homePath() : projectRoot,
        QString::fromUtf8("ملفات لغة الثعبان (*.apy);;كل الملفات (*)"));
    if (path.isEmpty()) {
        return;
    }

    QString error;
    if (!editor->saveFileAs(path, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر الحفظ"), error);
        return;
    }
    setStatus(QString::fromUtf8("تم الحفظ"));
}

void MainWindow::openFolder()
{
    const QString path = QFileDialog::getExistingDirectory(this, QString::fromUtf8("فتح مشروع"), QDir::homePath());
    if (!path.isEmpty()) {
        loadProject(path);
    }
}

void MainWindow::runCurrentFile()
{
    runRuntimeAction(RuntimeAction::Run, QString::fromUtf8("تشغيل"));
}

void MainWindow::lintCurrentFile()
{
    runRuntimeAction(RuntimeAction::Lint, QString::fromUtf8("فحص"));
}

void MainWindow::formatCurrentFile()
{
    runRuntimeAction(RuntimeAction::Format, QString::fromUtf8("تنسيق"), true);
}

void MainWindow::cancelRuntimeProcess()
{
    if (!activeRuntimeProcess || activeRuntimeProcess->state() == QProcess::NotRunning) {
        return;
    }

    appendRuntimeOutput(QString::fromUtf8("النظام"), QString::fromUtf8("تم طلب إيقاف العملية."));
    activeRuntimeProcess->kill();
    setStatus(QString::fromUtf8("تم إيقاف التشغيل"));
}

void MainWindow::appendRuntimeStdout()
{
    if (!activeRuntimeProcess) {
        return;
    }
    appendRuntimeOutput(QString::fromUtf8("stdout"), QString::fromUtf8(activeRuntimeProcess->readAllStandardOutput()));
}

void MainWindow::appendRuntimeStderr()
{
    if (!activeRuntimeProcess) {
        return;
    }
    appendRuntimeOutput(QString::fromUtf8("stderr"), QString::fromUtf8(activeRuntimeProcess->readAllStandardError()));
}

void MainWindow::finishRuntimeProcess(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    if (!activeRuntimeProcess) {
        return;
    }

    appendRuntimeStdout();
    appendRuntimeStderr();
    const QString finalText = QString::fromUtf8("رمز الخروج: %1\nالمدة: %2 ms")
        .arg(exitCode)
        .arg(activeRuntimeTimer.isValid() ? activeRuntimeTimer.elapsed() : 0);
    appendRuntimeOutput(QString::fromUtf8("النظام"), finalText);
    completeRuntimeProcess(QString::fromUtf8("%1 انتهى: %2").arg(activeRuntimeTitle).arg(exitCode));
}

void MainWindow::handleRuntimeProcessError(QProcess::ProcessError error)
{
    if (!activeRuntimeProcess || activeRuntimeHandledError) {
        return;
    }

    if (error != QProcess::FailedToStart) {
        return;
    }

    activeRuntimeHandledError = true;
    appendRuntimeOutput(
        QString::fromUtf8("النظام"),
        QString::fromUtf8("تعذر بدء العملية: %1\nرمز الخروج: -1").arg(activeRuntimeProcess->errorString()));
    completeRuntimeProcess(QString::fromUtf8("تعذر بدء %1").arg(activeRuntimeTitle));
}

void MainWindow::handleRuntimeTimeout()
{
    if (!activeRuntimeProcess || activeRuntimeProcess->state() == QProcess::NotRunning) {
        return;
    }

    appendRuntimeOutput(QString::fromUtf8("النظام"), QString::fromUtf8("انتهت مهلة التشغيل."));
    activeRuntimeProcess->kill();
    setStatus(QString::fromUtf8("انتهت مهلة التشغيل"));
}

void MainWindow::openCommandPalette()
{
    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("commandPaletteDialog"));
    dialog.setWindowTitle(QString::fromUtf8("لوحة الأوامر"));
    dialog.setLayoutDirection(Qt::RightToLeft);
    dialog.resize(560, 360);

    auto *layout = new QVBoxLayout(&dialog);
    auto *input = new QLineEdit(&dialog);
    input->setObjectName(QStringLiteral("commandPaletteInput"));
    input->setPlaceholderText(QString::fromUtf8("اكتب اسم الأمر..."));
    input->setLayoutDirection(Qt::RightToLeft);

    auto *commands = new QListWidget(&dialog);
    commands->setObjectName(QStringLiteral("commandPaletteResults"));
    commands->setLayoutDirection(Qt::RightToLeft);
    commands->addItem(QString::fromUtf8("ملف جديد                    Ctrl+N"));
    commands->addItem(QString::fromUtf8("فتح ملف                     Ctrl+O"));
    commands->addItem(QString::fromUtf8("تشغيل الملف الحالي           F5"));
    commands->addItem(QString::fromUtf8("بحث في المشروع               Ctrl+Shift+F"));
    commands->addItem(QString::fromUtf8("فتح الإعدادات                Ctrl+,"));

    layout->addWidget(input);
    layout->addWidget(commands);
    dialog.exec();
}

void MainWindow::findInProject()
{
    if (projectRoot.isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("لا يوجد مشروع"), QString::fromUtf8("افتح مشروعا قبل البحث."));
        return;
    }

    const QString query = commandBox ? commandBox->text().trimmed() : QString();
    if (query.isEmpty()) {
        return;
    }

    SearchService service;
    const auto rows = service.search(projectRoot, query);
    QStringList lines;
    for (const auto &row : rows) {
        lines << QStringLiteral("%1:%2: %3").arg(row.path).arg(row.line).arg(row.preview);
    }
    writeOutput(QString::fromUtf8("البحث"), lines.join('\n'));
    setStatus(QString::fromUtf8("نتائج البحث: %1").arg(rows.size()));
}

void MainWindow::openSelectedProjectFile(const QModelIndex &index)
{
    const QString path = fileSystemModel->filePath(index);
    if (QFileInfo(path).isFile()) {
        openEditorFile(path);
    }
}

void MainWindow::openSettings()
{
    bool ok = false;
    const QString family = QInputDialog::getText(
        this,
        QString::fromUtf8("إعدادات الخط"),
        QString::fromUtf8("اسم الخط"),
        QLineEdit::Normal,
        settings.editorFontFamily(),
        &ok);
    if (!ok || family.trimmed().isEmpty()) {
        return;
    }
    settings.setEditorFontFamily(family.trimmed());
    QFont font = editor->font();
    font.setFamily(family.trimmed());
    editor->setFont(font);
    setStatus(QString::fromUtf8("تم تحديث الإعدادات"));
}

void MainWindow::setStatus(const QString &text)
{
    statusLabel->setText(text);
}

bool MainWindow::loadProject(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        return false;
    }
    projectRoot = QDir(path).absolutePath();
    fileSystemModel->setRootPath(projectRoot);
    projectTree->setRootIndex(fileSystemModel->index(projectRoot));
    settings.addRecentProject(projectRoot);
    setStatus(QString::fromUtf8("المشروع: %1").arg(projectRoot));
    return true;
}

bool MainWindow::openEditorFile(const QString &path)
{
    const QString normalizedPath = QFileInfo(path).absoluteFilePath();
    for (int i = 0; editorTabs && i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (surface && QFileInfo(surface->currentFilePath()).absoluteFilePath() == normalizedPath) {
            editorTabs->setCurrentIndex(i);
            return true;
        }
    }

    EditorSurface *target = editor;
    if (!target || !target->currentFilePath().isEmpty() || target->isDirty() || editorTabs->count() > 1) {
        target = createEditorTab(QFileInfo(normalizedPath).fileName());
    }

    QString error;
    if (!target->openFile(normalizedPath, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر فتح الملف"), error);
        return false;
    }
    setCurrentEditor(target);
    updateEditorTabTitle(target);
    if (projectRoot.isEmpty()) {
        loadProject(QFileInfo(normalizedPath).absolutePath());
    }
    setStatus(QString::fromUtf8("فتح: %1").arg(QFileInfo(normalizedPath).fileName()));
    return true;
}

EditorSurface *MainWindow::createEditorTab(const QString &title)
{
    auto *surface = new EditorSurface(editorTabs);
    const int index = editorTabs->addTab(surface, title);
    editorTabs->setCurrentIndex(index);
    setCurrentEditor(surface);

    connect(surface, &EditorSurface::filePathChanged, this, [this, surface](const QString &) {
        updateEditorTabTitle(surface);
        if (surface == editor) {
            setWindowTitle(surface->currentFilePath().isEmpty()
                ? QString::fromUtf8("استوديو لسان")
                : QString::fromUtf8("استوديو لسان - %1").arg(QFileInfo(surface->currentFilePath()).fileName()));
        }
    });
    connect(surface, &EditorSurface::dirtyStateChanged, this, [this, surface](bool) {
        updateEditorTabTitle(surface);
    });

    updateEditorTabTitle(surface);
    return surface;
}

void MainWindow::setCurrentEditor(EditorSurface *surface)
{
    if (!surface) {
        return;
    }

    for (int i = 0; editorTabs && i < editorTabs->count(); ++i) {
        if (auto *tabEditor = qobject_cast<EditorSurface *>(editorTabs->widget(i))) {
            tabEditor->setObjectName(tabEditor == surface ? QStringLiteral("editorSurface") : QStringLiteral("editorSurfaceInactive"));
        }
    }

    editor = surface;
    setWindowTitle(editor->currentFilePath().isEmpty()
        ? QString::fromUtf8("استوديو لسان")
        : QString::fromUtf8("استوديو لسان - %1").arg(QFileInfo(editor->currentFilePath()).fileName()));
}

void MainWindow::updateEditorTabTitle(EditorSurface *surface)
{
    if (!surface || !editorTabs) {
        return;
    }

    const int index = editorTabs->indexOf(surface);
    if (index < 0) {
        return;
    }

    QString title = surface->currentFilePath().isEmpty()
        ? QString::fromUtf8("ملف جديد")
        : QFileInfo(surface->currentFilePath()).fileName();
    if (surface->isDirty()) {
        title.prepend(QLatin1Char('*'));
    }
    editorTabs->setTabText(index, title);
}

void MainWindow::closeEditorTab(int index)
{
    auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(index));
    if (!surface) {
        return;
    }

    EditorSurface *previousEditor = editor;
    setCurrentEditor(surface);
    if (!confirmSaveIfDirty()) {
        setCurrentEditor(previousEditor);
        return;
    }

    if (editorTabs->count() == 1) {
        surface->resetForNewFile();
        updateEditorTabTitle(surface);
        return;
    }

    editorTabs->removeTab(index);
    surface->deleteLater();
    setCurrentEditor(qobject_cast<EditorSurface *>(editorTabs->currentWidget()));
}

void MainWindow::writeOutput(const QString &title, const QString &text)
{
    showOutputPanel();
    outputPanel->setPlainText(QStringLiteral("[%1]\n%2").arg(title, text));
}

void MainWindow::showOutputPanel()
{
    if (!outputDock->isVisible()) {
        outputDock->show();
    }
    if (bottomPanelTabs) {
        bottomPanelTabs->setCurrentWidget(outputPanel);
    }
    outputDock->raise();
    resizeDocks({outputDock}, {190}, Qt::Vertical);
}

bool MainWindow::confirmSaveIfDirty()
{
    if (!editor->isDirty()) {
        return true;
    }

    const auto answer = QMessageBox::question(
        this,
        QString::fromUtf8("حفظ التغييرات"),
        QString::fromUtf8("هل تريد حفظ التغييرات قبل المتابعة؟"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (answer == QMessageBox::Cancel) {
        return false;
    }
    if (answer == QMessageBox::Save) {
        saveFile();
    }
    return true;
}

QString MainWindow::runtimeWorkingDirectory() const
{
    if (!projectRoot.isEmpty()) {
        return projectRoot;
    }
    if (!editor->currentFilePath().isEmpty()) {
        return QFileInfo(editor->currentFilePath()).absolutePath();
    }
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

void MainWindow::runRuntimeAction(RuntimeAction action, const QString &title, bool reloadAfterSuccess)
{
    if (activeRuntimeProcess && activeRuntimeProcess->state() != QProcess::NotRunning) {
        showOutputPanel();
        setStatus(QString::fromUtf8("هناك عملية قيد التشغيل"));
        return;
    }

    QString materializeError;
    const QString runFilePath = materializeRunnableBuffer(&materializeError);
    if (runFilePath.isEmpty()) {
        writeOutput(title, materializeError);
        return;
    }

    const QString workingDirectory = runtimeWorkingDirectory();
    const RuntimeCommand command = runtime.buildCommand(action, runFilePath, workingDirectory);
    activeRuntimeTitle = title;
    activeRuntimeHandledError = false;
    showOutputPanel();
    outputPanel->setPlainText(QStringLiteral("[%1]\n%2\n%3\n%4\n\n%5\n")
        .arg(title)
        .arg(QString::fromUtf8("الأمر: %1").arg(title))
        .arg(QString::fromUtf8("ملف: %1").arg(QDir::toNativeSeparators(runFilePath)))
        .arg(QString::fromUtf8("مجلد العمل: %1").arg(QDir::toNativeSeparators(command.workingDirectory)))
        .arg(QString::fromUtf8("جار التنفيذ...")));
    setStatus(QString::fromUtf8("%1...").arg(title));
    setRuntimeActionsRunning(true);

    activeRuntimeProcess = new QProcess(this);
    activeRuntimeProcess->setProgram(command.program);
    activeRuntimeProcess->setArguments(command.arguments);
    activeRuntimeProcess->setWorkingDirectory(command.workingDirectory);
    activeRuntimeProcess->setProcessEnvironment(runtime.processEnvironment());
    activeRuntimeProcess->setProperty("reloadAfterSuccess", reloadAfterSuccess);
    activeRuntimeProcess->setProperty("runFilePath", runFilePath);
    connect(activeRuntimeProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::appendRuntimeStdout);
    connect(activeRuntimeProcess, &QProcess::readyReadStandardError, this, &MainWindow::appendRuntimeStderr);
    connect(activeRuntimeProcess, &QProcess::finished, this, &MainWindow::finishRuntimeProcess);
    connect(activeRuntimeProcess, &QProcess::errorOccurred, this, &MainWindow::handleRuntimeProcessError);

    activeRuntimeTimer.start();
    runtimeTimeoutTimer->start();
    activeRuntimeProcess->start();
}

void MainWindow::appendRuntimeOutput(const QString &label, const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    showOutputPanel();
    QTextCursor cursor = outputPanel->textCursor();
    cursor.movePosition(QTextCursor::End);
    outputPanel->setTextCursor(cursor);
    outputPanel->appendPlainText(QStringLiteral("[%1]").arg(label));
    outputPanel->appendPlainText(text.trimmed());
}

void MainWindow::completeRuntimeProcess(const QString &statusText)
{
    if (!activeRuntimeProcess) {
        return;
    }

    const bool reloadAfterSuccess = activeRuntimeProcess->property("reloadAfterSuccess").toBool();
    const QString runFilePath = activeRuntimeProcess->property("runFilePath").toString();
    const int exitCode = activeRuntimeProcess->exitCode();
    QProcess *finishedProcess = activeRuntimeProcess;
    activeRuntimeProcess = nullptr;
    if (runtimeTimeoutTimer) {
        runtimeTimeoutTimer->stop();
    }
    setRuntimeActionsRunning(false);
    setStatus(statusText);
    finishedProcess->deleteLater();

    if (reloadAfterSuccess && exitCode == 0) {
        QString error;
        editor->openFile(runFilePath, &error);
    }
}

void MainWindow::setRuntimeActionsRunning(bool running)
{
    if (runAction) {
        runAction->setEnabled(!running);
    }
    if (lintAction) {
        lintAction->setEnabled(!running);
    }
    if (formatAction) {
        formatAction->setEnabled(!running);
    }
    if (cancelRunAction) {
        cancelRunAction->setEnabled(running);
    }
}
