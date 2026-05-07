#include "SearchService.h"

#include "ProjectModel.h"

#include <QFile>

QVector<SearchResultRow> SearchService::search(const QString &rootPath, const QString &query, int limit) const
{
    QVector<SearchResultRow> rows;
    if (query.trimmed().isEmpty()) {
        return rows;
    }

    ProjectModel model;
    model.openRoot(rootPath);

    for (const QString &path : model.files()) {
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

