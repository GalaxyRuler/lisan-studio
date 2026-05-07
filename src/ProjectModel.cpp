#include "ProjectModel.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

void ProjectModel::openRoot(const QString &path)
{
    root = QDir(path).absolutePath();
    projectFiles.clear();

    QDirIterator iterator(root, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString filePath = iterator.next();
        const QFileInfo info(filePath);
        const QString relative = QDir::fromNativeSeparators(QDir(root).relativeFilePath(filePath));
        const auto parts = relative.split('/', Qt::SkipEmptyParts);

        bool ignored = false;
        for (const QString &part : parts) {
            if (isIgnoredDirectoryName(part)) {
                ignored = true;
                break;
            }
        }

        if (!ignored && isOpenableFile(info.fileName())) {
            projectFiles.push_back(info.absoluteFilePath());
        }
    }

    projectFiles.sort(Qt::CaseInsensitive);
}

QString ProjectModel::rootPath() const
{
    return root;
}

QStringList ProjectModel::files() const
{
    return projectFiles;
}

bool ProjectModel::isIgnoredDirectoryName(const QString &name)
{
    static const QStringList ignored = {
        QStringLiteral(".git"),
        QStringLiteral(".cache"),
        QStringLiteral(".pytest_cache"),
        QStringLiteral(".ruff_cache"),
        QStringLiteral("__pycache__"),
        QStringLiteral("build"),
        QStringLiteral("build-debug"),
        QStringLiteral("build-release"),
        QStringLiteral("dist"),
        QStringLiteral("node_modules"),
        QStringLiteral("venv"),
        QStringLiteral(".venv"),
    };
    return ignored.contains(name, Qt::CaseInsensitive);
}

bool ProjectModel::isOpenableFile(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QStringLiteral("apy")
        || suffix == QStringLiteral("py")
        || suffix == QStringLiteral("md")
        || suffix == QStringLiteral("txt");
}
