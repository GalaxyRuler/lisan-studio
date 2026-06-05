#include "TerminalBackend.h"

#include <QProcess>

struct TerminalBackend::State
{
    QProcess *process = nullptr;
    bool running = false;
};

TerminalBackend::TerminalBackend(QObject *parent)
    : QObject(parent),
      state(std::make_unique<State>())
{
}

TerminalBackend::~TerminalBackend()
{
    close();
}

bool TerminalBackend::start(const TerminalCommand &command, QString *error)
{
    if (error) {
        error->clear();
    }
    if (state->running) {
        if (error) {
            *error = QStringLiteral("Terminal already running.");
        }
        return false;
    }
    if (command.program.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("Terminal program is empty.");
        }
        return false;
    }

    auto *process = new QProcess(this);
    process->setProgram(command.program);
    process->setArguments(command.arguments);
    if (!command.workingDirectory.isEmpty()) {
        process->setWorkingDirectory(command.workingDirectory);
    }
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect(process, &QProcess::readyRead, this, [this, process]() {
        const QString text = QString::fromLocal8Bit(process->readAll());
        if (!text.isEmpty()) {
            emit outputReceived(text);
        }
    });
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus) {
        state->running = false;
        emit processExited(exitCode);
    });

    process->start();
    if (!process->waitForStarted(5000)) {
        if (error) {
            *error = process->errorString();
        }
        delete process;
        return false;
    }

    state->process = process;
    state->running = true;
    return true;
}

bool TerminalBackend::writeInput(const QString &input, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!state->running || !state->process) {
        if (error) {
            *error = QStringLiteral("Terminal is not running.");
        }
        return false;
    }

    const QByteArray bytes = input.toLocal8Bit();
    const qint64 written = state->process->write(bytes);
    if (written != bytes.size() || !state->process->waitForBytesWritten(1000)) {
        if (error) {
            *error = state->process->errorString();
        }
        return false;
    }
    return true;
}

bool TerminalBackend::isRunning() const
{
    return state->running;
}

void TerminalBackend::close()
{
    if (!state->process) {
        state->running = false;
        return;
    }

    state->running = false;
    if (state->process->state() != QProcess::NotRunning) {
        state->process->terminate();
        if (!state->process->waitForFinished(2000)) {
            state->process->kill();
            state->process->waitForFinished(1000);
        }
    }
    delete state->process;
    state->process = nullptr;
}
