#pragma once

#include <QString>

struct RuntimeProblemDetail
{
    QString message;
    int line = 0;
};

RuntimeProblemDetail parseRuntimeProblemDetail(const QString &runtimeText, const QString &actionTitle, int exitCode);
