#include "MainWindow.h"

#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
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

void MainWindow::buildUi()
{
    setLayoutDirection(Qt::RightToLeft);

    auto *toolbar = addToolBar(QString::fromUtf8("الأوامر"));
    toolbar->setMovable(false);
    toolbar->setLayoutDirection(Qt::RightToLeft);

    auto addActionButton = [&](const QString &label, auto slot) {
        QAction *action = toolbar->addAction(label);
        connect(action, &QAction::triggered, this, slot);
    };

    addActionButton(QString::fromUtf8("جديد"), &MainWindow::newFile);
    addActionButton(QString::fromUtf8("فتح ملف"), &MainWindow::openFile);
    addActionButton(QString::fromUtf8("حفظ"), &MainWindow::saveFile);
    addActionButton(QString::fromUtf8("حفظ باسم"), &MainWindow::saveFileAs);
    addActionButton(QString::fromUtf8("فتح مشروع"), &MainWindow::openFolder);
    addActionButton(QString::fromUtf8("تشغيل"), &MainWindow::runCurrentFile);
    addActionButton(QString::fromUtf8("فحص"), &MainWindow::lintCurrentFile);
    addActionButton(QString::fromUtf8("تنسيق"), &MainWindow::formatCurrentFile);
    addActionButton(QString::fromUtf8("الإعدادات"), &MainWindow::openSettings);

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
    projectTree->setHeaderHidden(true);
    projectTree->setModel(fileSystemModel);
    projectTree->setMinimumWidth(240);
    connect(projectTree, &QTreeView::doubleClicked, this, &MainWindow::openSelectedProjectFile);

    editor = new EditorSurface(splitter);
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
    outputPanel->setReadOnly(true);
    outputPanel->setLayoutDirection(Qt::LeftToRight);
    outputPanel->setPlaceholderText(QString::fromUtf8("المخرجات ستظهر هنا"));

    auto *dock = new QDockWidget(QString::fromUtf8("المخرجات"), this);
    dock->setWidget(outputPanel);
    addDockWidget(Qt::BottomDockWidgetArea, dock);

    statusLabel = new QLabel(QString::fromUtf8("جاهز"), this);
    statusBar()->addPermanentWidget(statusLabel, 1);
}

void MainWindow::newFile()
{
    if (!confirmSaveIfDirty()) {
        return;
    }
    editor->clear();
    editor->document()->setModified(false);
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

void MainWindow::loadProject(const QString &path)
{
    projectRoot = QDir(path).absolutePath();
    fileSystemModel->setRootPath(projectRoot);
    projectTree->setRootIndex(fileSystemModel->index(projectRoot));
    settings.addRecentProject(projectRoot);
    setStatus(QString::fromUtf8("المشروع: %1").arg(projectRoot));
}

void MainWindow::openEditorFile(const QString &path)
{
    QString error;
    if (!editor->openFile(path, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر فتح الملف"), error);
        return;
    }
    if (projectRoot.isEmpty()) {
        loadProject(QFileInfo(path).absolutePath());
    }
    setStatus(QString::fromUtf8("فتح: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::writeOutput(const QString &title, const QString &text)
{
    outputPanel->setPlainText(QStringLiteral("[%1]\n%2").arg(title, text));
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
