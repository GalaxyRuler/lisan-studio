#include "TerminalProfileModel.h"

#include <QDir>

TerminalProfile TerminalProfileModel::defaultPowerShellProfile(const QString &workingDirectory)
{
    TerminalProfile profile;
    profile.id = QStringLiteral("powershell");
    profile.name = QStringLiteral("PowerShell");
    profile.program = QStringLiteral("powershell.exe");
    profile.arguments = {QStringLiteral("-NoLogo")};
    profile.workingDirectory = QDir::fromNativeSeparators(workingDirectory);
    profile.requiresTrustedWorkspace = true;
    return profile;
}

TerminalProfile TerminalProfileModel::defaultCmdProfile(const QString &workingDirectory)
{
    TerminalProfile profile;
    profile.id = QStringLiteral("cmd");
    profile.name = QStringLiteral("cmd");
    profile.program = QStringLiteral("cmd.exe");
    profile.workingDirectory = QDir::fromNativeSeparators(workingDirectory);
    profile.requiresTrustedWorkspace = true;
    return profile;
}

TerminalProfile TerminalProfileModel::defaultWslBashProfile(const QString &workingDirectory)
{
    TerminalProfile profile;
    profile.id = QStringLiteral("wsl");
    profile.name = QStringLiteral("WSL Bash");
    profile.program = QStringLiteral("wsl.exe");
    profile.workingDirectory = QDir::fromNativeSeparators(workingDirectory);
    profile.requiresTrustedWorkspace = true;
    return profile;
}

TerminalLaunchPlan TerminalProfileModel::buildLaunchPlan(const TerminalProfile &profile, bool workspaceTrusted)
{
    TerminalLaunchPlan plan;
    if (profile.requiresTrustedWorkspace && !workspaceTrusted) {
        plan.reason = QString::fromUtf8("يتطلب فتح الطرفية الثقة بمساحة العمل.");
        return plan;
    }

    plan.allowed = true;
    plan.command.program = profile.program;
    plan.command.arguments = profile.arguments;
    plan.command.workingDirectory = QDir::fromNativeSeparators(profile.workingDirectory);
    return plan;
}
