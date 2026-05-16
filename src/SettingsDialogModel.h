#pragma once

#include "RuntimeRunner.h"
#include "SettingsStore.h"

#include <QString>
#include <QStringList>

struct SettingsDialogState
{
    QStringList categories;
    QStringList editorFontFamilies;
    QString selectedEditorFontFamily;
    int editorFontSize = 12;
    QString themeLabel;
    QString runtimePythonPath;
    QString runtimePackageStatus;
    QString runtimeRunStatus;
    QString runtimeLintStatus;
    QString runtimeFormatStatus;
    QStringList recentProjects;
};

class SettingsDialogModel final
{
public:
    static SettingsDialogState build(
        const SettingsStore &settings,
        const RuntimeDiagnostics &diagnostics,
        const QStringList &editorFontFamilies);
};
