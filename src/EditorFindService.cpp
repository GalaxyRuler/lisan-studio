#include "EditorFindService.h"

#include <QtGlobal>

QVector<EditorFindMatch> EditorFindService::findAll(
    const QString &text,
    const QString &query,
    Qt::CaseSensitivity caseSensitivity)
{
    QVector<EditorFindMatch> matches;
    if (query.isEmpty()) {
        return matches;
    }

    int start = 0;
    while (start <= text.size()) {
        const int index = text.indexOf(query, start, caseSensitivity);
        if (index < 0) {
            break;
        }
        const int length = static_cast<int>(query.size());
        matches.push_back({index, length});
        start = index + qMax(1, length);
    }
    return matches;
}

bool EditorFindService::replaceAt(QString *text, const EditorFindMatch &match, const QString &replacement)
{
    if (!text || match.start < 0 || match.length < 0 || match.start + match.length > text->size()) {
        return false;
    }

    text->replace(match.start, match.length, replacement);
    return true;
}

QString EditorFindService::replaceAll(
    const QString &text,
    const QString &query,
    const QString &replacement,
    int *replacedCount,
    Qt::CaseSensitivity caseSensitivity)
{
    if (replacedCount) {
        *replacedCount = 0;
    }
    if (query.isEmpty()) {
        return text;
    }

    QString result;
    int cursor = 0;
    const auto matches = findAll(text, query, caseSensitivity);
    for (const EditorFindMatch &match : matches) {
        result += text.mid(cursor, match.start - cursor);
        result += replacement;
        cursor = match.start + match.length;
    }
    result += text.mid(cursor);
    if (replacedCount) {
        *replacedCount = matches.size();
    }
    return result;
}
