#include "RuntimeOrchestrator.h"

#include "RuntimeProblemParser.h"

#include <QDir>

RuntimeOrchestrator::RuntimeOrchestrator(QObject *parent)
    : QObject(parent),
      timeoutTimer(new QTimer(this)),
      killEscalationTimer(new QTimer(this))
{
    timeoutTimer->setObjectName(QStringLiteral("runtimeTimeoutTimer"));
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(timeoutMs);
    connect(timeoutTimer, &QTimer::timeout, this, &RuntimeOrchestrator::handleTimeout);

    killEscalationTimer->setObjectName(QStringLiteral("runtimeKillEscalationTimer"));
    killEscalationTimer->setSingleShot(true);
    killEscalationTimer->setInterval(cancelEscalationGraceMs);
    connect(killEscalationTimer, &QTimer::timeout, this, &RuntimeOrchestrator::escalateKill);
}

void RuntimeOrchestrator::setRuntimeRoot(const QString &path)
{
    runtime.setRuntimeRoot(path);
}

QString RuntimeOrchestrator::runtimeRoot() const
{
    return runtime.runtimeRoot();
}

QString RuntimeOrchestrator::pythonExecutablePath() const
{
    return runtime.pythonExecutablePath();
}

RuntimeDiagnostics RuntimeOrchestrator::diagnostics(int timeoutMs) const
{
    return runtime.diagnostics(timeoutMs);
}

RuntimeLaunchPlan RuntimeOrchestrator::buildLaunchPlan(
    RuntimeAction action,
    const QString &title,
    const QString &filePath,
    const QString &workingDirectory,
    bool reloadAfterSuccess) const
{
    return runtime.buildLaunchPlan(action, title, filePath, workingDirectory, reloadAfterSuccess);
}

bool RuntimeOrchestrator::start(const RuntimeLaunchPlan &plan, bool recordHistory)
{
    if (isRunning()) {
        emitOutput(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), QString::fromUtf8("هناك عملية قيد التشغيل"));
        emit statusChanged(QString::fromUtf8("هناك عملية قيد التشغيل"));
        return false;
    }

    if (recordHistory) {
        history.recordLaunch(plan);
        emit historyRecorded(history.entries().first());
    }

    activePlan = plan;
    handledError = false;
    activeStdout.clear();
    activeStderr.clear();

    emit outputCleared();
    const QString initialText = plan.initialOutput.section(QLatin1Char('\n'), 1).trimmed();
    emitOutput(OutputTranscriptChannel::System, plan.title, initialText);
    emit statusChanged(plan.runningStatus);
    setRunning(true);

    activeProcess = new QProcess(this);
    activeProcess->setProgram(plan.command.program);
    activeProcess->setArguments(plan.command.arguments);
    activeProcess->setWorkingDirectory(plan.command.workingDirectory);
    activeProcess->setProcessEnvironment(runtime.processEnvironment());
    connect(activeProcess, &QProcess::readyReadStandardOutput, this, &RuntimeOrchestrator::appendStdout);
    connect(activeProcess, &QProcess::readyReadStandardError, this, &RuntimeOrchestrator::appendStderr);
    connect(activeProcess, &QProcess::finished, this, &RuntimeOrchestrator::finishProcess);
    connect(activeProcess, &QProcess::errorOccurred, this, &RuntimeOrchestrator::handleProcessError);

    activeTimer.start();
    timeoutTimer->start();
    activeProcess->start();
    return true;
}

bool RuntimeOrchestrator::rerunLast()
{
    if (isRunning()) {
        emitOutput(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), QString::fromUtf8("هناك عملية قيد التشغيل"));
        emit statusChanged(QString::fromUtf8("هناك عملية قيد التشغيل"));
        return false;
    }

    const QVector<RuntimeHistoryEntry> entries = history.entries();
    if (entries.isEmpty()) {
        emitOutput(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), QString::fromUtf8("لا يوجد تشغيل سابق"));
        emit statusChanged(QString::fromUtf8("لا يوجد تشغيل سابق"));
        return false;
    }

    const RuntimeHistoryEntry lastRun = entries.first();
    const RuntimeLaunchPlan plan = runtime.buildLaunchPlan(
        lastRun.action,
        lastRun.title,
        lastRun.filePath,
        lastRun.workingDirectory,
        lastRun.reloadAfterSuccess);
    return start(plan, true);
}

void RuntimeOrchestrator::cancel()
{
    if (!isRunning()) {
        return;
    }

    emitOutput(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), QString::fromUtf8("تم طلب إيقاف العملية."));
    activeProcess->terminate();
    emit statusChanged(QString::fromUtf8("جار إيقاف التشغيل"));
    if (cancelEscalationGraceMs <= 0) {
        escalateKill();
    } else {
        killEscalationTimer->start(cancelEscalationGraceMs);
    }
}

bool RuntimeOrchestrator::isRunning() const
{
    return activeProcess && activeProcess->state() != QProcess::NotRunning;
}

bool RuntimeOrchestrator::hasHistory() const
{
    return !history.entries().isEmpty();
}

