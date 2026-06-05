#include <QtTest/QtTest>

#include "TerminalBackend.h"

#include <QSignalSpy>
#include <QTemporaryDir>

class TestTerminalBackend : public QObject
{
    Q_OBJECT

private slots:
    void cmdEchoProducesOutput();
    void multiLineInputProducesOutputAndExitSignal();
};

namespace {
TerminalCommand cmdCommand(const QString &workingDirectory)
{
    TerminalCommand command;
    command.program = QStringLiteral("cmd.exe");
    command.workingDirectory = workingDirectory;
    return command;
}
}

void TestTerminalBackend::cmdEchoProducesOutput()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    TerminalBackend backend;
    QString output;
    connect(&backend, &TerminalBackend::outputReceived, this, [&output](const QString &text) {
        output += text;
    });

    QString error;
    QVERIFY2(backend.start(cmdCommand(dir.path()), &error), qPrintable(error));
    QVERIFY(backend.isRunning());
    QTRY_VERIFY2(output.contains(QStringLiteral(">")), qPrintable(output));
    QVERIFY2(backend.writeInput(QStringLiteral("echo hello\r\n"), &error), qPrintable(error));
    QTRY_VERIFY2(output.contains(QStringLiteral("hello")), qPrintable(output));
    QVERIFY2(backend.writeInput(QStringLiteral("exit\r\n"), &error), qPrintable(error));
    backend.close();
}

void TestTerminalBackend::multiLineInputProducesOutputAndExitSignal()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    TerminalBackend backend;
    QString output;
    connect(&backend, &TerminalBackend::outputReceived, this, [&output](const QString &text) {
        output += text;
    });
    QSignalSpy exitedSpy(&backend, &TerminalBackend::processExited);

    QString error;
    QVERIFY2(backend.start(cmdCommand(dir.path()), &error), qPrintable(error));
    QTRY_VERIFY2(output.contains(QStringLiteral(">")), qPrintable(output));
    QVERIFY2(backend.writeInput(QStringLiteral("echo alpha\r\n"), &error), qPrintable(error));

    QTRY_VERIFY2(output.contains(QStringLiteral("alpha")), qPrintable(output));
    QVERIFY2(backend.writeInput(QStringLiteral("echo beta\r\n"), &error), qPrintable(error));
    QTRY_VERIFY2(output.contains(QStringLiteral("beta")), qPrintable(output));
    QVERIFY2(backend.writeInput(QStringLiteral("exit 7\r\n"), &error), qPrintable(error));
    QTRY_VERIFY(!exitedSpy.isEmpty());
    QCOMPARE(exitedSpy.first().first().toInt(), 7);
    QVERIFY(!backend.isRunning());
}

QTEST_MAIN(TestTerminalBackend)
#include "TestTerminalBackend.moc"
