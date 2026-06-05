#include <QtTest/QtTest>

#include "LspClient.h"

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTextStream>

class TestLspClient : public QObject
{
    Q_OBJECT

private slots:
    void initializeHandshakeReadsServerMetadataAndSendsInitialized();
    void documentSyncNotificationsReachMockServerInOrder();
    void completionRequestParsesItemsAndStaysWithinBudget();
    void hoverRequestParsesMarkdownAndStaysWithinBudget();
};

namespace {
QString writeMockServer(QTemporaryDir &dir)
{
    const QString scriptPath = dir.filePath(QStringLiteral("mock_lsp_server.py"));
    QFile script(scriptPath);
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream out(&script);
    out.setEncoding(QStringConverter::Utf8);
    out << QStringLiteral(R"PY(
import json
import sys

log_path = sys.argv[1]

def read_message():
    headers = {}
    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            return None
        line = line.decode("ascii", errors="replace").strip()
        if not line:
            break
        key, _, value = line.partition(":")
        headers[key.lower()] = value.strip()
    length = int(headers.get("content-length", "0"))
    if length == 0:
        return None
    return json.loads(sys.stdin.buffer.read(length).decode("utf-8"))

def write_message(payload):
    body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    sys.stdout.buffer.write(f"Content-Length: {len(body)}\r\n\r\n".encode("ascii"))
    sys.stdout.buffer.write(body)
    sys.stdout.buffer.flush()

def log(payload):
    with open(log_path, "a", encoding="utf-8") as handle:
        handle.write(json.dumps(payload, ensure_ascii=False, separators=(",", ":")) + "\n")

running = True
while running:
    message = read_message()
    if message is None:
        break
    log(message)
    method = message.get("method")
    if method == "initialize":
        write_message({
            "jsonrpc": "2.0",
            "id": message["id"],
            "result": {
                "serverInfo": {"name": "lughat-althuban-lsp", "version": "0.4.0"},
                "capabilities": {
                    "textDocumentSync": {"openClose": True, "change": 1, "save": True},
                    "hoverProvider": True,
                    "completionProvider": {"resolveProvider": False, "triggerCharacters": []},
                },
            },
        })
    elif method == "shutdown":
        write_message({"jsonrpc": "2.0", "id": message["id"], "result": None})
    elif method == "textDocument/completion":
        write_message({
            "jsonrpc": "2.0",
            "id": message["id"],
            "result": {
                "isIncomplete": False,
                "items": [
                    {
                        "label": "اطبع",
                        "detail": "print(value)",
                        "documentation": {"kind": "markdown", "value": "**اطبع** -> `print`"},
                        "insertText": "اطبع",
                    },
                    {
                        "label": "اذا",
                        "detail": "conditional",
                    },
                ],
            },
        })
    elif method == "textDocument/hover":
        write_message({
            "jsonrpc": "2.0",
            "id": message["id"],
            "result": {"contents": {"kind": "markdown", "value": "**اطبع** -> `print(value)`"}},
        })
    elif method == "exit":
        running = False
)PY");
    return scriptPath;
}

LspServerCommand mockServerCommand(const QString &scriptPath, const QString &logPath)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    environment.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));

    LspServerCommand command;
    command.program = QStringLiteral("python");
    command.arguments = {QStringLiteral("-u"), scriptPath, logPath};
    command.workingDirectory = QFileInfo(scriptPath).absolutePath();
    command.environment = environment;
    return command;
}

QVector<QJsonObject> readLogMessages(const QString &logPath)
{
    QFile file(logPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QVector<QJsonObject> messages;
    while (!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QJsonDocument document = QJsonDocument::fromJson(line);
        if (document.isObject()) {
            messages.append(document.object());
        }
    }
    return messages;
}

bool waitForMethodCount(const QString &logPath, int expectedCount)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 3000) {
        if (readLogMessages(logPath).size() >= expectedCount) {
            return true;
        }
        QTest::qWait(25);
    }
    return false;
}
}

void TestLspClient::initializeHandshakeReadsServerMetadataAndSendsInitialized()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString logPath = dir.filePath(QStringLiteral("lsp-log.jsonl"));
    const QString scriptPath = writeMockServer(dir);
    QVERIFY(!scriptPath.isEmpty());

    LspClient client;
    client.setServerCommand(mockServerCommand(scriptPath, logPath));

    QString error;
    QVERIFY2(client.startAndInitialize(QStringLiteral("file:///workspace"), 5000, &error), qPrintable(error));
    const LspInitializeResult result = client.initializeResult();
    QCOMPARE(result.serverName, QStringLiteral("lughat-althuban-lsp"));
    QCOMPARE(result.serverVersion, QStringLiteral("0.4.0"));
    QVERIFY(result.textDocumentOpenClose);
    QCOMPARE(result.textDocumentChange, 1);
    QVERIFY(result.textDocumentSave);
    QVERIFY(result.hoverProvider);
    QVERIFY(result.completionProvider);

    QVERIFY(waitForMethodCount(logPath, 2));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.at(0).value(QStringLiteral("method")).toString(), QStringLiteral("initialize"));
    QCOMPARE(messages.at(1).value(QStringLiteral("method")).toString(), QStringLiteral("initialized"));
}

