#pragma once

#include <QModelIndex>
#include <QObject>
#include <QString>

#include <functional>

class QFileSystemModel;
class QTreeView;
class QAction;

class ProjectTreeController final : public QObject
{
    Q_OBJECT

public:
    using DeleteConfirmationCallback = std::function<bool(const QString &path, bool isDirectory)>;

    ProjectTreeController(QTreeView *tree, QFileSystemModel *model, QObject *parent = nullptr);

    void setProjectRoot(const QString &path);
    void refresh();
    QString currentSelectionPath() const;
    QString currentSelectionFolderPath() const;
    void setDeleteConfirmationCallback(DeleteConfirmationCallback callback);

public slots:
    void openSelectedItem();
    void createFile();
    void createFolder();
    void renameSelectedItem();
    void deleteSelectedItem();
    void revealSelectedItem();
    void copySelectedItemPath();
    void openSelectedContainingFolder();

signals:
    void openPathRequested(QString path);
    void revealRequested(QString path);
    void pathRenamed(QString oldPath, QString newPath);
    void pathDeleted(QString path);
    void statusMessage(QString message);
    void errorMessage(QString title, QString body);

private:
    QTreeView *projectTree = nullptr;
    QFileSystemModel *fileSystemModel = nullptr;
    QString projectRoot;
    QModelIndex contextIndex;
    DeleteConfirmationCallback confirmDelete;
    QAction *newFileAction = nullptr;
    QAction *newFolderAction = nullptr;
    QAction *openAction = nullptr;
    QAction *renameAction = nullptr;
    QAction *deleteAction = nullptr;
    QAction *revealAction = nullptr;
    QAction *copyPathAction = nullptr;
    QAction *openContainingFolderAction = nullptr;
    QAction *refreshAction = nullptr;

    QModelIndex activeIndex() const;
    QString activePath() const;
    QString activeFolderPath() const;
    void showContextMenu(const QPoint &pos);
    QAction *createAction(const QString &objectName, const QString &label, const QString &commandId);
};
