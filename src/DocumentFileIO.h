#pragma once

#include <QDateTime>
#include <QString>

enum class DocumentEncoding
{
    Utf8
};

enum class DocumentLineEnding
{
    Lf,
    Crlf,
    Mixed,
    None
};

struct DocumentFileIdentity
{
    QString path;
    qint64 sizeBytes = -1;
    QDateTime lastModifiedUtc;
};

struct DocumentLoadResult
{
    QString text;
    DocumentFileIdentity identity;
    DocumentEncoding encoding = DocumentEncoding::Utf8;
    DocumentLineEnding lineEnding = DocumentLineEnding::None;
};

class DocumentFileIO final
{
public:
    static DocumentLoadResult loadUtf8(const QString &path, QString *error = nullptr);
    static bool saveUtf8Atomically(const QString &path, const QString &text, QString *error = nullptr);
    static DocumentFileIdentity identityForPath(const QString &path);
    static DocumentLineEnding detectLineEnding(const QString &text);
};
