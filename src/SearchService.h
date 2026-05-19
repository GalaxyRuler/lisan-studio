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
    QVector<SearchResultRow> search(const QString &rootPath, const QString &query, int limit = 250) const;
    SearchResults searchWithMetadata(const QString &rootPath, const QString &query, int limit = 250) const;
    QVector<SearchResultRow> searchText(const QString &path, const QString &text, const QString &query, int limit = 250) const;
    static QVector<SearchResultRow> mergeRows(
        const QVector<SearchResultRow> &priorityRows,
        const QVector<SearchResultRow> &secondaryRows,
        int limit = 250);
};

