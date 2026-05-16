#include "TerminalLinkParser.h"

#include <QRegularExpression>

QVector<TerminalLink> TerminalLinkParser::linksForText(const QString &text)
{
    static const QRegularExpression linkPattern(
        QStringLiteral(R"(\b([A-Za-z]:(?:[\\/][^\\/:*?"<>|\r\n]+)+)(?::(\d+)(?::(\d+))?)?)"));

    QVector<TerminalLink> links;
    QRegularExpressionMatchIterator matches = linkPattern.globalMatch(text);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        TerminalLink link;
        link.path = match.captured(1);
        link.line = match.captured(2).isEmpty() ? 0 : match.captured(2).toInt();
        link.column = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();
        link.start = match.capturedStart(0);
        link.length = match.capturedLength(0);
        links.push_back(link);
    }
    return links;
}
