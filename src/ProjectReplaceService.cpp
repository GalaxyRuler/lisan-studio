#include "ProjectReplaceService.h"

#include "EditorFindService.h"
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

void appendSummary(ProjectReplacePreview *preview, const QString &path, int rowCount, int matchCount)
{
    if (!preview || rowCount <= 0 || matchCount <= 0) {
        return;
    }
    preview->summaries.push_back({path, rowCount, matchCount});
    preview->totalMatches += matchCount;
}
}

ProjectReplaceSelectionState::ProjectReplaceSelectionState(const QVector<ProjectReplacePreviewRow> &previewRows)
    : rows(previewRows)
    , accepted(previewRows.size(), true)
{
}

bool ProjectReplaceSelectionState::isRowAccepted(int rowIndex) const
{
    return rowIndex >= 0 && rowIndex < accepted.size() && accepted.at(rowIndex);
}

void ProjectReplaceSelectionState::setRowAccepted(int rowIndex, bool rowAccepted)
{
    if (rowIndex < 0 || rowIndex >= accepted.size()) {
        return;
    }
    accepted[rowIndex] = rowAccepted;
}

void ProjectReplaceSelectionState::setFileAccepted(const QString &path, bool fileAccepted)
{
    for (int i = 0; i < rows.size(); ++i) {
        if (rows.at(i).path == path) {
            accepted[i] = fileAccepted;
        }
    }
}

QVector<ProjectReplacePreviewRow> ProjectReplaceSelectionState::acceptedRows() const
{
    QVector<ProjectReplacePreviewRow> result;
    for (int i = 0; i < rows.size(); ++i) {
        if (isRowAccepted(i)) {
            result.push_back(rows.at(i));
        }
    }
    return result;
}

int ProjectReplaceSelectionState::acceptedMatchCount() const
{
    int total = 0;
    for (int i = 0; i < rows.size(); ++i) {
        if (isRowAccepted(i)) {
            total += rows.at(i).matchCount;
        }
    }
    return total;
}

ProjectReplacePreview ProjectReplaceService::previewText(
    const QString &path,
    const QString &text,
    const QString &query,
    const QString &replacement,
    int limit) const
{
    ProjectReplacePreview preview;
    if (query.trimmed().isEmpty() || limit <= 0) {
        return preview;
    }

    int lineNumber = 0;
    int rowCount = 0;
    int matchCount = 0;
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        ++lineNumber;
        const auto matches = EditorFindService::findAll(line, query);
        if (matches.isEmpty()) {
            continue;
        }

        int replacedOnLine = 0;
        const QString after = EditorFindService::replaceAll(line, query, replacement, &replacedOnLine);
        preview.rows.push_back({path, lineNumber, line.trimmed(), after.trimmed(), replacedOnLine});
        ++rowCount;
        matchCount += replacedOnLine;
        if (preview.rows.size() >= limit) {
            break;
        }
    }

    appendSummary(&preview, path, rowCount, matchCount);
    return preview;
}

ProjectReplacePreview ProjectReplaceService::previewProject(
    const QString &rootPath,
    const QString &query,
    const QString &replacement,
    int limit) const
{
    ProjectReplacePreview preview;
    if (query.trimmed().isEmpty() || limit <= 0) {
        return preview;
    }

    int scannedFiles = 0;
    QDirIterator iterator(QDir(rootPath).absolutePath(), QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (iterator.hasNext() && scannedFiles < MaxScannedFiles && preview.rows.size() < limit) {
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

        const QString text = QString::fromUtf8(file.readAll());
        const ProjectReplacePreview filePreview = previewText(path, text, query, replacement, limit - preview.rows.size());
        if (filePreview.rows.isEmpty()) {
            continue;
        }

        preview.rows += filePreview.rows;
        preview.summaries += filePreview.summaries;
        preview.totalMatches += filePreview.totalMatches;
    }
    preview.scannedFiles = scannedFiles;
    return preview;
}

QVector<ProjectReplacePreviewRow> ProjectReplaceService::mergePreviewRows(
    const QVector<ProjectReplacePreviewRow> &priorityRows,
    const QVector<ProjectReplacePreviewRow> &secondaryRows,
    int limit)
{
    QVector<ProjectReplacePreviewRow> rows = priorityRows;
    if (rows.size() > limit) {
        rows.resize(limit);
        return rows;
    }

    for (const ProjectReplacePreviewRow &row : secondaryRows) {
        bool duplicate = false;
        for (const ProjectReplacePreviewRow &existing : std::as_const(rows)) {
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

ProjectReplaceSelectionState ProjectReplaceService::selectionFromRows(const QVector<ProjectReplacePreviewRow> &rows)
{
    return ProjectReplaceSelectionState(rows);
}
