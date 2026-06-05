#include <QtTest/QtTest>

#include "DapClient.h"

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
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
    void beginLaunchHandlesInitializedBeforeLaunchResponse();
    void moduleLaunchSerializesModuleInsteadOfProgram();
    void setBreakpointsRequestSerializesSourceLines();
    void runControlRequestsRoundTripAndStoppedEventsReachSignals();
    void inspectionRequestsParseStackScopesVariablesAndWatchEvaluation();
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

configuration_done_seen = False
pending_launch = None
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
    elif command == "setBreakpoints":
        response(message, {
            "breakpoints": [
                {"id": 1, "verified": True, "line": bp.get("line", 0)}
                for bp in message.get("arguments", {}).get("breakpoints", [])
            ]
        })
    elif command == "configurationDone":
        configuration_done_seen = True
        response(message)
        if pending_launch is not None:
            response(pending_launch)
            write_message({
                "seq": seq,
                "type": "event",
                "event": "stopped",
                "body": {"reason": "breakpoint", "threadId": 7},
            })
            seq += 1
            pending_launch = None
    elif command == "launch":
        write_message({
            "seq": seq,
            "type": "event",
            "event": "initialized",
        })
        seq += 1
        if configuration_done_seen:
            response(message)
            write_message({
                "seq": seq,
                "type": "event",
                "event": "stopped",
                "body": {"reason": "breakpoint", "threadId": 7},
            })
            seq += 1
        else:
            pending_launch = message
    elif command == "continue":
        response(message)
        write_message({
            "seq": seq,
            "type": "event",
            "event": "continued",
            "body": {"threadId": message.get("arguments", {}).get("threadId", 0)},
        })
        seq += 1
    elif command == "next":
        response(message)
    elif command == "stepIn":
        response(message)
    elif command == "stepOut":
        response(message)
    elif command == "stackTrace":
        response(message, {
            "stackFrames": [
                {
                    "id": 11,
                    "name": "الرئيسية",
                    "source": {"path": "C:/project/main.apy"},
                    "line": 4,
                    "column": 1,
                }
            ],
            "totalFrames": 1,
        })
    elif command == "scopes":
        response(message, {
            "scopes": [
                {"name": "Locals", "variablesReference": 31, "expensive": False},
                {"name": "Globals", "variablesReference": 32, "expensive": True},
            ]
        })
    elif command == "variables":
        response(message, {
            "variables": [
                {"name": "عدد", "value": "42", "type": "int", "variablesReference": 0},
                {"name": "رسالة", "value": "مرحبا", "type": "str", "variablesReference": 0},
            ]
        })
    elif command == "evaluate":
        response(message, {
            "result": "84",
            "type": "int",
            "variablesReference": 0,
        })
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
    QVERIFY2(client.configurationDone(5000, &error), qPrintable(error));

    DapLaunchRequest launch;
    launch.program = QStringLiteral("C:/project/main.apy");
    launch.arguments = {QStringLiteral("--seed"), QStringLiteral("42")};
    launch.workingDirectory = QStringLiteral("C:/project");
    launch.stopOnEntry = true;
    QVERIFY2(client.launch(launch, 5000, &error), qPrintable(error));

    QVERIFY(waitForMethodCount(logPath, 3));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.at(2).value(QStringLiteral("command")).toString(), QStringLiteral("launch"));
    const QJsonObject arguments = messages.at(2).value(QStringLiteral("arguments")).toObject();
    QCOMPARE(arguments.value(QStringLiteral("program")).toString(), QStringLiteral("C:/project/main.apy"));
    QCOMPARE(arguments.value(QStringLiteral("cwd")).toString(), QStringLiteral("C:/project"));
    QVERIFY(arguments.value(QStringLiteral("stopOnEntry")).toBool());
    QCOMPARE(arguments.value(QStringLiteral("args")).toArray().size(), 2);
}

