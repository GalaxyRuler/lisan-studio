#pragma once

#include "JsonMessageFraming.h"

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

struct DapServerCommand
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment;
};

struct DapInitializeResult
{
    bool success = false;
    QString adapterId;
    bool supportsConfigurationDoneRequest = false;
    bool supportsRestartRequest = false;
};

struct DapLaunchRequest
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
    bool stopOnEntry = false;
};

class DapClient final : public QObject
{
    Q_OBJECT

public:
    explicit DapClient(QObject *parent = nullptr);
    ~DapClient() override;

    void setServerCommand(const DapServerCommand &command);
    DapServerCommand serverCommand() const;

    bool startAndInitialize(int timeoutMs = 5000, QString *error = nullptr);
    bool isRunning() const;
    DapInitializeResult initializeResult() const;

    bool launch(const DapLaunchRequest &request, int timeoutMs = 5000, QString *error = nullptr);
    void disconnect(int timeoutMs = 2000);

private:
    DapServerCommand command;
    QProcess process;
    JsonMessageBuffer incomingBuffer;
    int nextSequence = 1;
    DapInitializeResult lastInitializeResult;
    QMap<int, QJsonObject> responses;

    int sendRequest(const QString &commandName, const QJsonObject &arguments = {});
    void writePayload(const QJsonObject &payload);
    bool waitForResponse(int requestSequence, QJsonObject *response, int timeoutMs, QString *error);
    void readAvailableMessages();
    bool responseSucceeded(const QJsonObject &response, QString *error) const;
};
