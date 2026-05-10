#include "SearchService.h"

#include "ProjectModel.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

namespace {
constexpr int MaxScannedFiles = 500;
constexpr qint64 MaxFileBytes = 1024 * 1024;

bool hasIgnoredDirectoryPart(const QString &rootPath, const QString &filePath)
{
    const QString relative = QDir::fromNativeSeparators(QDir(rootPath).relativeFilePath(filePath));
    const auto parts = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        if (ProjectModel::isIgnoredDirectoryName(part)) {
            return true;
        }
    }
    return false;
}
}

QVector<SearchResultRow> SearchService::search(const QString &rootPath, const QString &query, int limit) const
{
    QVector<SearchResultRow> rows;
    if (query.trimmed().isEmpty()) {
        return rows;
    }

    int scannedFiles = 0;
    QDirIterator iterator(QDir(rootPath).absolutePath(), QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (iterator.hasNext() && scannedFiles < MaxScannedFiles) {
        const QString path = iterator.next();
        const QFileInfo info(path);
        if (hasIgnoredDirectoryPart(rootPath, path)
            || !ProjectModel::isOpenableFile(info.fileName())
            || info.size() > MaxFileBytes) {
            continue;
        }

        ++scannedFiles;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        int lineNumber = 0;
        while (!file.atEnd()) {
            ++lineNumber;
            const QString line = QString::fromUtf8(file.readLine()).trimmed();
            if (line.contains(query, Qt::CaseInsensitive)) {
                rows.push_back({path, lineNumber, line});
                if (rows.size() >= limit) {
                    return rows;
                }
            }
        }
    }

    return rows;
}
