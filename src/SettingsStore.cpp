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
    return settings.value(QStringLiteral("editor/fontFamily"), QStringLiteral("Segoe UI")).toString();
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

SavedWorkbenchSession SettingsStore::savedWorkbenchSession() const
{
    QSettings settings(path, QSettings::IniFormat);
    SavedWorkbenchSession session;
    session.projectRoot = QDir::fromNativeSeparators(settings.value(QStringLiteral("session/projectRoot")).toString());
    session.openFiles = settings.value(QStringLiteral("session/openFiles")).toStringList();
    for (QString &openFile : session.openFiles) {
        openFile = QDir::fromNativeSeparators(openFile);
    }
    session.activeFileIndex = settings.value(QStringLiteral("session/activeFileIndex"), -1).toInt();
    if (session.activeFileIndex < 0 || session.activeFileIndex >= session.openFiles.size()) {
        session.activeFileIndex = session.openFiles.isEmpty() ? -1 : 0;
    }
    session.bottomPanelId = settings.value(QStringLiteral("session/bottomPanelId")).toString();
    return session;
}

void SettingsStore::saveWorkbenchSession(const SavedWorkbenchSession &session)
{
    QStringList openFiles = session.openFiles;
    for (QString &openFile : openFiles) {
        openFile = QDir::fromNativeSeparators(openFile);
    }

    int activeFileIndex = session.activeFileIndex;
    if (activeFileIndex < 0 || activeFileIndex >= openFiles.size()) {
        activeFileIndex = openFiles.isEmpty() ? -1 : 0;
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("session/projectRoot"), QDir::fromNativeSeparators(session.projectRoot));
    settings.setValue(QStringLiteral("session/openFiles"), openFiles);
    settings.setValue(QStringLiteral("session/activeFileIndex"), activeFileIndex);
    settings.setValue(QStringLiteral("session/bottomPanelId"), session.bottomPanelId);
}

QString SettingsStore::settingsPath() const
{
    return path;
}
