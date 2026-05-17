#pragma once

#include <QString>
#include <QStringList>

struct ProjectFileOperationTarget
{
    bool allowed = false;
    QString path;
};

class ProjectFileOperations final
{
public:
    static ProjectFileOperationTarget childTarget(
        const QString &folderPath,
        const QString &projectRoot,
        const QString &name,
        bool fileTarget,
        QString *error = nullptr);
    static ProjectFileOperationTarget renameTarget(
        const QString &currentPath,
        const QString &newName,
        const QString &projectRoot,
        QString *error = nullptr);
    static bool canDelete(const QString &path, const QString &projectRoot, QString *error = nullptr);
    static QString pathForClipboard(const QString &path);
    static QString containingFolder(const QString &path);
    static QStringList explorerRevealArguments(const QString &path);
};
