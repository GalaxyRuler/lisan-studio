#include "DapClient.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>

DapClient::DapClient(QObject *parent)
    : QObject(parent)
{
    connect(&process, &QProcess::readyReadStandardOutput, this, [this]() {
        readAvailableMessages();
    });
}

DapClient::~DapClient()
{
    disconnect();
}

void DapClient::setServerCommand(const DapServerCommand &serverCommand)
{
    command = serverCommand;
}

DapServerCommand DapClient::serverCommand() const
{
    return command;
}

bool DapClient::startAndInitialize(int timeoutMs, QString *error)
{
    if (error) {
        error->clear();
    }
    if (command.program.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("DAP server program is empty.");
        }
        return false;
    }

    if (process.state() == QProcess::NotRunning) {
        process.setProgram(command.program);
        process.setArguments(command.arguments);
        if (!command.workingDirectory.isEmpty()) {
            process.setWorkingDirectory(command.workingDirectory);
        }
        process.setProcessEnvironment(command.environment.isEmpty() ? QProcessEnvironment::systemEnvironment() : command.environment);
        process.start();
        if (!process.waitForStarted(timeoutMs)) {
            if (error) {
                *error = process.errorString();
            }
            return false;
        }
    }

    QJsonObject arguments;
    arguments.insert(QStringLiteral("clientID"), QStringLiteral("lisan-studio"));
    arguments.insert(QStringLiteral("clientName"), QStringLiteral("Lisan Studio"));
    arguments.insert(QStringLiteral("adapterID"), QStringLiteral("debugpy"));
    arguments.insert(QStringLiteral("pathFormat"), QStringLiteral("path"));
    arguments.insert(QStringLiteral("linesStartAt1"), true);
    arguments.insert(QStringLiteral("columnsStartAt1"), true);
    arguments.insert(QStringLiteral("supportsVariableType"), true);

    const int requestSequence = sendRequest(QStringLiteral("initialize"), arguments);
    QJsonObject response;
    if (!waitForResponse(requestSequence, &response, timeoutMs, error)) {
        return false;
    }
    if (!responseSucceeded(response, error)) {
        return false;
    }

    const QJsonObject body = response.value(QStringLiteral("body")).toObject();
    DapInitializeResult parsed;
    parsed.success = true;
    parsed.adapterId = arguments.value(QStringLiteral("adapterID")).toString();
    parsed.supportsConfigurationDoneRequest = body.value(QStringLiteral("supportsConfigurationDoneRequest")).toBool();
    parsed.supportsRestartRequest = body.value(QStringLiteral("supportsRestartRequest")).toBool();
    lastInitializeResult = parsed;
    return true;
}

bool DapClient::isRunning() const
{
    return process.state() != QProcess::NotRunning;
}

DapInitializeResult DapClient::initializeResult() const
{
    return lastInitializeResult;
}

bool DapClient::launch(const DapLaunchRequest &request, int timeoutMs, QString *error)
{
    if (error) {
        error->clear();
    }
    if (process.state() == QProcess::NotRunning) {
        if (error) {
            *error = QStringLiteral("DAP server is not running.");
        }
        return false;
    }

    QJsonArray args;
    for (const QString &argument : request.arguments) {
        args.append(argument);
    }

    QJsonObject arguments;
    arguments.insert(QStringLiteral("program"), request.program);
    arguments.insert(QStringLiteral("args"), args);
    arguments.insert(QStringLiteral("cwd"), request.workingDirectory);
    arguments.insert(QStringLiteral("stopOnEntry"), request.stopOnEntry);
    arguments.insert(QStringLiteral("console"), QStringLiteral("internalConsole"));

    const int requestSequence = sendRequest(QStringLiteral("launch"), arguments);
    QJsonObject response;
    if (!waitForResponse(requestSequence, &response, timeoutMs, error)) {
        return false;
    }
    return responseSucceeded(response, error);
}

bool DapClient::setBreakpoints(const QString &sourcePath, const QVector<int> &lines, int timeoutMs, QString *error)
{
    if (error) {
        error->clear();
    }
    if (process.state() == QProcess::NotRunning) {
        if (error) {
            *error = QStringLiteral("DAP server is not running.");
        }
        return false;
    }

    QJsonArray breakpoints;
    for (const int line : lines) {
        if (line > 0) {
            breakpoints.append(QJsonObject{{QStringLiteral("line"), line}});
        }
    }

    QJsonObject source;
    source.insert(QStringLiteral("path"), sourcePath);

    QJsonObject arguments;
    arguments.insert(QStringLiteral("source"), source);
    arguments.insert(QStringLiteral("breakpoints"), breakpoints);

    const int requestSequence = sendRequest(QStringLiteral("setBreakpoints"), arguments);
    QJsonObject response;
    if (!waitForResponse(requestSequence, &response, timeoutMs, error)) {
        return false;
    }
    return responseSucceeded(response, error);
}

