#include "RuntimeRunner.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

RuntimeRunner::RuntimeRunner(QObject *parent)
    : QObject(parent)
{
}

void RuntimeRunner::setRuntimeRoot(const QString &path)
{
    root = QDir::fromNativeSeparators(path);
}

QString RuntimeRunner::runtimeRoot() const
{
    return root;
}

RuntimeCommand RuntimeRunner::buildCommand(RuntimeAction action, const QString &filePath) const
{
    RuntimeCommand command;
    command.program = pythonExecutable();
    command.workingDirectory = QFileInfo(filePath).absolutePath();

    switch (action) {
    case RuntimeAction::Run:
        command.arguments = {QStringLiteral("-m"), QStringLiteral("arabicpython.cli"), filePath};
        break;
    case RuntimeAction::Lint:
        command.arguments = {QStringLiteral("-m"), QStringLiteral("arabicpython.linter"), filePath};
        break;
    case RuntimeAction::Format:
        command.arguments = {QStringLiteral("-m"), QStringLiteral("arabicpython.formatter"), filePath};
        break;
    }

    return command;
}

QProcessEnvironment RuntimeRunner::processEnvironment() const
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.remove(QStringLiteral("PYTHONHOME"));
    environment.remove(QStringLiteral("PYTHONPATH"));
    environment.insert(QStringLiteral("PYTHONNOUSERSITE"), QStringLiteral("1"));
    environment.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    environment.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    return environment;
}

RuntimeResult RuntimeRunner::runBlocking(RuntimeAction action, const QString &filePath, int timeoutMs) const
{
    const RuntimeCommand command = buildCommand(action, filePath);
    QProcess process;
    process.setProgram(command.program);
    process.setArguments(command.arguments);
    process.setWorkingDirectory(command.workingDirectory);
    process.setProcessEnvironment(processEnvironment());
    process.start();

    RuntimeResult result;
    if (!process.waitForStarted(5000)) {
        result.standardError = process.errorString();
        return result;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(3000);
        result.standardError = QString::fromUtf8("انتهت مهلة التشغيل.");
        return result;
    }

    result.exitCode = process.exitCode();
    result.standardOutput = QString::fromUtf8(process.readAllStandardOutput());
    result.standardError = QString::fromUtf8(process.readAllStandardError());
    return result;
}

QString RuntimeRunner::pythonExecutable() const
{
    const QString normalizedRoot = root.isEmpty() ? QCoreApplication::applicationDirPath() + QStringLiteral("/runtime") : root;
    const QString candidate = QDir(normalizedRoot).filePath(QStringLiteral("python/python.exe"));
    return QDir::toNativeSeparators(candidate);
}
