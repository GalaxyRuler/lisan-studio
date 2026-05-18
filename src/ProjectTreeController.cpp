#include "ProjectTreeController.h"

#include "ProjectFileOperations.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QTreeView>

ProjectTreeController::ProjectTreeController(QTreeView *tree, QFileSystemModel *model, QObject *parent)
    : QObject(parent),
      projectTree(tree),
      fileSystemModel(model)
{
    Q_ASSERT(projectTree != nullptr);
    Q_ASSERT(fileSystemModel != nullptr);

    projectTree->setHeaderHidden(true);
    projectTree->setModel(fileSystemModel);
    projectTree->setLayoutDirection(Qt::RightToLeft);
    projectTree->setContextMenuPolicy(Qt::CustomContextMenu);
    projectTree->hideColumn(1);
    projectTree->hideColumn(2);
    projectTree->hideColumn(3);
    projectTree->setMinimumWidth(240);

    connect(projectTree, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        contextIndex = index.siblingAtColumn(0);
        openSelectedItem();
        contextIndex = QModelIndex();
    });
    connect(projectTree, &QTreeView::customContextMenuRequested, this, &ProjectTreeController::showContextMenu);

    newFileAction = createAction(QStringLiteral("projectTreeNewFileAction"), QString::fromUtf8("ملف جديد"), QStringLiteral("project.file.new"));
    connect(newFileAction, &QAction::triggered, this, &ProjectTreeController::createFile);

    newFolderAction = createAction(QStringLiteral("projectTreeNewFolderAction"), QString::fromUtf8("مجلد جديد"), QStringLiteral("project.folder.new"));
    connect(newFolderAction, &QAction::triggered, this, &ProjectTreeController::createFolder);

    openAction = createAction(QStringLiteral("projectTreeOpenAction"), QString::fromUtf8("فتح"), QStringLiteral("project.item.open"));
    connect(openAction, &QAction::triggered, this, &ProjectTreeController::openSelectedItem);

    renameAction = createAction(QStringLiteral("projectTreeRenameAction"), QString::fromUtf8("إعادة تسمية"), QStringLiteral("project.item.rename"));
    connect(renameAction, &QAction::triggered, this, &ProjectTreeController::renameSelectedItem);

    deleteAction = createAction(QStringLiteral("projectTreeDeleteAction"), QString::fromUtf8("حذف"), QStringLiteral("project.item.deleteWithPrompt"));
    connect(deleteAction, &QAction::triggered, this, &ProjectTreeController::deleteSelectedItem);

    revealAction = createAction(QStringLiteral("projectTreeRevealAction"), QString::fromUtf8("إظهار في مستكشف الملفات"), QStringLiteral("project.item.reveal"));
    connect(revealAction, &QAction::triggered, this, &ProjectTreeController::revealSelectedItem);

    copyPathAction = createAction(QStringLiteral("projectTreeCopyPathAction"), QString::fromUtf8("نسخ المسار"), QStringLiteral("project.item.copyPath"));
    connect(copyPathAction, &QAction::triggered, this, &ProjectTreeController::copySelectedItemPath);

    openContainingFolderAction = createAction(QStringLiteral("projectTreeOpenContainingFolderAction"), QString::fromUtf8("فتح المجلد الحاوي"), QStringLiteral("project.item.openContainingFolder"));
    connect(openContainingFolderAction, &QAction::triggered, this, &ProjectTreeController::openSelectedContainingFolder);

    refreshAction = createAction(QStringLiteral("projectTreeRefreshAction"), QString::fromUtf8("تحديث"), QStringLiteral("project.refresh"));
    connect(refreshAction, &QAction::triggered, this, &ProjectTreeController::refresh);
}

void ProjectTreeController::setProjectRoot(const QString &path)
{
    projectRoot = QFileInfo(path).absoluteFilePath();
    refresh();
}