void TestLspClient::documentSyncNotificationsReachMockServerInOrder()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString logPath = dir.filePath(QStringLiteral("lsp-log.jsonl"));
    const QString scriptPath = writeMockServer(dir);
    QVERIFY(!scriptPath.isEmpty());

    LspClient client;
    client.setServerCommand(mockServerCommand(scriptPath, logPath));

    QString error;
    QVERIFY2(client.startAndInitialize(QStringLiteral("file:///workspace"), 5000, &error), qPrintable(error));

    const QString uri = QStringLiteral("file:///workspace/main.apy");
    client.openDocument(uri, QStringLiteral("apy"), 1, QString::fromUtf8("س = ١\n"));
    client.changeDocument(uri, 2, QString::fromUtf8("س = ٢\n"));
    client.saveDocument(uri, QString::fromUtf8("س = ٢\n"));
    client.closeDocument(uri);

    QVERIFY(waitForMethodCount(logPath, 6));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QStringList methods;
    for (const QJsonObject &message : messages) {
        methods.append(message.value(QStringLiteral("method")).toString());
    }

    QCOMPARE(methods.mid(0, 6), QStringList({
        QStringLiteral("initialize"),
        QStringLiteral("initialized"),
        QStringLiteral("textDocument/didOpen"),
        QStringLiteral("textDocument/didChange"),
        QStringLiteral("textDocument/didSave"),
        QStringLiteral("textDocument/didClose"),
    }));

    const QJsonObject didOpenTextDocument = messages.at(2)
        .value(QStringLiteral("params")).toObject()
        .value(QStringLiteral("textDocument")).toObject();
    QCOMPARE(didOpenTextDocument.value(QStringLiteral("uri")).toString(), uri);
    QCOMPARE(didOpenTextDocument.value(QStringLiteral("languageId")).toString(), QStringLiteral("apy"));
    QCOMPARE(didOpenTextDocument.value(QStringLiteral("version")).toInt(), 1);

    const QJsonObject didChangeParams = messages.at(3).value(QStringLiteral("params")).toObject();
    QCOMPARE(didChangeParams.value(QStringLiteral("textDocument")).toObject().value(QStringLiteral("version")).toInt(), 2);
    QCOMPARE(didChangeParams.value(QStringLiteral("contentChanges")).toArray().first().toObject().value(QStringLiteral("text")).toString(), QString::fromUtf8("س = ٢\n"));
}

void TestLspClient::completionRequestParsesItemsAndStaysWithinBudget()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString logPath = dir.filePath(QStringLiteral("lsp-log.jsonl"));
    const QString scriptPath = writeMockServer(dir);
    QVERIFY(!scriptPath.isEmpty());

    LspClient client;
    client.setServerCommand(mockServerCommand(scriptPath, logPath));

    QString error;
    QVERIFY2(client.startAndInitialize(QStringLiteral("file:///workspace"), 5000, &error), qPrintable(error));

    QElapsedTimer timer;
    timer.start();
    const QVector<LspCompletionItem> items = client.requestCompletion(QStringLiteral("file:///workspace/main.apy"), 1, 4, 5000, &error);
    QVERIFY2(timer.elapsed() <= 200, qPrintable(QStringLiteral("completion round-trip took %1 ms").arg(timer.elapsed())));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(items.size(), 2);
    QCOMPARE(items.at(0).label, QString::fromUtf8("اطبع"));
    QCOMPARE(items.at(0).detail, QStringLiteral("print(value)"));
    QCOMPARE(items.at(0).documentation, QStringLiteral("**اطبع** -> `print`"));
    QCOMPARE(items.at(0).insertText, QString::fromUtf8("اطبع"));
    QCOMPARE(items.at(1).label, QString::fromUtf8("اذا"));

    QVERIFY(waitForMethodCount(logPath, 3));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.at(2).value(QStringLiteral("method")).toString(), QStringLiteral("textDocument/completion"));
    const QJsonObject position = messages.at(2).value(QStringLiteral("params")).toObject().value(QStringLiteral("position")).toObject();
    QCOMPARE(position.value(QStringLiteral("line")).toInt(), 1);
    QCOMPARE(position.value(QStringLiteral("character")).toInt(), 4);
}

void TestLspClient::hoverRequestParsesMarkdownAndStaysWithinBudget()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString logPath = dir.filePath(QStringLiteral("lsp-log.jsonl"));
    const QString scriptPath = writeMockServer(dir);
    QVERIFY(!scriptPath.isEmpty());

    LspClient client;
    client.setServerCommand(mockServerCommand(scriptPath, logPath));

    QString error;
    QVERIFY2(client.startAndInitialize(QStringLiteral("file:///workspace"), 5000, &error), qPrintable(error));

    QElapsedTimer timer;
    timer.start();
    const LspHoverResult hover = client.requestHover(QStringLiteral("file:///workspace/main.apy"), 2, 8, 5000, &error);
    QVERIFY2(timer.elapsed() <= 100, qPrintable(QStringLiteral("hover round-trip took %1 ms").arg(timer.elapsed())));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(hover.hasContent);
    QCOMPARE(hover.markdown, QStringLiteral("**اطبع** -> `print(value)`"));

    QVERIFY(waitForMethodCount(logPath, 3));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.at(2).value(QStringLiteral("method")).toString(), QStringLiteral("textDocument/hover"));
    const QJsonObject position = messages.at(2).value(QStringLiteral("params")).toObject().value(QStringLiteral("position")).toObject();
    QCOMPARE(position.value(QStringLiteral("line")).toInt(), 2);
    QCOMPARE(position.value(QStringLiteral("character")).toInt(), 8);
}

QTEST_MAIN(TestLspClient)
#include "TestLspClient.moc"
