#pragma once

#include "OutputTranscript.h"
#include "RuntimeHistory.h"
#include "RuntimeRunner.h"

#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QTimer>

class RuntimeOrchestrator final : public QObject
{
    Q_OBJECT

public:
    explicit RuntimeOrchestrator(QObject *parent = nullptr);

    void setRuntimeRoot(const QString &path);
    QString runtimeRoot() const;
    QString pythonExecutablePath() const;
    RuntimeDiagnostics diagnostics(int timeoutMs = 5000) const;
    RuntimeLaunchPlan buildLaunchPlan(
        RuntimeAction action,
        const QString &title,
        const QString &filePath,
        const QString &workingDirectory,
        bool reloadAfterSuccess = false) const;

    bool start(const RuntimeLaunchPlan &plan, bool recordHistory = true);
    bool rerunLast();
    void cancel();

    bool isRunning() const;
    bool hasHistory() const;
    QVector<RuntimeHistoryEntry> historyEntries() const;
    RuntimeLaunchPlan lastCompletedPlan() const;
    void setTimeoutMs(int timeoutMs);
    void setCancelEscalationGraceMs(int graceMs);

signals:
    void outputCleared();
    void outputProduced(OutputTranscriptChannel channel, const QString &label, const QString &text);
    void problemDetected(const QString &severity, const QString &message, const QString &path, int line);
    void problemsRequested();
    void statusChanged(const QString &statusText);
    void runningChanged(bool running);
    void historyRecorded(const RuntimeHistoryEntry &entry);
    void reloadRequested(const QString &path);

private slots:
    void appendStdout();
    void appendStderr();
    void finishProcess(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProcessError(QProcess::ProcessError error);
    void handleTimeout();
    void escalateKill();

private:
    RuntimeRunner runtime;
    RuntimeHistory history;
    QProcess *activeProcess = nullptr;
    QTimer *timeoutTimer = nullptr;
    QTimer *killEscalationTimer = nullptr;
    QElapsedTimer activeTimer;
    RuntimeLaunchPlan activePlan;
    RuntimeLaunchPlan completedPlan;
    QString activeStdout;
    QString activeStderr;
    bool handledError = false;
    int timeoutMs = 30000;
    int cancelEscalationGraceMs = 1500;

    void complete(const QString &statusText);
    void setRunning(bool running);
    void emitOutput(OutputTranscriptChannel channel, const QString &label, const QString &text);
};
