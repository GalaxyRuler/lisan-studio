#pragma once

#include "TerminalProfileModel.h"

#include <QObject>

#include <memory>

class TerminalBackend final : public QObject
{
    Q_OBJECT

public:
    explicit TerminalBackend(QObject *parent = nullptr);
    ~TerminalBackend() override;

    bool start(const TerminalCommand &command, QString *error = nullptr);
    bool writeInput(const QString &input, QString *error = nullptr);
    bool isRunning() const;
    void close();

signals:
    void outputReceived(const QString &text);
    void processExited(int exitCode);

private:
    struct State;
    std::unique_ptr<State> state;
};
