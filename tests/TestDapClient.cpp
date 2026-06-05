#include <QtTest/QtTest>

#include "DapClient.h"

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTextStream>

class TestDapClient : public QObject
{
    Q_OBJECT

private slots:
    void initializeHandshakeReadsCapabilities();
    void launchRequestSerializesProgramArgumentsAndWorkingDirectory();
};

namespace {
QString writeMockServer(QTemporaryDir &dir)
{
    const QString scriptPath = dir.filePath(QStringLiteral("mock_dap_server.py"));
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
seq = 1

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

def response(request, body=None):
    global seq
    payload = {
        "seq": seq,
        "type": "response",
        "request_seq": request["seq"],
        "success": True,
        "command": request["command"],
    }
    seq += 1
    if body is not None:
        payload["body"] = body
    write_message(payload)

running = True
while running:
    message = read_message()
    if message is None:
        break
    log(message)
    command = message.get("command")
    if command == "initialize":
        response(message, {
            "supportsConfigurationDoneRequest": True,
            "supportsRestartRequest": False,
        })
    elif command == "launch":
        response(message)
    elif command == "disconnect":
        response(message)
        running = False
)PY");
    return scriptPath;
}

DapServerCommand mockServerCommand(const QString &scriptPath, const QString &logPath)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    environment.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));

    DapServerCommand command;
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

void TestDapClient::initializeHandshakeReadsCapabilities()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString logPath = dir.filePath(QStringLiteral("dap-log.jsonl"));
    const QString scriptPath = writeMockServer(dir);
    QVERIFY(!scriptPath.isEmpty());

    DapClient client;
    client.setServerCommand(mockServerCommand(scriptPath, logPath));

    QString error;
    QVERIFY2(client.startAndInitialize(5000, &error), qPrintable(error));
    const DapInitializeResult result = client.initializeResult();
    QVERIFY(result.success);
    QCOMPARE(result.adapterId, QStringLiteral("debugpy"));
    QVERIFY(result.supportsConfigurationDoneRequest);
    QVERIFY(!result.supportsRestartRequest);

    QVERIFY(waitForMethodCount(logPath, 1));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.first().value(QStringLiteral("command")).toString(), QStringLiteral("initialize"));
    const QJsonObject arguments = messages.first().value(QStringLiteral("arguments")).toObject();
    QCOMPARE(arguments.value(QStringLiteral("adapterID")).toString(), QStringLiteral("debugpy"));
    QVERIFY(arguments.value(QStringLiteral("linesStartAt1")).toBool());
    QVERIFY(arguments.value(QStringLiteral("columnsStartAt1")).toBool());
}

void TestDapClient::launchRequestSerializesProgramArgumentsAndWorkingDirectory()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString logPath = dir.filePath(QStringLiteral("dap-log.jsonl"));
    const QString scriptPath = writeMockServer(dir);
    QVERIFY(!scriptPath.isEmpty());

    DapClient client;
    client.setServerCommand(mockServerCommand(scriptPath, logPath));

    QString error;
    QVERIFY2(client.startAndInitialize(5000, &error), qPrintable(error));

    DapLaunchRequest launch;
    launch.program = QStringLiteral("C:/project/main.apy");
    launch.arguments = {QStringLiteral("--seed"), QStringLiteral("42")};
    launch.workingDirectory = QStringLiteral("C:/project");
    launch.stopOnEntry = true;
    QVERIFY2(client.launch(launch, 5000, &error), qPrintable(error));

    QVERIFY(waitForMethodCount(logPath, 2));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.at(1).value(QStringLiteral("command")).toString(), QStringLiteral("launch"));
    const QJsonObject arguments = messages.at(1).value(QStringLiteral("arguments")).toObject();
    QCOMPARE(arguments.value(QStringLiteral("program")).toString(), QStringLiteral("C:/project/main.apy"));
    QCOMPARE(arguments.value(QStringLiteral("cwd")).toString(), QStringLiteral("C:/project"));
    QVERIFY(arguments.value(QStringLiteral("stopOnEntry")).toBool());
    QCOMPARE(arguments.value(QStringLiteral("args")).toArray().size(), 2);
}

QTEST_MAIN(TestDapClient)
#include "TestDapClient.moc"
