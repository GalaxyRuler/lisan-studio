#include "RuntimeProblemParser.h"

#include <QRegularExpression>
#include <QStringList>

RuntimeProblemDetail parseRuntimeProblemDetail(const QString &runtimeText, const QString &actionTitle, int exitCode)
{
    RuntimeProblemDetail detail;
    detail.message = QString::fromUtf8("%1 انتهى برمز خروج %2").arg(actionTitle).arg(exitCode);

    const QStringList lines = runtimeText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    for (auto it = lines.crbegin(); it != lines.crend(); ++it) {
        const QString line = it->trimmed();
        if (line.contains(QString::fromUtf8("خطأ")) || line.contains(QStringLiteral("Error"), Qt::CaseInsensitive)) {
            detail.message = QString::fromUtf8("%1 - رمز الخروج %2").arg(line).arg(exitCode);
            break;
        }
    }

    const QRegularExpression linePattern(QString::fromUtf8("(?:السطر|line)\\s+(\\d+)"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = linePattern.match(detail.message);
    if (match.hasMatch()) {
        detail.line = match.captured(1).toInt();
    }

    return detail;
}