bool DapClient::configurationDone(int timeoutMs, QString *error)
{
    const int requestSequence = sendRequest(QStringLiteral("configurationDone"), QJsonObject{});
    QJsonObject response;
    if (!waitForResponse(requestSequence, &response, timeoutMs, error)) {
        return false;
    }
    return responseSucceeded(response, error);
}

bool DapClient::continueExecution(int threadId, int timeoutMs, QString *error)
{
    return requestThreadCommand(QStringLiteral("continue"), threadId, timeoutMs, error);
}

bool DapClient::stepOver(int threadId, int timeoutMs, QString *error)
{
    return requestThreadCommand(QStringLiteral("next"), threadId, timeoutMs, error);
}

bool DapClient::stepInto(int threadId, int timeoutMs, QString *error)
{
    return requestThreadCommand(QStringLiteral("stepIn"), threadId, timeoutMs, error);
}

bool DapClient::stepOut(int threadId, int timeoutMs, QString *error)
{
    return requestThreadCommand(QStringLiteral("stepOut"), threadId, timeoutMs, error);
}

void DapClient::disconnect(int timeoutMs)
{
    if (process.state() == QProcess::NotRunning) {
        return;
    }

    const int requestSequence = sendRequest(QStringLiteral("disconnect"), QJsonObject{{QStringLiteral("terminateDebuggee"), true}});
    QJsonObject response;
    QString ignoredError;
    waitForResponse(requestSequence, &response, timeoutMs, &ignoredError);
    process.waitForFinished(timeoutMs);
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(1000);
    }
}

int DapClient::sendRequest(const QString &commandName, const QJsonObject &arguments)
{
    const int sequence = nextSequence++;
    QJsonObject payload;
    payload.insert(QStringLiteral("seq"), sequence);
    payload.insert(QStringLiteral("type"), QStringLiteral("request"));
    payload.insert(QStringLiteral("command"), commandName);
    if (!arguments.isEmpty()) {
        payload.insert(QStringLiteral("arguments"), arguments);
    }
    writePayload(payload);
    return sequence;
}

void DapClient::writePayload(const QJsonObject &payload)
{
    if (process.state() == QProcess::NotRunning) {
        return;
    }
    process.write(encodeJsonMessage(payload));
    process.waitForBytesWritten(1000);
}

bool DapClient::waitForResponse(int requestSequence, QJsonObject *response, int timeoutMs, QString *error)
{
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        readAvailableMessages();
        const auto it = responses.find(requestSequence);
        if (it != responses.end()) {
            if (response) {
                *response = it.value();
            }
            responses.erase(it);
            return true;
        }

        if (process.state() == QProcess::NotRunning) {
            if (error) {
                *error = QString::fromUtf8(process.readAllStandardError()).trimmed();
                if (error->isEmpty()) {
                    *error = process.errorString();
                }
            }
            return false;
        }

        process.waitForReadyRead(25);
    }

    if (error) {
        *error = QStringLiteral("Timed out waiting for DAP response.");
    }
    return false;
}

void DapClient::readAvailableMessages()
{
    incomingBuffer.append(process.readAllStandardOutput());
    const QVector<QJsonObject> messages = incomingBuffer.takeMessages();
    for (const QJsonObject &object : messages) {
        if (object.value(QStringLiteral("type")).toString() == QStringLiteral("response")) {
            responses.insert(object.value(QStringLiteral("request_seq")).toInt(), object);
            continue;
        }
        if (object.value(QStringLiteral("type")).toString() != QStringLiteral("event")) {
            continue;
        }
        const QString event = object.value(QStringLiteral("event")).toString();
        const QJsonObject body = object.value(QStringLiteral("body")).toObject();
        if (event == QStringLiteral("stopped")) {
            emit stopped(body.value(QStringLiteral("reason")).toString(), body.value(QStringLiteral("threadId")).toInt());
        } else if (event == QStringLiteral("continued")) {
            emit continued(body.value(QStringLiteral("threadId")).toInt());
        } else if (event == QStringLiteral("terminated")) {
            emit terminated();
        }
    }
}

bool DapClient::requestThreadCommand(const QString &commandName, int threadId, int timeoutMs, QString *error)
{
    if (error) {
        error->clear();
    }
    if (process.state() == QProcess::NotRunning) {
        if (error) {
            *error = QStringLiteral("DAP server is not running.");
        }
        return false;
    }

    const int requestSequence = sendRequest(commandName, QJsonObject{{QStringLiteral("threadId"), threadId}});
    QJsonObject response;
    if (!waitForResponse(requestSequence, &response, timeoutMs, error)) {
        return false;
    }
    return responseSucceeded(response, error);
}

bool DapClient::responseSucceeded(const QJsonObject &response, QString *error) const
{
    if (response.value(QStringLiteral("success")).toBool()) {
        return true;
    }
    if (error) {
        *error = response.value(QStringLiteral("message")).toString(QStringLiteral("DAP request failed."));
    }
    return false;
}
