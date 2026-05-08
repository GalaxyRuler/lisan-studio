#include "MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTextStream>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    resize(1280, 820);
    setWindowTitle(QString::fromUtf8("استوديو البرمجة"));
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

void MainWindow::buildUi()
{
    setLayoutDirection(Qt::RightToLeft);

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background: #202124; color: #e8eaed; }"
        "QToolBar { background: #181a1b; border: 0; spacing: 6px; padding: 6px; }"
        "QToolButton { background: #2b2f31; border: 1px solid #3d4347; border-radius: 4px; padding: 5px 8px; }"
        "QToolButton:hover { background: #343a3f; }"
        "QLineEdit { background: #111315; border: 1px solid #3d4347; border-radius: 4px; padding: 5px 8px; }"
        "QTreeView, QPlainTextEdit { background: #25282a; border: 1px solid #343a3f; selection-background-color: #365b6d; }"
        "QDockWidget::title { background: #181a1b; padding: 5px; text-align: right; }"
        "QStatusBar { background: #181a1b; }"));

    auto *toolbar = addToolBar(QString::fromUtf8("الأوامر"));
    toolbar->setMovable(false);
    toolbar->setLayoutDirection(Qt::RightToLeft);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    auto addActionButton = [&](const QIcon &icon, const QString &label, auto slot) {
        QAction *action = toolbar->addAction(icon, label);
        connect(action, &QAction::triggered, this, slot);
    };

    addActionButton(style()->standardIcon(QStyle::SP_FileIcon), QString::fromUtf8("جديد"), &MainWindow::newFile);
    addActionButton(style()->standardIcon(QStyle::SP_DialogOpenButton), QString::fromUtf8("فتح ملف"), &MainWindow::openFile);
    addActionButton(style()->standardIcon(QStyle::SP_DialogSaveButton), QString::fromUtf8("حفظ"), &MainWindow::saveFile);
    addActionButton(style()->standardIcon(QStyle::SP_DriveFDIcon), QString::fromUtf8("حفظ باسم"), &MainWindow::saveFileAs);
    addActionButton(style()->standardIcon(QStyle::SP_DirOpenIcon), QString::fromUtf8("فتح مشروع"), &MainWindow::openFolder);
    addActionButton(style()->standardIcon(QStyle::SP_MediaPlay), QString::fromUtf8("تشغيل"), &MainWindow::runCurrentFile);
    addActionButton(style()->standardIcon(QStyle::SP_MessageBoxInformation), QString::fromUtf8("فحص"), &MainWindow::lintCurrentFile);
    addActionButton(style()->standardIcon(QStyle::SP_BrowserReload), QString::fromUtf8("تنسيق"), &MainWindow::formatCurrentFile);
    addActionButton(style()->standardIcon(QStyle::SP_FileDialogDetailedView), QString::fromUtf8("الإعدادات"), &MainWindow::openSettings);

    commandBox = new QLineEdit(this);
    commandBox->setPlaceholderText(QString::fromUtf8("ابحث أو اكتب أمرا"));
    commandBox->setLayoutDirection(Qt::RightToLeft);
    toolbar->addWidget(commandBox);

    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText(QString::fromUtf8("بحث في المشروع"));
    searchBox->setLayoutDirection(Qt::RightToLeft);
    toolbar->addWidget(searchBox);
    connect(searchBox, &QLineEdit::returnPressed, this, &MainWindow::findInProject);

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

    editor = new EditorSurface(splitter);
    editor->setObjectName(QStringLiteral("editorSurface"));
    connect(editor, &EditorSurface::filePathChanged, this, [this](const QString &path) {
        setWindowTitle(path.isEmpty()
            ? QString::fromUtf8("استوديو البرمجة")
            : QString::fromUtf8("استوديو البرمجة - %1").arg(QFileInfo(path).fileName()));
    });

    splitter->addWidget(projectTree);
    splitter->addWidget(editor);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    outputPanel = new QPlainTextEdit(this);
    outputPanel->setObjectName(QStringLiteral("outputPanel"));
    outputPanel->setReadOnly(true);
    outputPanel->setLayoutDirection(Qt::LeftToRight);
    outputPanel->setMinimumHeight(160);
    outputPanel->setPlaceholderText(QString::fromUtf8("المخرجات ستظهر هنا"));

    outputDock = new QDockWidget(QString::fromUtf8("المخرجات"), this);
    outputDock->setObjectName(QStringLiteral("outputDock"));
    outputDock->setWidget(outputPanel);
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
    editor->resetForNewFile();
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

void MainWindow::findInProject()
{
    if (projectRoot.isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("لا يوجد مشروع"), QString::fromUtf8("افتح مشروعا قبل البحث."));
        return;
    }

    const QString query = searchBox->text().trimmed();
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
    QString error;
    if (!editor->openFile(path, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر فتح الملف"), error);
        return false;
    }
    if (projectRoot.isEmpty()) {
        loadProject(QFileInfo(path).absolutePath());
    }
    setStatus(QString::fromUtf8("فتح: %1").arg(QFileInfo(path).fileName()));
    return true;
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

bool MainWindow::ensureCurrentFileSaved()
{
    if (editor->currentFilePath().isEmpty()) {
        saveFileAs();
    }
    if (editor->currentFilePath().isEmpty()) {
        return false;
    }
    if (editor->isDirty()) {
        saveFile();
    }
    return !editor->isDirty();
}

void MainWindow::runRuntimeAction(RuntimeAction action, const QString &title, bool reloadAfterSuccess)
{
    if (!ensureCurrentFileSaved()) {
        return;
    }

    writeOutput(title, QString::fromUtf8("جار التنفيذ..."));
    setStatus(QString::fromUtf8("%1...").arg(title));
    QApplication::processEvents();

    const RuntimeResult result = runtime.runBlocking(action, editor->currentFilePath(), 30000);
    const QString output = QString::fromUtf8("exit=%1\n\n%2\n%3")
        .arg(result.exitCode)
        .arg(result.standardOutput)
        .arg(result.standardError);
    writeOutput(title, output);

    if (reloadAfterSuccess && result.exitCode == 0) {
        QString error;
        editor->openFile(editor->currentFilePath(), &error);
    }
}
