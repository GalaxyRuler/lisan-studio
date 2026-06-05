#include "SettingsStore.h"

#include <QDir>
#include <QJsonDocument>
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

QString SettingsStore::themePreference() const
{
    QSettings settings(path, QSettings::IniFormat);
    const QString preference = settings.value(QStringLiteral("editor/theme"), QStringLiteral("dark")).toString();
    return preference == QStringLiteral("light") ? QStringLiteral("light") : QStringLiteral("dark");
}

void SettingsStore::setThemePreference(const QString &preference)
{
    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(
        QStringLiteral("editor/theme"),
        preference == QStringLiteral("light") ? QStringLiteral("light") : QStringLiteral("dark"));
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

QStringList SettingsStore::recentFiles() const
{
    QSettings settings(path, QSettings::IniFormat);
    return settings.value(QStringLiteral("files/recent")).toStringList();
}

void SettingsStore::addRecentFile(const QString &filePath)
{
    const QString normalizedPath = QDir::fromNativeSeparators(filePath);
    QStringList files = recentFiles();
    files.removeAll(normalizedPath);
    files.prepend(normalizedPath);
    while (files.size() > 20) {
        files.removeLast();
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("files/recent"), files);
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
    session.untitledDrafts = settings.value(QStringLiteral("session/untitledDrafts")).toStringList();
    session.untitledDrafts.removeAll(QString());
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
    if (session.untitledDrafts.isEmpty()) {
        settings.remove(QStringLiteral("session/untitledDrafts"));
    } else {
        settings.setValue(QStringLiteral("session/untitledDrafts"), session.untitledDrafts);
    }
    settings.setValue(QStringLiteral("session/activeFileIndex"), activeFileIndex);
    settings.setValue(QStringLiteral("session/bottomPanelId"), session.bottomPanelId);
}

bool SettingsStore::hasNonOrderlyShutdown() const
{
    QSettings settings(path, QSettings::IniFormat);
    return settings.value(QStringLiteral("session/nonOrderlyShutdown"), false).toBool();
}

void SettingsStore::markWorkbenchSessionStarted()
{
    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("session/nonOrderlyShutdown"), true);
    settings.sync();
}

void SettingsStore::markWorkbenchSessionClosedGracefully()
{
    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("session/nonOrderlyShutdown"), false);
    settings.sync();
}

QJsonObject SettingsStore::shortcutSettingsJson() const
{
    QSettings settings(path, QSettings::IniFormat);
    const QByteArray json = settings.value(QStringLiteral("shortcuts/json")).toString().toUtf8();
    if (json.isEmpty()) {
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }
    return document.object();
}

void SettingsStore::saveShortcutSettingsJson(const QJsonObject &object)
{
    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(
        QStringLiteral("shortcuts/json"),
        QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)));
}

QString SettingsStore::settingsPath() const
{
    return path;
}
