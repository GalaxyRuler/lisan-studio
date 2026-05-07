#pragma once

#include <QString>
#include <QStringList>

class SettingsStore
{
public:
    explicit SettingsStore(const QString &settingsPath = QString());

    QString editorFontFamily() const;
    void setEditorFontFamily(const QString &family);

    int editorFontSize() const;
    void setEditorFontSize(int size);

    QStringList recentProjects() const;
    void addRecentProject(const QString &path);

    QString settingsPath() const;

private:
    QString path;
};

