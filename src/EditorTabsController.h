#pragma once

#include "DocumentRegistry.h"
#include "WorkspaceSettingsStore.h"

#include <QFont>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

class EditorSurface;
class QTabWidget;
class WorkbenchState;

class EditorTabsController final : public QObject
{
    Q_OBJECT

public:
    EditorTabsController(
        QTabWidget *tabs,
        DocumentRegistry &documents,
        WorkbenchState &workbench,
        QObject *parent = nullptr);

    bool openFile(const QString &path, QString *error = nullptr);
    bool openOrFocusByDocumentId(DocumentId id);
    EditorSurface *createUntitled(const QString &title = QString());
    void closeTab(int index);
    bool saveCurrent(QString *error = nullptr);
    bool saveCurrentAs(const QString &path, QString *error = nullptr);
    EditorSurface *currentSurface() const;
    EditorSurface *surfaceForDocument(DocumentId id) const;
    DocumentId documentIdForSurface(EditorSurface *surface) const;
    void applyFont(const QFont &font);
    void applyWorkspaceSettings(const WorkspaceSettings &settings);
    void syncSessionsInto(WorkbenchState &target) const;
    QStringList openFilePaths() const;
    QStringList untitledDrafts() const;
    int activeFileIndexAmongSavedFiles() const;
    QVector<EditorSurface *> dirtySurfaces() const;
    QStringList dirtyFilePaths() const;
    void discardUntitledDrafts();
    void closeTabsMatching(const std::function<bool(EditorSurface *)> &matches);

signals:
    void currentEditorChanged(EditorSurface *surface);
    void tabAdded(EditorSurface *surface, DocumentId id);
    void tabClosed(DocumentId id);
    void dirtyStateChanged(DocumentId id, bool dirty);
    void pathChanged(DocumentId id, QString path);
    void titleChanged(EditorSurface *surface, QString title);

private:
    QTabWidget *editorTabs = nullptr;
    DocumentRegistry &documentRegistry;
    WorkbenchState &workbenchState;
    WorkspaceSettings workspaceSettings;
    QFont editorFont;
    bool hasEditorFont = false;
    EditorSurface *currentEditor = nullptr;

    EditorSurface *createSurface(const QString &title, DocumentId id, bool emitCurrentEditor);
    void activateSurface(EditorSurface *surface, bool forceSignal = false, bool emitSignal = true);
    void setDocumentIdForSurface(EditorSurface *surface, DocumentId id);
    void syncDocumentRegistryFromSurface(EditorSurface *surface);
    void markDocumentSaved(EditorSurface *surface);
    void syncEditorSession(EditorSurface *surface);
    void updateEditorTabTitle(EditorSurface *surface);
    void applyFontToSurface(EditorSurface *surface) const;
    void applyWorkspaceSettingsToSurface(EditorSurface *surface) const;
};
