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
    QString module;
    QStringList arguments;
    QString workingDirectory;
    bool stopOnEntry = false;
};

struct DapStackFrame
{
    int id = 0;
    QString name;
    QString sourcePath;
    int line = 0;
    int column = 0;
};

struct DapScope
{
    QString name;
    int variablesReference = 0;
    bool expensive = false;
};

struct DapVariable
{
    QString name;
    QString value;
    QString type;
    int variablesReference = 0;
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

    int beginLaunch(const DapLaunchRequest &request, QString *error = nullptr);
    bool launch(const DapLaunchRequest &request, int timeoutMs = 5000, QString *error = nullptr);
    bool waitForRequest(int requestSequence, int timeoutMs = 5000, QString *error = nullptr);
    bool waitForInitialized(int timeoutMs = 5000, QString *error = nullptr);
    bool setBreakpoints(const QString &sourcePath, const QVector<int> &lines, int timeoutMs = 5000, QString *error = nullptr);
    bool configurationDone(int timeoutMs = 5000, QString *error = nullptr);
    bool continueExecution(int threadId, int timeoutMs = 5000, QString *error = nullptr);
    bool stepOver(int threadId, int timeoutMs = 5000, QString *error = nullptr);
    bool stepInto(int threadId, int timeoutMs = 5000, QString *error = nullptr);
    bool stepOut(int threadId, int timeoutMs = 5000, QString *error = nullptr);
    QVector<DapStackFrame> stackTrace(int threadId, int timeoutMs = 5000, QString *error = nullptr);
    QVector<DapScope> scopes(int frameId, int timeoutMs = 5000, QString *error = nullptr);
    QVector<DapVariable> variables(int variablesReference, int timeoutMs = 5000, QString *error = nullptr);
    DapVariable evaluate(const QString &expression, int frameId, const QString &context = QStringLiteral("watch"), int timeoutMs = 5000, QString *error = nullptr);
    void disconnect(int timeoutMs = 2000);

signals:
    void stopped(const QString &reason, int threadId);
    void continued(int threadId);
    void initialized();
    void terminated();

private:
    DapServerCommand command;
    QProcess process;
    JsonMessageBuffer incomingBuffer;
    int nextSequence = 1;
    DapInitializeResult lastInitializeResult;
    QMap<int, QJsonObject> responses;
    bool initializedEventSeen = false;

    int sendRequest(const QString &commandName, const QJsonObject &arguments = {});
    void writePayload(const QJsonObject &payload);
    bool waitForResponse(int requestSequence, QJsonObject *response, int timeoutMs, QString *error);
    void readAvailableMessages();
    bool requestThreadCommand(const QString &commandName, int threadId, int timeoutMs, QString *error);
    bool responseSucceeded(const QJsonObject &response, QString *error) const;
};
