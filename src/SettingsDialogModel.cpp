#include "SettingsDialogModel.h"

#include <QDir>

namespace {
QString readyStatus(bool available)
{
    return available ? QString::fromUtf8("جاهز") : QString::fromUtf8("غير متوفر");
}

QString resolvedEditorFontFamily(const QString &configuredFamily, const QStringList &families)
{
    for (const QString &family : families) {
        if (family.compare(configuredFamily, Qt::CaseInsensitive) == 0) {
            return family;
        }
    }
    return families.isEmpty() ? QString() : families.first();
}

QStringList nativeProjectPaths(const QStringList &paths)
{
    QStringList nativePaths;
    for (const QString &path : paths) {
        nativePaths.append(QDir::toNativeSeparators(path));
    }
    return nativePaths;
}

QString themeLabel(const QString &preference)
{
    return preference == QStringLiteral("light")
        ? QString::fromUtf8("فاتح")
        : QString::fromUtf8("داكن");
}
}

SettingsDialogState SettingsDialogModel::build(
    const SettingsStore &settings,
    const RuntimeDiagnostics &diagnostics,
    const QStringList &editorFontFamilies)
{
    SettingsDialogState state;
    state.categories = {
        QString::fromUtf8("المحرر"),
        QString::fromUtf8("التشغيل"),
        QString::fromUtf8("المشاريع"),
    };
    state.editorFontFamilies = editorFontFamilies;
    state.selectedEditorFontFamily = resolvedEditorFontFamily(settings.editorFontFamily(), editorFontFamilies);
    state.editorFontSize = settings.editorFontSize();
    state.themeLabel = themeLabel(settings.themePreference());
    state.runtimePythonPath = QDir::toNativeSeparators(diagnostics.pythonExecutable);
    state.runtimePackageStatus = diagnostics.statusText;
    state.runtimeRunStatus = readyStatus(diagnostics.runModuleAvailable);
    state.runtimeLintStatus = readyStatus(diagnostics.lintModuleAvailable);
    state.runtimeFormatStatus = readyStatus(diagnostics.formatModuleAvailable);
    state.recentProjects = nativeProjectPaths(settings.recentProjects());
    return state;
}
