#include "JsonMessageFraming.h"

#include <QJsonDocument>
#include <QJsonParseError>

QByteArray encodeJsonMessage(const QJsonObject &payload)
{
    const QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    return QByteArrayLiteral("Content-Length: ")
        + QByteArray::number(body.size())
        + QByteArrayLiteral("\r\n\r\n")
        + body;
}

void JsonMessageBuffer::append(const QByteArray &data)
{
    buffer.append(data);
}

QVector<QJsonObject> JsonMessageBuffer::takeMessages()
{
    QVector<QJsonObject> messages;
    while (true) {
        const int headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return messages;
        }

        const QByteArray header = buffer.left(headerEnd);
        int contentLength = -1;
        const QList<QByteArray> lines = header.split('\n');
        for (QByteArray line : lines) {
            line = line.trimmed();
            const int separator = line.indexOf(':');
            if (separator < 0) {
                continue;
            }
            const QByteArray name = line.left(separator).trimmed().toLower();
            if (name == QByteArrayLiteral("content-length")) {
                contentLength = line.mid(separator + 1).trimmed().toInt();
                break;
            }
        }

        if (contentLength < 0) {
            buffer.remove(0, headerEnd + 4);
            continue;
        }

        const int messageStart = headerEnd + 4;
        if (buffer.size() < messageStart + contentLength) {
            return messages;
        }

        const QByteArray body = buffer.mid(messageStart, contentLength);
        buffer.remove(0, messageStart + contentLength);

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error == QJsonParseError::NoError && document.isObject()) {
            messages.append(document.object());
        }
    }
}

void JsonMessageBuffer::clear()
{
    buffer.clear();
}
