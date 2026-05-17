#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

struct SavedWorkbenchSession
{
    QString projectRoot;
    QStringList openFiles;
    QStringList untitledDrafts;
    int activeFileIndex = -1;
    QString bottomPanelId;
};

class SettingsStore
{
public:
    explicit SettingsStore(const QString &settingsPath = QString());

    QString editorFontFamily() const;
    void setEditorFontFamily(const QString &family);

    int editorFontSize() const;
    void setEditorFontSize(int size);

    QString themePreference() const;
    void setThemePreference(const QString &preference);

    QStringList recentProjects() const;
    void addRecentProject(const QString &path);

    QStringList recentFiles() const;
    void addRecentFile(const QString &path);

    SavedWorkbenchSession savedWorkbenchSession() const;
    void saveWorkbenchSession(const SavedWorkbenchSession &session);

    QJsonObject shortcutSettingsJson() const;
    void saveShortcutSettingsJson(const QJsonObject &object);

    QString settingsPath() const;

private:
    QString path;
};

