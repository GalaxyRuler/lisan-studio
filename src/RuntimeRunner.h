#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

enum class RuntimeAction
{
    Run,
    Lint,
    Format
};

struct RuntimeCommand
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
};

struct RuntimeLaunchPlan
{
    RuntimeAction action = RuntimeAction::Run;
    QString title;
    QString filePath;
    RuntimeCommand command;
    bool reloadAfterSuccess = false;
    QString initialOutput;
    QString runningStatus;
};

struct RuntimeResult
{
    int exitCode = -1;
    QString standardOutput;
    QString standardError;
};

struct RuntimeDiagnostics
{
    QString pythonExecutable;
    bool pythonExists = false;
    bool packageAvailable = false;
    QString packageVersion;
    bool runModuleAvailable = false;
    bool lintModuleAvailable = false;
    bool formatModuleAvailable = false;
    QString statusText;
};

class RuntimeRunner final : public QObject
{
    Q_OBJECT

public:
    explicit RuntimeRunner(QObject *parent = nullptr);

    void setRuntimeRoot(const QString &path);
    QString runtimeRoot() const;

    RuntimeCommand buildCommand(RuntimeAction action, const QString &filePath) const;
    RuntimeCommand buildCommand(RuntimeAction action, const QString &filePath, const QString &workingDirectory) const;
    QString pythonExecutablePath() const;
    RuntimeLaunchPlan buildLaunchPlan(
        RuntimeAction action,
        const QString &title,
        const QString &filePath,
        const QString &workingDirectory,
        bool reloadAfterSuccess = false) const;
    QProcessEnvironment processEnvironment() const;
    RuntimeDiagnostics diagnostics(int timeoutMs = 5000) const;
    RuntimeResult runBlocking(RuntimeAction action, const QString &filePath, int timeoutMs = 30000) const;
    RuntimeResult runBlocking(RuntimeAction action, const QString &filePath, const QString &workingDirectory, int timeoutMs = 30000) const;

private:
    QString root;

    QString pythonExecutable() const;
};
