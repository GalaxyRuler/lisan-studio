#include <QtTest/QtTest>

#include "RuntimeOrchestrator.h"

class TestRuntimeOrchestrator : public QObject
{
    Q_OBJECT

private slots:
    void runtimeOrchestratorCancelEscalatesFromTerminateToKill();
    void runtimeOrchestratorRecordsHistoryForCompletedRun();
    void runtimeOrchestratorRefusesConcurrentStart();
};

static RuntimeLaunchPlan planForCommand(const QString &title, const QStringList &arguments)
{
    RuntimeLaunchPlan plan;
    plan.action = RuntimeAction::Run;
    plan.title = title;
    plan.filePath = QStringLiteral("fixture.apy");
    plan.command.program = QStringLiteral("cmd.exe");
    plan.command.arguments = arguments;
    plan.command.workingDirectory = QDir::tempPath();
    plan.initialOutput = QStringLiteral("[%1]\n%2\n").arg(title, QStringLiteral("جار التنفيذ..."));
    plan.runningStatus = QStringLiteral("%1...").arg(title);
    return plan;
}

static RuntimeLaunchPlan quickSuccessPlan()
{
    return planForCommand(QStringLiteral("quick success"), {QStringLiteral("/C"), QStringLiteral("exit /B 0")});
}

static RuntimeLaunchPlan longRunningPlan()
{
    return planForCommand(QStringLiteral("long running"), {QStringLiteral("/C"), QStringLiteral("ping -n 10 127.0.0.1 > nul")});
}

void TestRuntimeOrchestrator::runtimeOrchestratorCancelEscalatesFromTerminateToKill()
{
    RuntimeOrchestrator orchestrator;
    orchestrator.setCancelEscalationGraceMs(0);
    QSignalSpy runningSpy(&orchestrator, &RuntimeOrchestrator::runningChanged);

    QVERIFY(orchestrator.start(longRunningPlan()));
    QTRY_VERIFY_WITH_TIMEOUT(orchestrator.isRunning(), 1000);

    orchestrator.cancel();

    QTRY_VERIFY_WITH_TIMEOUT(!orchestrator.isRunning(), 3000);
    QVERIFY(runningSpy.count() >= 2);
    QCOMPARE(runningSpy.last().at(0).toBool(), false);
}

void TestRuntimeOrchestrator::runtimeOrchestratorRecordsHistoryForCompletedRun()
{
    RuntimeOrchestrator orchestrator;
    QSignalSpy runningSpy(&orchestrator, &RuntimeOrchestrator::runningChanged);

    QVERIFY(orchestrator.start(quickSuccessPlan()));
    QTRY_VERIFY_WITH_TIMEOUT(!orchestrator.isRunning() && runningSpy.count() >= 2, 3000);

    const QVector<RuntimeHistoryEntry> entries = orchestrator.historyEntries();
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().title, QStringLiteral("quick success"));
    QCOMPARE(orchestrator.lastCompletedPlan().title, QStringLiteral("quick success"));
}

void TestRuntimeOrchestrator::runtimeOrchestratorRefusesConcurrentStart()
{
    RuntimeOrchestrator orchestrator;

    QVERIFY(orchestrator.start(longRunningPlan()));
    QTRY_VERIFY_WITH_TIMEOUT(orchestrator.isRunning(), 1000);
    QVERIFY(!orchestrator.start(quickSuccessPlan()));
    QCOMPARE(orchestrator.historyEntries().size(), 1);
    QCOMPARE(orchestrator.historyEntries().first().title, QStringLiteral("long running"));

    orchestrator.setCancelEscalationGraceMs(0);
    orchestrator.cancel();
    QTRY_VERIFY_WITH_TIMEOUT(!orchestrator.isRunning(), 3000);
}

QTEST_MAIN(TestRuntimeOrchestrator)
#include "TestRuntimeOrchestrator.moc"
