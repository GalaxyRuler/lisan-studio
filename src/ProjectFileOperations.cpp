#include "ProjectFileOperations.h"

#include "ProjectModel.h"

#include <QDir>
#include <QFileInfo>

namespace {
void clearError(QString *error)
{
    if (error) {
        error->clear();
    }
}

ProjectFileOperationTarget reject(QString *error, const QString &message)
{
    if (error) {
        *error = message;
    }
    return {};
}

QString absoluteCleanPath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}
}

ProjectFileOperationTarget ProjectFileOperations::childTarget(
    const QString &folderPath,
    const QString &projectRoot,
    const QString &name,
    bool fileTarget,
    QString *error)
{
    clearError(error);
    if (!ProjectModel::isValidChildName(name)) {
        return reject(error, QString::fromUtf8("اسم غير صالح."));
    }

    const QFileInfo folderInfo(folderPath);
    if (!folderInfo.exists() || !folderInfo.isDir()) {
        return reject(error, QString::fromUtf8("المجلد غير موجود."));
    }
    if (!ProjectModel::pathIsSameOrInside(folderInfo.absoluteFilePath(), projectRoot)) {
        return reject(error, QString::fromUtf8("الهدف خارج المشروع."));
    }

    const QString targetPath = QDir(folderInfo.absoluteFilePath()).filePath(name.trimmed());
    if (!ProjectModel::pathIsSameOrInside(targetPath, projectRoot)) {
        return reject(error, QString::fromUtf8("الهدف خارج المشروع."));
    }
    if (QFileInfo::exists(targetPath)) {
        return reject(error, fileTarget
            ? QString::fromUtf8("الملف موجود.")
            : QString::fromUtf8("المجلد موجود."));
    }

    return {true, absoluteCleanPath(targetPath)};
}

ProjectFileOperationTarget ProjectFileOperations::renameTarget(
    const QString &currentPath,
    const QString &newName,
    const QString &projectRoot,
    QString *error)
{
    clearError(error);
    const QFileInfo currentInfo(currentPath);
    if (!currentInfo.exists()) {
        return reject(error, QString::fromUtf8("العنصر غير موجود."));
    }
    if (absoluteCleanPath(currentInfo.absoluteFilePath()) == absoluteCleanPath(projectRoot)) {
        return reject(error, QString::fromUtf8("لا يمكن إعادة تسمية جذر المشروع."));
    }
    if (!ProjectModel::isValidChildName(newName)) {
        return reject(error, QString::fromUtf8("اسم غير صالح."));
    }
    if (!ProjectModel::pathIsSameOrInside(currentInfo.absoluteFilePath(), projectRoot)) {
        return reject(error, QString::fromUtf8("العنصر خارج المشروع."));
    }
    if (newName.trimmed() == currentInfo.fileName()) {
        return reject(error, QString::fromUtf8("الاسم مستخدم بالفعل."));
    }

    const QString targetPath = QDir(currentInfo.absolutePath()).filePath(newName.trimmed());
    if (!ProjectModel::pathIsSameOrInside(targetPath, projectRoot)) {
        return reject(error, QString::fromUtf8("الهدف خارج المشروع."));
    }
    if (QFileInfo::exists(targetPath)) {
        return reject(error, QString::fromUtf8("الاسم مستخدم."));
    }

    return {true, absoluteCleanPath(targetPath)};
}

bool ProjectFileOperations::canDelete(const QString &path, const QString &projectRoot, QString *error)
{
    clearError(error);
    const QFileInfo info(path);
    if (!info.exists()) {
        if (error) {
            *error = QString::fromUtf8("العنصر غير موجود.");
        }
        return false;
    }
    if (absoluteCleanPath(info.absoluteFilePath()) == absoluteCleanPath(projectRoot)) {
        if (error) {
            *error = QString::fromUtf8("لا يمكن حذف جذر المشروع.");
        }
        return false;
    }
    if (!ProjectModel::pathIsSameOrInside(info.absoluteFilePath(), projectRoot)) {
        if (error) {
            *error = QString::fromUtf8("العنصر خارج المشروع.");
        }
        return false;
    }
    return true;
}

QString ProjectFileOperations::pathForClipboard(const QString &path)
{
    return QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
}

QString ProjectFileOperations::containingFolder(const QString &path)
{
    const QFileInfo info(path);
    return info.isDir() ? info.absoluteFilePath() : info.absolutePath();
}
