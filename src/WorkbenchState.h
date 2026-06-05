#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct EditorSessionState
{
    QString path;
    bool dirty = false;
};

class WorkbenchState final
{
public:
    void setProjectRoot(const QString &path);
    QString projectRoot() const;
    void setProjectRoots(const QStringList &paths);
    QStringList projectRoots() const;
    bool addProjectRoot(const QString &path);
    bool removeProjectRoot(const QString &path);

    int currentSearchGeneration() const;
    int nextSearchGeneration();
    bool isCurrentSearchGeneration(int generation) const;

    int addEditorSession(const QString &path);
    void removeEditorSession(int index);
    int editorSessionCount() const;
    QVector<EditorSessionState> editorSessions() const;

    void setCurrentEditorSessionIndex(int index);
    int currentEditorSessionIndex() const;
    QString currentEditorPath() const;
    bool isCurrentEditorSessionDirty() const;

    void setEditorSessionPath(int index, const QString &path);
    void setEditorSessionDirty(int index, bool dirty);
    int findEditorSessionByPath(const QString &path) const;

private:
    QString rootPath;
    QStringList rootPaths;
    int searchGeneration = 0;
    QVector<EditorSessionState> sessions;
    int currentSessionIndex = -1;
};
