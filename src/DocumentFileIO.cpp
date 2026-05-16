#include "DocumentFileIO.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringDecoder>

DocumentLoadResult DocumentFileIO::loadUtf8(const QString &path, QString *error)
{
    if (error) {
        error->clear();
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return {};
    }

    const QByteArray bytes = file.readAll();
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString text = decoder.decode(bytes);
    if (decoder.hasError()) {
        if (error) {
            *error = QString::fromUtf8("الملف ليس UTF-8 صالحا: %1").arg(QDir::toNativeSeparators(path));
        }
        return {};
    }

    DocumentLoadResult result;
    result.text = text;
    result.identity = identityForPath(path);
    result.lineEnding = detectLineEnding(text);
    return result;
}

bool DocumentFileIO::saveUtf8Atomically(const QString &path, const QString &text, QString *error)
{
    if (error) {
        error->clear();
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }

    file.write(text.toUtf8());
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

DocumentFileIdentity DocumentFileIO::identityForPath(const QString &path)
{
    const QFileInfo info(path);
    DocumentFileIdentity identity;
    identity.path = info.absoluteFilePath();
    identity.sizeBytes = info.exists() ? info.size() : -1;
    identity.lastModifiedUtc = info.exists() ? info.lastModified().toUTC() : QDateTime();
    return identity;
}

DocumentLineEnding DocumentFileIO::detectLineEnding(const QString &text)
{
    const bool hasCrlf = text.contains(QStringLiteral("\r\n"));
    QString withoutCrlf = text;
    withoutCrlf.replace(QStringLiteral("\r\n"), QString());
    const bool hasLf = withoutCrlf.contains(QLatin1Char('\n'));

    if (hasCrlf && hasLf) {
        return DocumentLineEnding::Mixed;
    }
    if (hasCrlf) {
        return DocumentLineEnding::Crlf;
    }
    if (hasLf) {
        return DocumentLineEnding::Lf;
    }
    return DocumentLineEnding::None;
}
