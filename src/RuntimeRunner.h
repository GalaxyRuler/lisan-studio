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

struct RuntimeResult
{
    int exitCode = -1;
    QString standardOutput;
    QString standardError;
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
    QProcessEnvironment processEnvironment() const;
    RuntimeResult runBlocking(RuntimeAction action, const QString &filePath, int timeoutMs = 30000) const;
    RuntimeResult runBlocking(RuntimeAction action, const QString &filePath, const QString &workingDirectory, int timeoutMs = 30000) const;

private:
    QString root;

    QString pythonExecutable() const;
};
