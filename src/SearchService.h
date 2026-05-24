#pragma once

#include <QString>
#include <QVector>

struct SearchResultRow
{
    QString path;
    int line = 0;
    QString preview;
};

struct SearchResults
{
    QVector<SearchResultRow> rows;
    bool truncatedAtFileCap = false;
};

class SearchService
{
public:
    static constexpr int MaxScannedFiles = 5'000;

    QVector<SearchResultRow> search(
        const QString &rootPath,
        const QString &query,
        int limit = 250,
        int maxScannedFiles = SearchService::MaxScannedFiles) const;
    SearchResults searchWithMetadata(
        const QString &rootPath,
        const QString &query,
        int limit = 250,
        int maxScannedFiles = SearchService::MaxScannedFiles) const;
    QVector<SearchResultRow> searchText(const QString &path, const QString &text, const QString &query, int limit = 250) const;
    static QVector<SearchResultRow> mergeRows(
        const QVector<SearchResultRow> &priorityRows,
        const QVector<SearchResultRow> &secondaryRows,
        int limit = 250);
};

