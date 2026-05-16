#pragma once

#include <QString>
#include <QVector>

struct EditorFindMatch
{
    int start = -1;
    int length = 0;
};

class EditorFindService final
{
public:
    static QVector<EditorFindMatch> findAll(
        const QString &text,
        const QString &query,
        Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive);
    static bool replaceAt(QString *text, const EditorFindMatch &match, const QString &replacement);
    static QString replaceAll(
        const QString &text,
        const QString &query,
        const QString &replacement,
        int *replacedCount = nullptr,
        Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive);
};
