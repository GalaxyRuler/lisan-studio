#include "WorkbenchState.h"

#include <QDir>

void WorkbenchState::setProjectRoot(const QString &path)
{
    rootPath = QDir::fromNativeSeparators(path);
    if (rootPath.isEmpty()) {
        rootPaths.clear();
    } else if (!rootPaths.contains(rootPath, Qt::CaseInsensitive)) {
        rootPaths.prepend(rootPath);
    }
}

QString WorkbenchState::projectRoot() const
{
    return rootPath;
}

void WorkbenchState::setProjectRoots(const QStringList &paths)
{
    rootPaths.clear();
    for (const QString &path : paths) {
        addProjectRoot(path);
    }
    if (rootPath.isEmpty() && !rootPaths.isEmpty()) {
        rootPath = rootPaths.first();
    }
}

QStringList WorkbenchState::projectRoots() const
{
    return rootPaths;
}

bool WorkbenchState::addProjectRoot(const QString &path)
{
    const QString normalized = QDir::fromNativeSeparators(path);
    if (normalized.isEmpty() || rootPaths.contains(normalized, Qt::CaseInsensitive)) {
        return false;
    }
    rootPaths.append(normalized);
    if (rootPath.isEmpty()) {
        rootPath = normalized;
    }
    return true;
}

bool WorkbenchState::removeProjectRoot(const QString &path)
{
    const QString normalized = QDir::fromNativeSeparators(path);
    for (int i = 0; i < rootPaths.size(); ++i) {
        if (rootPaths.at(i).compare(normalized, Qt::CaseInsensitive) != 0) {
            continue;
        }
        rootPaths.removeAt(i);
        if (rootPath.compare(normalized, Qt::CaseInsensitive) == 0) {
            rootPath = rootPaths.isEmpty() ? QString() : rootPaths.first();
        }
        return true;
    }
    return false;
}

int WorkbenchState::currentSearchGeneration() const
{
    return searchGeneration;
}

int WorkbenchState::nextSearchGeneration()
{
    return ++searchGeneration;
}

bool WorkbenchState::isCurrentSearchGeneration(int generation) const
{
    return generation == searchGeneration;
}

int WorkbenchState::addEditorSession(const QString &path)
{
    sessions.push_back({QDir::fromNativeSeparators(path), false});
    currentSessionIndex = sessions.size() - 1;
    return currentSessionIndex;
}

void WorkbenchState::removeEditorSession(int index)
{
    if (index < 0 || index >= sessions.size()) {
        return;
    }

    sessions.remove(index);
    if (sessions.isEmpty()) {
        currentSessionIndex = -1;
        return;
    }

    if (currentSessionIndex > index) {
        --currentSessionIndex;
    } else if (currentSessionIndex >= sessions.size()) {
        currentSessionIndex = sessions.size() - 1;
    }
}

int WorkbenchState::editorSessionCount() const
{
    return sessions.size();
}

QVector<EditorSessionState> WorkbenchState::editorSessions() const
{
    return sessions;
}

void WorkbenchState::setCurrentEditorSessionIndex(int index)
{
    if (index < 0 || index >= sessions.size()) {
        currentSessionIndex = sessions.isEmpty() ? -1 : currentSessionIndex;
        return;
    }
    currentSessionIndex = index;
}

int WorkbenchState::currentEditorSessionIndex() const
{
    return currentSessionIndex;
}

QString WorkbenchState::currentEditorPath() const
{
    if (currentSessionIndex < 0 || currentSessionIndex >= sessions.size()) {
        return QString();
    }
    return sessions.at(currentSessionIndex).path;
}

bool WorkbenchState::isCurrentEditorSessionDirty() const
{
    if (currentSessionIndex < 0 || currentSessionIndex >= sessions.size()) {
        return false;
    }
    return sessions.at(currentSessionIndex).dirty;
}

void WorkbenchState::setEditorSessionPath(int index, const QString &path)
{
    if (index < 0 || index >= sessions.size()) {
        return;
    }
    sessions[index].path = QDir::fromNativeSeparators(path);
}

void WorkbenchState::setEditorSessionDirty(int index, bool dirty)
{
    if (index < 0 || index >= sessions.size()) {
        return;
    }
    sessions[index].dirty = dirty;
}

int WorkbenchState::findEditorSessionByPath(const QString &path) const
{
    const QString normalizedPath = QDir::fromNativeSeparators(path);
    for (int i = 0; i < sessions.size(); ++i) {
        if (!normalizedPath.isEmpty() && sessions.at(i).path == normalizedPath) {
            return i;
        }
    }
    return -1;
}