void ProjectTreeController::refresh()
{
    if (projectRoot.isEmpty() || !fileSystemModel || !projectTree) {
        return;
    }

    fileSystemModel->setRootPath(projectRoot);
    const QModelIndex rootIndex = fileSystemModel->index(projectRoot);
    projectTree->setRootIndex(rootIndex);
    projectTree->expand(rootIndex);
    emit statusMessage(QString::fromUtf8("تم تحديث المشروع"));
}

QString ProjectTreeController::currentSelectionPath() const
{
    return activePath();
}

QString ProjectTreeController::currentSelectionFolderPath() const
{
    return activeFolderPath();
}

void ProjectTreeController::setDeleteConfirmationCallback(DeleteConfirmationCallback callback)
{
    confirmDelete = std::move(callback);
}

void ProjectTreeController::openSelectedItem()
{
    const QString path = activePath();
    if (path.isEmpty()) {
        return;
    }

    const QFileInfo info(path);
    if (info.isDir() && projectTree) {
        const QModelIndex index = activeIndex();
        projectTree->setExpanded(index, !projectTree->isExpanded(index));
        return;
    }
    if (info.isFile()) {
        emit openPathRequested(path);
    }
}

void ProjectTreeController::createFile()
{
    const QString folderPath = activeFolderPath();
    if (folderPath.isEmpty()) {
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getText(
        projectTree,
        QString::fromUtf8("ملف جديد"),
        QString::fromUtf8("اسم الملف"),
        QLineEdit::Normal,
        QStringLiteral("main.apy"),
        &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }

    QString error;
    const ProjectFileOperationTarget target = ProjectFileOperations::childTarget(folderPath, projectRoot, name, true, &error);
    if (!target.allowed) {
        emit errorMessage(QString::fromUtf8("تعذر إنشاء الملف"), error);
        return;
    }

    QFile file(target.path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Text)) {
        emit errorMessage(QString::fromUtf8("تعذر إنشاء الملف"), file.errorString());
        return;
    }
    file.close();

    refresh();
    emit openPathRequested(target.path);
}

