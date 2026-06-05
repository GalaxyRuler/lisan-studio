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

QJsonObject positionObject(int line, int character)
{
    QJsonObject object;
    object.insert(QStringLiteral("line"), line);
    object.insert(QStringLiteral("character"), character);
    return object;
}

QJsonObject textDocumentPositionParams(const QString &uri, int line, int character)
{
    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), textDocumentIdentifier(uri));
    params.insert(QStringLiteral("position"), positionObject(line, character));
    return params;
}

QString documentationText(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isObject()) {
        return value.toObject().value(QStringLiteral("value")).toString();
    }
    return {};
}

QString hoverContentsText(const QJsonValue &value)
{
    if (value.isString() || value.isObject()) {
        return documentationText(value);
    }
    if (!value.isArray()) {
        return {};
    }

    QStringList parts;
    const QJsonArray array = value.toArray();
    for (const QJsonValue &item : array) {
        const QString part = hoverContentsText(item).trimmed();
        if (!part.isEmpty()) {
            parts.append(part);
        }
    }
    return parts.join(QStringLiteral("\n\n"));
}

LspLocation locationFromObject(const QJsonObject &object)
{
    LspLocation location;
    location.uri = object.value(QStringLiteral("uri")).toString();
    QJsonObject range = object.value(QStringLiteral("range")).toObject();
    if (location.uri.isEmpty()) {
        location.uri = object.value(QStringLiteral("targetUri")).toString();
        range = object.value(QStringLiteral("targetSelectionRange")).toObject();
        if (range.isEmpty()) {
            range = object.value(QStringLiteral("targetRange")).toObject();
        }
    }
    const QJsonObject start = range.value(QStringLiteral("start")).toObject();
    location.line = start.value(QStringLiteral("line")).toInt();
    location.character = start.value(QStringLiteral("character")).toInt();
    return location;
}

QVector<LspLocation> locationsFromValue(const QJsonValue &value)
{
    QVector<LspLocation> locations;
    if (value.isObject()) {
        const LspLocation location = locationFromObject(value.toObject());
        if (!location.uri.isEmpty()) {
            locations.append(location);
        }
        return locations;
    }

    const QJsonArray array = value.toArray();
    locations.reserve(array.size());
    for (const QJsonValue &item : array) {
        if (!item.isObject()) {
            continue;
        }
        const LspLocation location = locationFromObject(item.toObject());
        if (!location.uri.isEmpty()) {
            locations.append(location);
        }
    }
    return locations;
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

QVector<LspCompletionItem> LspClient::requestCompletion(const QString &uri, int line, int character, int timeoutMs, QString *error)
{
    if (error) {
        error->clear();
    }
    if (process.state() == QProcess::NotRunning) {
        if (error) {
            *error = QStringLiteral("LSP server is not running.");
        }
        return {};
    }

    const int requestId = sendRequest(QStringLiteral("textDocument/completion"), textDocumentPositionParams(uri, line, character));
    QJsonObject response;
    if (!waitForResponse(requestId, &response, timeoutMs, error)) {
        return {};
    }
    if (response.contains(QStringLiteral("error"))) {
        if (error) {
            *error = response.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        }
        return {};
    }

    QJsonArray rawItems;
    const QJsonValue result = response.value(QStringLiteral("result"));
    if (result.isArray()) {
        rawItems = result.toArray();
    } else if (result.isObject()) {
        rawItems = result.toObject().value(QStringLiteral("items")).toArray();
    }

    QVector<LspCompletionItem> items;
    items.reserve(rawItems.size());
    for (const QJsonValue &value : rawItems) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        LspCompletionItem item;
        item.label = object.value(QStringLiteral("label")).toString();
        item.detail = object.value(QStringLiteral("detail")).toString();
        item.documentation = documentationText(object.value(QStringLiteral("documentation")));
        item.insertText = object.value(QStringLiteral("insertText")).toString(item.label);
        if (!item.label.isEmpty()) {
            items.append(item);
        }
    }
    return items;
}

LspHoverResult LspClient::requestHover(const QString &uri, int line, int character, int timeoutMs, QString *error)
{
    if (error) {
        error->clear();
    }
    LspHoverResult hover;
    if (process.state() == QProcess::NotRunning) {
        if (error) {
            *error = QStringLiteral("LSP server is not running.");
        }
        return hover;
    }

    const int requestId = sendRequest(QStringLiteral("textDocument/hover"), textDocumentPositionParams(uri, line, character));
    QJsonObject response;
    if (!waitForResponse(requestId, &response, timeoutMs, error)) {
        return hover;
    }
    if (response.contains(QStringLiteral("error"))) {
        if (error) {
            *error = response.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        }
        return hover;
    }

    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    hover.markdown = hoverContentsText(result.value(QStringLiteral("contents")));
    hover.hasContent = !hover.markdown.trimmed().isEmpty();
    return hover;
}

QVector<LspLocation> LspClient::requestDefinition(const QString &uri, int line, int character, int timeoutMs, QString *error)
{
    if (error) {
        error->clear();
    }
    if (process.state() == QProcess::NotRunning) {
        if (error) {
            *error = QStringLiteral("LSP server is not running.");
        }
        return {};
    }

    const int requestId = sendRequest(QStringLiteral("textDocument/definition"), textDocumentPositionParams(uri, line, character));
    QJsonObject response;
    if (!waitForResponse(requestId, &response, timeoutMs, error)) {
        return {};
    }
    if (response.contains(QStringLiteral("error"))) {
        if (error) {
            *error = response.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        }
        return {};
    }
    return locationsFromValue(response.value(QStringLiteral("result")));
}

QVector<LspLocation> LspClient::requestReferences(const QString &uri, int line, int character, bool includeDeclaration, int timeoutMs, QString *error)
{
    if (error) {
        error->clear();
    }
    if (process.state() == QProcess::NotRunning) {
        if (error) {
            *error = QStringLiteral("LSP server is not running.");
        }
        return {};
    }

    QJsonObject params = textDocumentPositionParams(uri, line, character);
    QJsonObject context;
    context.insert(QStringLiteral("includeDeclaration"), includeDeclaration);
    params.insert(QStringLiteral("context"), context);

    const int requestId = sendRequest(QStringLiteral("textDocument/references"), params);
    QJsonObject response;
    if (!waitForResponse(requestId, &response, timeoutMs, error)) {
        return {};
    }
    if (response.contains(QStringLiteral("error"))) {
        if (error) {
            *error = response.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        }
        return {};
    }
    return locationsFromValue(response.value(QStringLiteral("result")));
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
