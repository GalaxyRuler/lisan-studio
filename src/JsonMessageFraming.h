#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QVector>

QByteArray encodeJsonMessage(const QJsonObject &payload);

class JsonMessageBuffer final
{
public:
    void append(const QByteArray &data);
    QVector<QJsonObject> takeMessages();
    void clear();

private:
    QByteArray buffer;
};