void TestDapClient::beginLaunchHandlesInitializedBeforeLaunchResponse()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString logPath = dir.filePath(QStringLiteral("dap-log.jsonl"));
    const QString scriptPath = writeMockServer(dir);
    QVERIFY(!scriptPath.isEmpty());

    DapClient client;
    client.setServerCommand(mockServerCommand(scriptPath, logPath));
    QSignalSpy initializedSpy(&client, &DapClient::initialized);

    QString error;
    QVERIFY2(client.startAndInitialize(5000, &error), qPrintable(error));

    DapLaunchRequest launch;
    launch.program = QStringLiteral("C:/project/main.apy");
    launch.workingDirectory = QStringLiteral("C:/project");
    const int launchSequence = client.beginLaunch(launch, &error);
    QVERIFY2(launchSequence > 0, qPrintable(error));
    QVERIFY2(client.waitForInitialized(5000, &error), qPrintable(error));
    QCOMPARE(initializedSpy.count(), 1);
    QVERIFY2(client.setBreakpoints(QStringLiteral("C:/project/main.apy"), {2}, 5000, &error), qPrintable(error));
    QVERIFY2(client.configurationDone(5000, &error), qPrintable(error));
    QVERIFY2(client.waitForRequest(launchSequence, 5000, &error), qPrintable(error));

    QVERIFY(waitForMethodCount(logPath, 4));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.at(1).value(QStringLiteral("command")).toString(), QStringLiteral("launch"));
    QCOMPARE(messages.at(2).value(QStringLiteral("command")).toString(), QStringLiteral("setBreakpoints"));
    QCOMPARE(messages.at(3).value(QStringLiteral("command")).toString(), QStringLiteral("configurationDone"));
}

void TestDapClient::moduleLaunchSerializesModuleInsteadOfProgram()
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
    QVERIFY2(client.configurationDone(5000, &error), qPrintable(error));

    DapLaunchRequest launch;
    launch.program = QStringLiteral("C:/project/main.apy");
    launch.module = QStringLiteral("arabicpython.cli");
    launch.arguments = {QStringLiteral("C:/project/main.apy")};
    launch.workingDirectory = QStringLiteral("C:/project");
    QVERIFY2(client.launch(launch, 5000, &error), qPrintable(error));

    QVERIFY(waitForMethodCount(logPath, 3));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    const QJsonObject arguments = messages.at(2).value(QStringLiteral("arguments")).toObject();
    QCOMPARE(arguments.value(QStringLiteral("module")).toString(), QStringLiteral("arabicpython.cli"));
    QVERIFY(!arguments.contains(QStringLiteral("program")));
    QCOMPARE(arguments.value(QStringLiteral("args")).toArray().first().toString(), QStringLiteral("C:/project/main.apy"));
}

void TestDapClient::setBreakpointsRequestSerializesSourceLines()
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
    QVERIFY2(client.setBreakpoints(QStringLiteral("C:/project/main.apy"), {3, 8}, 5000, &error), qPrintable(error));

    QVERIFY(waitForMethodCount(logPath, 2));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.at(1).value(QStringLiteral("command")).toString(), QStringLiteral("setBreakpoints"));
    const QJsonObject arguments = messages.at(1).value(QStringLiteral("arguments")).toObject();
    QCOMPARE(arguments.value(QStringLiteral("source")).toObject().value(QStringLiteral("path")).toString(), QStringLiteral("C:/project/main.apy"));
    const QJsonArray breakpoints = arguments.value(QStringLiteral("breakpoints")).toArray();
    QCOMPARE(breakpoints.size(), 2);
    QCOMPARE(breakpoints.at(0).toObject().value(QStringLiteral("line")).toInt(), 3);
    QCOMPARE(breakpoints.at(1).toObject().value(QStringLiteral("line")).toInt(), 8);
}

