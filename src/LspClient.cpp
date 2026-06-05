#include "LspClient.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

namespace {
QJsonObject textDocumentIdentifier(const QString &uri)
{
    QJsonObject object;
    object.insert(QStringLiteral("uri"), uri);
    return object;
}

QJsonObject versionedTextDocumentIdentifier(const QString &uri, int version)
{
    QJsonObject object = textDocumentIdentifier(uri);
    object.insert(QStringLiteral("version"), version);
    return object;
}

bool parseBooleanProvider(const QJsonValue &value)
{
    if (value.isBool()) {
        return value.toBool();
    }
    return value.isObject();
}
}

LspClient::LspClient(QObject *parent)
    : QObject(parent)
{
    connect(&process, &QProcess::readyReadStandardOutput, this, [this]() {
        readAvailableMessages();
    });
}

LspClient::~LspClient()
{
    shutdown();
}

void LspClient::setServerCommand(const LspServerCommand &serverCommand)
{
    command = serverCommand;
}

LspServerCommand LspClient::serverCommand() const
{
    return command;
}

bool LspClient::startAndInitialize(const QString &rootUri, int timeoutMs, QString *error)
{
    if (command.program.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("LSP server program is empty.");
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

    QJsonObject clientInfo;
    clientInfo.insert(QStringLiteral("name"), QStringLiteral("Lisan Studio"));

    QJsonObject params;
    params.insert(QStringLiteral("processId"), QJsonValue());
    params.insert(QStringLiteral("rootUri"), rootUri);
    params.insert(QStringLiteral("clientInfo"), clientInfo);
    params.insert(QStringLiteral("capabilities"), QJsonObject{});

    const int initializeRequestId = sendRequest(QStringLiteral("initialize"), params);
    QJsonObject response;
    if (!waitForResponse(initializeRequestId, &response, timeoutMs, error)) {
        return false;
    }

    if (response.contains(QStringLiteral("error"))) {
        if (error) {
            *error = response.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        }
        return false;
    }

    applyInitializeResult(response.value(QStringLiteral("result")).toObject());
    sendNotification(QStringLiteral("initialized"), QJsonObject{});
    return true;
}

bool LspClient::isRunning() const
{
    return process.state() != QProcess::NotRunning;
}

LspInitializeResult LspClient::initializeResult() const
{
    return lastInitializeResult;
}

void LspClient::openDocument(const QString &uri, const QString &languageId, int version, const QString &text)
{
    QJsonObject textDocument;
    textDocument.insert(QStringLiteral("uri"), uri);
    textDocument.insert(QStringLiteral("languageId"), languageId);
    textDocument.insert(QStringLiteral("version"), version);
    textDocument.insert(QStringLiteral("text"), text);

    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), textDocument);
    sendNotification(QStringLiteral("textDocument/didOpen"), params);
}

void LspClient::changeDocument(const QString &uri, int version, const QString &text)
{
    QJsonObject contentChange;
    contentChange.insert(QStringLiteral("text"), text);

    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), versionedTextDocumentIdentifier(uri, version));
    params.insert(QStringLiteral("contentChanges"), QJsonArray{contentChange});
    sendNotification(QStringLiteral("textDocument/didChange"), params);
}

void LspClient::saveDocument(const QString &uri, const QString &text)
{
    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), textDocumentIdentifier(uri));
    if (!text.isNull()) {
        params.insert(QStringLiteral("text"), text);
    }
    sendNotification(QStringLiteral("textDocument/didSave"), params);
}

void LspClient::closeDocument(const QString &uri)
{
    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), textDocumentIdentifier(uri));
    sendNotification(QStringLiteral("textDocument/didClose"), params);
}

void LspClient::shutdown(int timeoutMs)
{
    if (process.state() == QProcess::NotRunning) {
        return;
    }

    const int requestId = sendRequest(QStringLiteral("shutdown"), QJsonObject{});
    QJsonObject response;
    QString ignoredError;
    waitForResponse(requestId, &response, timeoutMs, &ignoredError);
    sendNotification(QStringLiteral("exit"));
    process.waitForFinished(timeoutMs);
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(1000);
    }
}

int LspClient::sendRequest(const QString &method, const QJsonObject &params)
{
    const int requestId = nextRequestId++;
    QJsonObject payload;
    payload.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    payload.insert(QStringLiteral("id"), requestId);
    payload.insert(QStringLiteral("method"), method);
    payload.insert(QStringLiteral("params"), params);
    writePayload(payload);
    return requestId;
}

void LspClient::sendNotification(const QString &method, const QJsonObject &params)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    payload.insert(QStringLiteral("method"), method);
    if (!params.isEmpty()) {
        payload.insert(QStringLiteral("params"), params);
    }
    writePayload(payload);
}

void LspClient::writePayload(const QJsonObject &payload)
{
    if (process.state() == QProcess::NotRunning) {
        return;
    }

    const QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    const QByteArray header = QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size()) + QByteArrayLiteral("\r\n\r\n");
    process.write(header);
    process.write(body);
    process.waitForBytesWritten(1000);
}

bool LspClient::waitForResponse(int requestId, QJsonObject *response, int timeoutMs, QString *error)
{
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        readAvailableMessages();
        const auto it = responses.find(requestId);
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
        *error = QStringLiteral("Timed out waiting for LSP response.");
    }
    return false;
}

void LspClient::readAvailableMessages()
{
    incomingBuffer.append(process.readAllStandardOutput());
    parseBufferedMessages();
}

void LspClient::parseBufferedMessages()
{
    while (true) {
        const int headerEnd = incomingBuffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }

        const QByteArray header = incomingBuffer.left(headerEnd);
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
            incomingBuffer.remove(0, headerEnd + 4);
            continue;
        }

        const int messageStart = headerEnd + 4;
        if (incomingBuffer.size() < messageStart + contentLength) {
            return;
        }

        const QByteArray body = incomingBuffer.mid(messageStart, contentLength);
        incomingBuffer.remove(0, messageStart + contentLength);

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            continue;
        }

        const QJsonObject object = document.object();
        if (object.contains(QStringLiteral("id"))) {
            responses.insert(object.value(QStringLiteral("id")).toInt(), object);
        }
    }
}

void LspClient::applyInitializeResult(const QJsonObject &result)
{
    LspInitializeResult parsed;
    const QJsonObject serverInfo = result.value(QStringLiteral("serverInfo")).toObject();
    parsed.serverName = serverInfo.value(QStringLiteral("name")).toString();
    parsed.serverVersion = serverInfo.value(QStringLiteral("version")).toString();

    const QJsonObject capabilities = result.value(QStringLiteral("capabilities")).toObject();
    const QJsonObject textDocumentSync = capabilities.value(QStringLiteral("textDocumentSync")).toObject();
    parsed.textDocumentOpenClose = textDocumentSync.value(QStringLiteral("openClose")).toBool();
    parsed.textDocumentChange = textDocumentSync.value(QStringLiteral("change")).toInt();
    parsed.textDocumentSave = parseBooleanProvider(textDocumentSync.value(QStringLiteral("save")));
    parsed.hoverProvider = parseBooleanProvider(capabilities.value(QStringLiteral("hoverProvider")));
    parsed.completionProvider = parseBooleanProvider(capabilities.value(QStringLiteral("completionProvider")));

    lastInitializeResult = parsed;
}
