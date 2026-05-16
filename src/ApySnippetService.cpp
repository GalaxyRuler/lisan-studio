#include "ApySnippetService.h"

QVector<ApySnippet> ApySnippetService::snippets()
{
    return {
        {
            QStringLiteral("apy.print"),
            QString::fromUtf8("اطبع"),
            QString::fromUtf8("اطبع(\"\")"),
            static_cast<int>(QString::fromUtf8("اطبع(\"").size()),
        },
        {
            QStringLiteral("apy.if"),
            QString::fromUtf8("إذا"),
            QString::fromUtf8("اذا شرط:\n    \n"),
            static_cast<int>(QString::fromUtf8("اذا شرط:\n    ").size()),
        },
    };
}

bool ApySnippetService::snippetById(const QString &id, ApySnippet *snippet)
{
    const QVector<ApySnippet> allSnippets = snippets();
    for (const ApySnippet &candidate : allSnippets) {
        if (candidate.id == id) {
            if (snippet) {
                *snippet = candidate;
            }
            return true;
        }
    }
    return false;
}
