#include "SearchService.h"

#include "ProjectModel.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include <utility>

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

bool isScannableFile(const QString &rootPath, const QString &path)
{
    const QFileInfo info(path);
    return !hasIgnoredDirectoryPart(rootPath, path)
        && ProjectModel::isOpenableFile(info.fileName())
        && info.size() <= MaxFileBytes;
}

bool hasMoreScannableFile(QDirIterator &iterator, const QString &rootPath)
{
    while (iterator.hasNext()) {
        if (isScannableFile(rootPath, iterator.next())) {
            return true;
        }
    }
    return false;
}
}

QVector<SearchResultRow> SearchService::search(const QString &rootPath, const QString &query, int limit) const
{
    return searchWithMetadata(rootPath, query, limit).rows;
}

SearchResults SearchService::searchWithMetadata(const QString &rootPath, const QString &query, int limit) const
{
    SearchResults result;
    if (query.trimmed().isEmpty() || limit <= 0) {
        return result;
    }

    int scannedFiles = 0;
    QDirIterator iterator(QDir(rootPath).absolutePath(), QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (iterator.hasNext() && scannedFiles < MaxScannedFiles) {
        const QString path = iterator.next();
        if (!isScannableFile(rootPath, path)) {
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
                result.rows.push_back({path, lineNumber, line});
                if (result.rows.size() >= limit) {
                    return result;
                }
            }
        }
    }

    result.truncatedAtFileCap = scannedFiles == MaxScannedFiles && hasMoreScannableFile(iterator, rootPath);
    return result;
}

QVector<SearchResultRow> SearchService::searchText(const QString &path, const QString &text, const QString &query, int limit) const
{
    QVector<SearchResultRow> rows;
    if (query.trimmed().isEmpty()) {
        return rows;
    }

    const QStringList lines = text.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i) {
        const QString preview = lines.at(i).trimmed();
        if (preview.contains(query, Qt::CaseInsensitive)) {
            rows.push_back({path, i + 1, preview});
            if (rows.size() >= limit) {
                return rows;
            }
        }
    }
    return rows;
}

QVector<SearchResultRow> SearchService::mergeRows(
    const QVector<SearchResultRow> &priorityRows,
    const QVector<SearchResultRow> &secondaryRows,
    int limit)
{
    QVector<SearchResultRow> rows = priorityRows;
    if (rows.size() > limit) {
        rows.resize(limit);
        return rows;
    }

    for (const auto &row : secondaryRows) {
        bool duplicate = false;
        for (const auto &existing : std::as_const(rows)) {
            if (existing.path == row.path && existing.line == row.line) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            rows.push_back(row);
            if (rows.size() >= limit) {
                return rows;
            }
        }
    }
    return rows;
}
