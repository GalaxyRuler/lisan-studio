#include "SettingsStore.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

SettingsStore::SettingsStore(const QString &settingsPath)
{
    if (!settingsPath.isEmpty()) {
        path = settingsPath;
        return;
    }

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    path = QDir(dir).filePath(QStringLiteral("settings.ini"));
}

QString SettingsStore::editorFontFamily() const
{
    QSettings settings(path, QSettings::IniFormat);
    return settings.value(QStringLiteral("editor/fontFamily"), QStringLiteral("Cascadia Code")).toString();
}

void SettingsStore::setEditorFontFamily(const QString &family)
{
    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("editor/fontFamily"), family);
}

int SettingsStore::editorFontSize() const
{
    QSettings settings(path, QSettings::IniFormat);
    return settings.value(QStringLiteral("editor/fontSize"), 12).toInt();
}

void SettingsStore::setEditorFontSize(int size)
{
    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("editor/fontSize"), qBound(8, size, 28));
}

QStringList SettingsStore::recentProjects() const
{
    QSettings settings(path, QSettings::IniFormat);
    return settings.value(QStringLiteral("project/recent")).toStringList();
}

void SettingsStore::addRecentProject(const QString &projectPath)
{
    QStringList projects = recentProjects();
    projects.removeAll(projectPath);
    projects.prepend(projectPath);
    while (projects.size() > 10) {
        projects.removeLast();
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("project/recent"), projects);
}

QString SettingsStore::settingsPath() const
{
    return path;
}

