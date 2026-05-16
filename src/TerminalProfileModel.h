#pragma once

#include <QString>
#include <QStringList>

struct TerminalCommand
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
};

struct TerminalProfile
{
    QString id;
    QString name;
    QString program;
    QStringList arguments;
    QString workingDirectory;
    bool requiresTrustedWorkspace = true;
};

struct TerminalLaunchPlan
{
    bool allowed = false;
    QString reason;
    TerminalCommand command;
};

class TerminalProfileModel final
{
public:
    static TerminalProfile defaultPowerShellProfile(const QString &workingDirectory);
    static TerminalLaunchPlan buildLaunchPlan(const TerminalProfile &profile, bool workspaceTrusted);
};
