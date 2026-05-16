#pragma once

#include <QString>
#include <QVector>

struct TerminalLink
{
    QString path;
    int line = 0;
    int column = 0;
    int start = 0;
    int length = 0;
};

class TerminalLinkParser final
{
public:
    static QVector<TerminalLink> linksForText(const QString &text);
};
