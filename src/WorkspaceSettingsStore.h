#pragma once

#include <QString>

struct WorkspaceSettings
{
    bool trusted = false;
    bool trimTrailingWhitespaceOnSave = false;
    QString defaultRunWorkingDirectory;
};

class WorkspaceSettingsStore final
{
public:
    explicit WorkspaceSettingsStore(const QString &workspaceRoot);

    QString settingsFilePath() const;
    QString trustAuditFilePath() const;
    WorkspaceSettings load(QString *error = nullptr) const;
    bool save(const WorkspaceSettings &settings, QString *error = nullptr) const;

private:
    QString rootPath;
};
