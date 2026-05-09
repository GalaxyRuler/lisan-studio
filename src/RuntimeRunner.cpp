#include "RuntimeRunner.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

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
    return buildCommand(action, filePath, QFileInfo(filePath).absolutePath());
}

RuntimeCommand RuntimeRunner::buildCommand(RuntimeAction action, const QString &filePath, const QString &workingDirectory) const
{
    RuntimeCommand command;
    command.program = pythonExecutable();
    command.workingDirectory = workingDirectory.isEmpty() ? QFileInfo(filePath).absolutePath() : workingDirectory;

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

RuntimeDiagnostics RuntimeRunner::diagnostics(int timeoutMs) const
{
    RuntimeDiagnostics diagnostics;
    diagnostics.pythonExecutable = pythonExecutable();
    diagnostics.pythonExists = QFileInfo::exists(diagnostics.pythonExecutable);

    if (!diagnostics.pythonExists) {
        diagnostics.statusText = QString::fromUtf8("غير متوفر: لم يتم العثور على Python المضمن.");
        return diagnostics;
    }

    QProcess process;
    process.setProgram(diagnostics.pythonExecutable);
    process.setArguments({
        QStringLiteral("-c"),
        QStringLiteral(
            "import importlib.metadata as m\n"
            "import importlib.util as u\n"
            "try:\n"
            "    print(m.version('lughat-althuban'))\n"
            "except Exception:\n"
            "    print('')\n"
            "for name in ('arabicpython.cli','arabicpython.linter','arabicpython.formatter'):\n"
            "    print('1' if u.find_spec(name) else '0')\n")
    });
    process.setProcessEnvironment(processEnvironment());
    process.start();

    if (!process.waitForStarted(5000)) {
        diagnostics.statusText = QString::fromUtf8("غير متوفر: تعذر بدء Python المضمن.");
        return diagnostics;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(3000);
        diagnostics.statusText = QString::fromUtf8("غير متوفر: انتهت مهلة فحص التشغيل.");
        return diagnostics;
    }

    const QStringList lines = QString::fromUtf8(process.readAllStandardOutput()).split(
        QRegularExpression(QStringLiteral("[\\r\\n]+")),
        Qt::SkipEmptyParts);
    diagnostics.packageVersion = lines.value(0).trimmed();
    diagnostics.packageAvailable = !diagnostics.packageVersion.isEmpty();
    diagnostics.runModuleAvailable = lines.value(1).trimmed() == QStringLiteral("1");
    diagnostics.lintModuleAvailable = lines.value(2).trimmed() == QStringLiteral("1");
    diagnostics.formatModuleAvailable = lines.value(3).trimmed() == QStringLiteral("1");

    diagnostics.statusText = diagnostics.packageAvailable && diagnostics.runModuleAvailable
        ? QString::fromUtf8("جاهز: lughat-althuban %1").arg(diagnostics.packageVersion)
        : QString::fromUtf8("غير متوفر: حزمة لغة الثعبان غير جاهزة.");
    return diagnostics;
}

RuntimeResult RuntimeRunner::runBlocking(RuntimeAction action, const QString &filePath, int timeoutMs) const
{
    return runBlocking(action, filePath, QFileInfo(filePath).absolutePath(), timeoutMs);
}

RuntimeResult RuntimeRunner::runBlocking(RuntimeAction action, const QString &filePath, const QString &workingDirectory, int timeoutMs) const
{
    const RuntimeCommand command = buildCommand(action, filePath, workingDirectory);
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