void TestDapClient::runControlRequestsRoundTripAndStoppedEventsReachSignals()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString logPath = dir.filePath(QStringLiteral("dap-log.jsonl"));
    const QString scriptPath = writeMockServer(dir);
    QVERIFY(!scriptPath.isEmpty());

    DapClient client;
    client.setServerCommand(mockServerCommand(scriptPath, logPath));
    QSignalSpy stoppedSpy(&client, &DapClient::stopped);
    QSignalSpy continuedSpy(&client, &DapClient::continued);

    QString error;
    QVERIFY2(client.startAndInitialize(5000, &error), qPrintable(error));
    QVERIFY2(client.configurationDone(5000, &error), qPrintable(error));

    DapLaunchRequest launch;
    launch.program = QStringLiteral("C:/project/main.apy");
    launch.workingDirectory = QStringLiteral("C:/project");
    QVERIFY2(client.launch(launch, 5000, &error), qPrintable(error));
    QTRY_VERIFY(!stoppedSpy.isEmpty());
    QCOMPARE(stoppedSpy.first().at(0).toString(), QStringLiteral("breakpoint"));
    QCOMPARE(stoppedSpy.first().at(1).toInt(), 7);

    QVERIFY2(client.continueExecution(7, 5000, &error), qPrintable(error));
    QTRY_VERIFY(!continuedSpy.isEmpty());
    QCOMPARE(continuedSpy.first().at(0).toInt(), 7);
    QVERIFY2(client.stepOver(7, 5000, &error), qPrintable(error));
    QVERIFY2(client.stepInto(7, 5000, &error), qPrintable(error));
    QVERIFY2(client.stepOut(7, 5000, &error), qPrintable(error));

    QVERIFY(waitForMethodCount(logPath, 7));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.at(1).value(QStringLiteral("command")).toString(), QStringLiteral("configurationDone"));
    QCOMPARE(messages.at(3).value(QStringLiteral("command")).toString(), QStringLiteral("continue"));
    QCOMPARE(messages.at(4).value(QStringLiteral("command")).toString(), QStringLiteral("next"));
    QCOMPARE(messages.at(5).value(QStringLiteral("command")).toString(), QStringLiteral("stepIn"));
    QCOMPARE(messages.at(6).value(QStringLiteral("command")).toString(), QStringLiteral("stepOut"));
}

void TestDapClient::inspectionRequestsParseStackScopesVariablesAndWatchEvaluation()
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

    const QVector<DapStackFrame> frames = client.stackTrace(7, 5000, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(frames.size(), 1);
    QCOMPARE(frames.first().id, 11);
    QCOMPARE(frames.first().name, QString::fromUtf8("الرئيسية"));
    QCOMPARE(frames.first().sourcePath, QStringLiteral("C:/project/main.apy"));
    QCOMPARE(frames.first().line, 4);

    const QVector<DapScope> scopes = client.scopes(frames.first().id, 5000, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(scopes.size(), 2);
    QCOMPARE(scopes.first().name, QStringLiteral("Locals"));
    QCOMPARE(scopes.first().variablesReference, 31);
    QVERIFY(scopes.at(1).expensive);

    const QVector<DapVariable> variables = client.variables(scopes.first().variablesReference, 5000, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(variables.size(), 2);
    QCOMPARE(variables.first().name, QString::fromUtf8("عدد"));
    QCOMPARE(variables.first().value, QStringLiteral("42"));
    QCOMPARE(variables.at(1).type, QStringLiteral("str"));

    const DapVariable watched = client.evaluate(QString::fromUtf8("عدد * 2"), frames.first().id, QStringLiteral("watch"), 5000, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(watched.name, QString::fromUtf8("عدد * 2"));
    QCOMPARE(watched.value, QStringLiteral("84"));
    QCOMPARE(watched.type, QStringLiteral("int"));

    QVERIFY(waitForMethodCount(logPath, 5));
    const QVector<QJsonObject> messages = readLogMessages(logPath);
    QCOMPARE(messages.at(1).value(QStringLiteral("command")).toString(), QStringLiteral("stackTrace"));
    QCOMPARE(messages.at(1).value(QStringLiteral("arguments")).toObject().value(QStringLiteral("threadId")).toInt(), 7);
    QCOMPARE(messages.at(2).value(QStringLiteral("command")).toString(), QStringLiteral("scopes"));
    QCOMPARE(messages.at(2).value(QStringLiteral("arguments")).toObject().value(QStringLiteral("frameId")).toInt(), 11);
    QCOMPARE(messages.at(3).value(QStringLiteral("command")).toString(), QStringLiteral("variables"));
    QCOMPARE(messages.at(3).value(QStringLiteral("arguments")).toObject().value(QStringLiteral("variablesReference")).toInt(), 31);
    QCOMPARE(messages.at(4).value(QStringLiteral("command")).toString(), QStringLiteral("evaluate"));
    QCOMPARE(messages.at(4).value(QStringLiteral("arguments")).toObject().value(QStringLiteral("expression")).toString(), QString::fromUtf8("عدد * 2"));
}

QTEST_MAIN(TestDapClient)
#include "TestDapClient.moc"
