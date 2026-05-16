#pragma once

#include <QString>
#include <QVector>

struct ApySnippet
{
    QString id;
    QString title;
    QString body;
    int cursorOffset = -1;
};

class ApySnippetService final
{
public:
    static QVector<ApySnippet> snippets();
    static bool snippetById(const QString &id, ApySnippet *snippet);
};
