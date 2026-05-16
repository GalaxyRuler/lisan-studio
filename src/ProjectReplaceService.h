#pragma once

#include <QString>
#include <QVector>

struct ProjectReplacePreviewRow
{
    QString path;
    int line = 0;
    QString before;
    QString after;
    int matchCount = 0;
};

struct ProjectReplaceFileSummary
{
    QString path;
    int rowCount = 0;
    int matchCount = 0;
};

struct ProjectReplacePreview
{
    QVector<ProjectReplacePreviewRow> rows;
    QVector<ProjectReplaceFileSummary> summaries;
    int totalMatches = 0;
    int scannedFiles = 0;
};

class ProjectReplaceService final
{
public:
    ProjectReplacePreview previewText(
        const QString &path,
        const QString &text,
        const QString &query,
        const QString &replacement,
        int limit = 250) const;

    ProjectReplacePreview previewProject(
        const QString &rootPath,
        const QString &query,
        const QString &replacement,
        int limit = 250) const;

    static QVector<ProjectReplacePreviewRow> mergePreviewRows(
        const QVector<ProjectReplacePreviewRow> &priorityRows,
        const QVector<ProjectReplacePreviewRow> &secondaryRows,
        int limit = 250);
};