QVector<RuntimeHistoryEntry> RuntimeOrchestrator::historyEntries() const
{
    return history.entries();
}

RuntimeLaunchPlan RuntimeOrchestrator::lastCompletedPlan() const
{
    return completedPlan;
}

void RuntimeOrchestrator::setTimeoutMs(int newTimeoutMs)
{
    timeoutMs = qMax(1, newTimeoutMs);
    timeoutTimer->setInterval(timeoutMs);
}

void RuntimeOrchestrator::setCancelEscalationGraceMs(int graceMs)
{
    cancelEscalationGraceMs = qMax(0, graceMs);
    killEscalationTimer->setInterval(cancelEscalationGraceMs);
}

void RuntimeOrchestrator::appendStdout()
{
    if (!activeProcess) {
        return;
    }
    const QString text = QString::fromUtf8(activeProcess->readAllStandardOutput());
    activeStdout.append(text);
    emitOutput(OutputTranscriptChannel::Stdout, QStringLiteral("stdout"), text);
}

void RuntimeOrchestrator::appendStderr()
{
    if (!activeProcess) {
        return;
    }
    const QString text = QString::fromUtf8(activeProcess->readAllStandardError());
    activeStderr.append(text);
    emitOutput(OutputTranscriptChannel::Stderr, QStringLiteral("stderr"), text);
}

void RuntimeOrchestrator::finishProcess(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    if (!activeProcess) {
        return;
    }

    appendStdout();
    appendStderr();
    const QString finalText = QString::fromUtf8("رمز الخروج: %1\nالمدة: %2 ms")
        .arg(exitCode)
        .arg(activeTimer.isValid() ? activeTimer.elapsed() : 0);
    emitOutput(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), finalText);
    if (exitCode != 0) {
        const RuntimeProblemDetail detail = parseRuntimeProblemDetail(
            activeStderr.isEmpty() ? activeStdout : activeStderr,
            activePlan.title,
            exitCode);
        emit problemDetected(QString::fromUtf8("خطأ"), detail.message, activePlan.filePath, detail.line);
        emit problemsRequested();
    }
    completedPlan = activePlan;
    complete(QString::fromUtf8("%1 انتهى: %2").arg(activePlan.title).arg(exitCode));
}

void RuntimeOrchestrator::handleProcessError(QProcess::ProcessError error)
{
    if (!activeProcess || handledError) {
        return;
    }

    if (error != QProcess::FailedToStart) {
        return;
    }

    handledError = true;
    emitOutput(
        OutputTranscriptChannel::System,
        QString::fromUtf8("النظام"),
        QString::fromUtf8("تعذر بدء العملية: %1\nرمز الخروج: -1").arg(activeProcess->errorString()));
    emit problemDetected(
        QString::fromUtf8("خطأ"),
        QString::fromUtf8("تعذر بدء %1: %2").arg(activePlan.title, activeProcess->errorString()),
        activePlan.filePath,
        0);
    completedPlan = activePlan;
    complete(QString::fromUtf8("تعذر بدء %1").arg(activePlan.title));
}

void RuntimeOrchestrator::handleTimeout()
{
    if (!isRunning()) {
        return;
    }

    emitOutput(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), QString::fromUtf8("انتهت مهلة التشغيل."));
    emit problemDetected(
        QString::fromUtf8("خطأ"),
        QString::fromUtf8("انتهت مهلة %1 بعد 30 ثانية.").arg(activePlan.title),
        activePlan.filePath,
        0);
    activeProcess->terminate();
    if (cancelEscalationGraceMs <= 0) {
        escalateKill();
    } else {
        killEscalationTimer->start(cancelEscalationGraceMs);
    }
    emit statusChanged(QString::fromUtf8("انتهت مهلة التشغيل"));
}

void RuntimeOrchestrator::escalateKill()
{
    if (!isRunning()) {
        return;
    }

    emitOutput(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), QString::fromUtf8("لم تتوقف العملية، سيتم إجبار الإيقاف."));
    activeProcess->kill();
    activeProcess->waitForFinished(250);
    emit statusChanged(QString::fromUtf8("تم إجبار إيقاف التشغيل"));
}

void RuntimeOrchestrator::complete(const QString &statusText)
{
    if (!activeProcess) {
        return;
    }

    const bool reloadAfterSuccess = activePlan.reloadAfterSuccess;
    const QString runFilePath = activePlan.filePath;
    const int exitCode = activeProcess->exitCode();
    QProcess *finishedProcess = activeProcess;
    activeProcess = nullptr;
    timeoutTimer->stop();
    killEscalationTimer->stop();
    setRunning(false);
    emit statusChanged(statusText);
    finishedProcess->deleteLater();

    if (reloadAfterSuccess && exitCode == 0) {
        emit reloadRequested(runFilePath);
    }
}

void RuntimeOrchestrator::setRunning(bool running)
{
    emit runningChanged(running);
}

void RuntimeOrchestrator::emitOutput(OutputTranscriptChannel channel, const QString &label, const QString &text)
{
    if (text.isEmpty()) {
        return;
    }
    emit outputProduced(channel, label, text);
}
