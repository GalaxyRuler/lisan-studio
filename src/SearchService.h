#pragma once

#include <QString>
#include <QVector>

struct SearchResultRow
{
    QString path;
    int line = 0;
    QString preview;
};

class SearchService
{
public:
    QVector<SearchResultRow> search(const QString &rootPath, const QString &query, int limit = 250) const;
};