void ProjectTreeController::createFolder()
{
    const QString folderPath = activeFolderPath();
    if (folderPath.isEmpty()) {
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getText(
        projectTree,
        QString::fromUtf8("مجلد جديد"),
        QString::fromUtf8("اسم المجلد"),
        QLineEdit::Normal,
        QString::fromUtf8("مجلد جديد"),
        &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }

    QString error;
    const ProjectFileOperationTarget target = ProjectFileOperations::childTarget(folderPath, projectRoot, name, false, &error);
    if (!target.allowed) {
        emit errorMessage(QString::fromUtf8("تعذر إنشاء المجلد"), error);
        return;
    }

    if (!QDir().mkpath(target.path)) {
        emit errorMessage(QString::fromUtf8("تعذر إنشاء المجلد"), QString::fromUtf8("راجع الاسم أو أذونات المجلد."));
        return;
    }
    refresh();
}

void ProjectTreeController::renameSelectedItem()
{
    const QString path = activePath();
    const QFileInfo info(path);
    if (!info.exists() || info.absoluteFilePath() == QFileInfo(projectRoot).absoluteFilePath()) {
        return;
    }

    bool accepted = false;
    const QString newName = QInputDialog::getText(
        projectTree,
        QString::fromUtf8("إعادة تسمية"),
        QString::fromUtf8("الاسم الجديد"),
        QLineEdit::Normal,
        info.fileName(),
        &accepted).trimmed();
    if (!accepted || newName.isEmpty() || newName == info.fileName()) {
        return;
    }

    QString error;
    const ProjectFileOperationTarget target = ProjectFileOperations::renameTarget(info.absoluteFilePath(), newName, projectRoot, &error);
    if (!target.allowed) {
        emit errorMessage(QString::fromUtf8("تعذرت إعادة التسمية"), error);
        return;
    }

    QDir parent(info.absolutePath());
    if (!parent.rename(info.fileName(), QFileInfo(target.path).fileName())) {
        emit errorMessage(QString::fromUtf8("تعذرت إعادة التسمية"), QString::fromUtf8("راجع أذونات الملف أو المجلد."));
        return;
    }

    refresh();
    emit pathRenamed(info.absoluteFilePath(), target.path);
}

void ProjectTreeController::deleteSelectedItem()
{
    const QString path = activePath();
    const QFileInfo info(path);
    if (!info.exists()) {
        return;
    }

    QString error;
    if (!ProjectFileOperations::canDelete(info.absoluteFilePath(), projectRoot, &error)) {
        emit errorMessage(QString::fromUtf8("تعذر الحذف"), error);
        return;
    }

    if (confirmDelete && !confirmDelete(info.absoluteFilePath(), info.isDir())) {
        return;
    }

    bool removed = false;
    if (info.isDir()) {
        removed = QDir(info.absoluteFilePath()).removeRecursively();
    } else {
        removed = QFile::remove(info.absoluteFilePath());
    }
    if (!removed) {
        emit errorMessage(QString::fromUtf8("تعذر الحذف"), QString::fromUtf8("راجع أذونات الملف أو المجلد."));
        return;
    }

    emit pathDeleted(info.absoluteFilePath());
    refresh();
}

void ProjectTreeController::revealSelectedItem()
{
    const QString path = activePath();
    if (!path.isEmpty()) {
        emit revealRequested(path);
    }
}

void ProjectTreeController::copySelectedItemPath()
{
    const QString path = activePath();
    if (path.isEmpty() || !QApplication::clipboard()) {
        return;
    }

    QApplication::clipboard()->setText(ProjectFileOperations::pathForClipboard(path));
    emit statusMessage(QString::fromUtf8("تم نسخ المسار"));
}

void ProjectTreeController::openSelectedContainingFolder()
{
    const QString path = activePath();
    if (path.isEmpty()) {
        return;
    }

    const QString folderPath = ProjectFileOperations::containingFolder(path);
    if (!folderPath.isEmpty()) {
        emit revealRequested(folderPath);
    }
}

QModelIndex ProjectTreeController::activeIndex() const
{
    if (contextIndex.isValid()) {
        return contextIndex.siblingAtColumn(0);
    }
    if (projectTree && projectTree->currentIndex().isValid()) {
        return projectTree->currentIndex().siblingAtColumn(0);
    }
    return QModelIndex();
}

QString ProjectTreeController::activePath() const
{
    const QModelIndex index = activeIndex();
    return index.isValid() && fileSystemModel ? fileSystemModel->filePath(index) : QString();
}

QString ProjectTreeController::activeFolderPath() const
{
    const QString path = activePath();
    if (path.isEmpty()) {
        return projectRoot;
    }

    const QFileInfo info(path);
    return info.isDir() ? info.absoluteFilePath() : info.absolutePath();
}

void ProjectTreeController::showContextMenu(const QPoint &pos)
{
    if (!projectTree || !fileSystemModel) {
        return;
    }

    const QModelIndex index = projectTree->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    contextIndex = index.siblingAtColumn(0);
    const QFileInfo info(fileSystemModel->filePath(contextIndex));
    if (!info.exists()) {
        contextIndex = QModelIndex();
        return;
    }

    QMenu menu(projectTree);
    menu.setLayoutDirection(Qt::RightToLeft);
    menu.setObjectName(QStringLiteral("projectTreeContextMenu"));

    if (info.isDir()) {
        menu.addAction(newFileAction);
        menu.addAction(newFolderAction);
        menu.addSeparator();
        menu.addAction(copyPathAction);
        menu.addAction(openContainingFolderAction);
        menu.addAction(revealAction);
        menu.addAction(refreshAction);
    } else {
        menu.addAction(openAction);
        menu.addAction(renameAction);
        menu.addAction(deleteAction);
        menu.addSeparator();
        menu.addAction(copyPathAction);
        menu.addAction(openContainingFolderAction);
        menu.addAction(revealAction);
    }

    menu.exec(projectTree->viewport()->mapToGlobal(pos));
    contextIndex = QModelIndex();
}

QAction *ProjectTreeController::createAction(const QString &objectName, const QString &label, const QString &commandId)
{
    auto *action = new QAction(label, this);
    action->setObjectName(objectName);
    action->setProperty("commandId", commandId);
    return action;
}
