#include "EditorTabsController.h"

#include "EditorSurface.h"
#include "WorkbenchState.h"

#include <QFileInfo>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTextDocument>
#include <QVariant>

namespace {
constexpr const char *DocumentIdProperty = "lisanDocumentId";
}

EditorTabsController::EditorTabsController(
    QTabWidget *tabs,
    DocumentRegistry &documents,
    WorkbenchState &workbench,
    QObject *parent)
    : QObject(parent),
      editorTabs(tabs),
      documentRegistry(documents),
      workbenchState(workbench)
{
    Q_ASSERT(editorTabs != nullptr);
    connect(editorTabs, &QTabWidget::currentChanged, this, [this](int index) {
        activateSurface(qobject_cast<EditorSurface *>(editorTabs->widget(index)));
    });
}

bool EditorTabsController::openFile(const QString &path, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!editorTabs) {
        return false;
    }

    const QString normalizedPath = QFileInfo(path).absoluteFilePath();
    const DocumentId existingId = documentRegistry.findByPath(normalizedPath);
    const bool alreadyRegistered = existingId.isValid();
    if (EditorSurface *existingSurface = surfaceForDocument(existingId)) {
        {
            const QSignalBlocker blocker(editorTabs);
            editorTabs->setCurrentWidget(existingSurface);
        }
        activateSurface(existingSurface, true);
        return true;
    }

    EditorSurface *target = currentEditor;
    if (!target || !target->currentFilePath().isEmpty() || target->isDirty() || editorTabs->count() > 1) {
        target = createSurface(QFileInfo(normalizedPath).fileName(), documentRegistry.createUntitled(), false);
    }

    const DocumentId previousId = documentIdForSurface(target);
    const DocumentId openedId = documentRegistry.openPath(normalizedPath, error);
    if (!openedId.isValid()) {
        return false;
    }

    if (!target->openFile(normalizedPath, error)) {
        if (!alreadyRegistered) {
            documentRegistry.close(openedId);
        }
        return false;
    }
    if (previousId.isValid() && previousId != openedId) {
        documentRegistry.close(previousId);
    }
    setDocumentIdForSurface(target, openedId);
    documentRegistry.markClean(openedId);
    activateSurface(target, true);
    syncEditorSession(target);
    updateEditorTabTitle(target);
    emit pathChanged(openedId, normalizedPath);
    emit dirtyStateChanged(openedId, false);
    return true;
}

bool EditorTabsController::openOrFocusByDocumentId(DocumentId id)
{
    EditorSurface *surface = surfaceForDocument(id);
    if (!surface) {
        return false;
    }

    {
        const QSignalBlocker blocker(editorTabs);
        editorTabs->setCurrentWidget(surface);
    }
    activateSurface(surface, true);
    return true;
}

EditorSurface *EditorTabsController::createUntitled(const QString &title)
{
    return createSurface(title.isEmpty() ? QString::fromUtf8("ملف جديد") : title, documentRegistry.createUntitled(), true);
}

void EditorTabsController::closeTab(int index)
{
    if (!editorTabs || index < 0 || index >= editorTabs->count()) {
        return;
    }

    auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(index));
    if (!surface) {
        return;
    }

    if (editorTabs->count() == 1) {
        const DocumentId previousId = documentIdForSurface(surface);
        surface->resetForNewFile();
        if (previousId.isValid()) {
            documentRegistry.close(previousId);
        }
        const DocumentId newId = documentRegistry.createUntitled();
        setDocumentIdForSurface(surface, newId);
        syncEditorSession(surface);
        updateEditorTabTitle(surface);
        activateSurface(surface, true);
        emit tabClosed(previousId);
        emit tabAdded(surface, newId);
        emit pathChanged(newId, QString());
        emit dirtyStateChanged(newId, false);
        return;
    }

    const DocumentId closedId = documentIdForSurface(surface);
    if (closedId.isValid()) {
        documentRegistry.close(closedId);
    }
    editorTabs->removeTab(index);
    workbenchState.removeEditorSession(index);
    surface->deleteLater();
    activateSurface(qobject_cast<EditorSurface *>(editorTabs->currentWidget()), true);
    emit tabClosed(closedId);
}

bool EditorTabsController::saveCurrent(QString *error)
{
    if (!currentEditor) {
        return false;
    }
    if (error) {
        error->clear();
    }
    if (!currentEditor->saveFile(error)) {
        return false;
    }
    markDocumentSaved(currentEditor);
    return true;
}

bool EditorTabsController::saveCurrentAs(const QString &path, QString *error)
{
    if (!currentEditor) {
        return false;
    }
    if (error) {
        error->clear();
    }
    if (!currentEditor->saveFileAs(path, error)) {
        return false;
    }
    markDocumentSaved(currentEditor);
    return true;
}

EditorSurface *EditorTabsController::currentSurface() const
{
    return currentEditor;
}

EditorSurface *EditorTabsController::surfaceForDocument(DocumentId id) const
{
    if (!id.isValid() || !editorTabs) {
        return nullptr;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (surface && documentIdForSurface(surface) == id) {
            return surface;
        }
    }
    return nullptr;
}

DocumentId EditorTabsController::documentIdForSurface(EditorSurface *surface) const
{
    if (!surface) {
        return {};
    }

    bool ok = false;
    const int id = surface->property(DocumentIdProperty).toInt(&ok);
    return ok && id > 0 ? DocumentId(id) : DocumentId();
}

void EditorTabsController::applyFont(const QFont &font)
{
    editorFont = font;
    hasEditorFont = true;
    if (!editorTabs) {
        return;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        applyFontToSurface(qobject_cast<EditorSurface *>(editorTabs->widget(i)));
    }
}

void EditorTabsController::applyWorkspaceSettings(const WorkspaceSettings &settings)
{
    workspaceSettings = settings;
    if (!editorTabs) {
        return;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        applyWorkspaceSettingsToSurface(qobject_cast<EditorSurface *>(editorTabs->widget(i)));
    }
}

void EditorTabsController::syncSessionsInto(WorkbenchState &target) const
{
    if (!editorTabs) {
        return;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (!surface) {
            continue;
        }
        const DocumentRecord record = documentRegistry.document(documentIdForSurface(surface));
        target.setEditorSessionPath(i, record.id.isValid() ? record.path : surface->currentFilePath());
        target.setEditorSessionDirty(i, record.id.isValid() ? record.dirty : surface->isDirty());
    }
}

QStringList EditorTabsController::openFilePaths() const
{
    QStringList paths;
    if (!editorTabs) {
        return paths;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (surface && !surface->currentFilePath().isEmpty()) {
            paths.append(surface->currentFilePath());
        }
    }
    return paths;
}

QStringList EditorTabsController::untitledDrafts() const
{
    QStringList drafts;
    if (!editorTabs) {
        return drafts;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (!surface || !surface->currentFilePath().isEmpty()) {
            continue;
        }

        const QString text = surface->toPlainText();
        if (surface->isDirty() || !text.isEmpty()) {
            drafts.append(text);
        }
    }
    return drafts;
}

int EditorTabsController::activeFileIndexAmongSavedFiles() const
{
    if (!editorTabs) {
        return -1;
    }

    int fileIndex = -1;
    for (int i = 0; i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (!surface || surface->currentFilePath().isEmpty()) {
            continue;
        }

        ++fileIndex;
        if (i == editorTabs->currentIndex()) {
            return fileIndex;
        }
    }
    return -1;
}

QVector<EditorSurface *> EditorTabsController::dirtySurfaces() const
{
    QVector<EditorSurface *> surfaces;
    if (!editorTabs) {
        return surfaces;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        const DocumentId id = documentIdForSurface(surface);
        if (surface && documentRegistry.document(id).dirty) {
            surfaces.append(surface);
        }
    }
    return surfaces;
}

QStringList EditorTabsController::dirtyFilePaths() const
{
    QStringList paths;
    for (EditorSurface *surface : dirtySurfaces()) {
        if (!surface->currentFilePath().isEmpty()) {
            paths.append(QFileInfo(surface->currentFilePath()).absoluteFilePath());
        }
    }
    return paths;
}

void EditorTabsController::discardUntitledDrafts()
{
    if (!editorTabs) {
        return;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (!surface || !surface->isDirty() || !surface->currentFilePath().isEmpty()) {
            continue;
        }

        surface->clear();
        surface->document()->setModified(false);
    }
}

void EditorTabsController::closeTabsMatching(const std::function<bool(EditorSurface *)> &matches)
{
    if (!editorTabs || !matches) {
        return;
    }

    for (int i = editorTabs->count() - 1; i >= 0; --i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (surface && matches(surface)) {
            closeTab(i);
        }
    }
}

EditorSurface *EditorTabsController::createSurface(const QString &title, DocumentId id, bool emitCurrentEditor)
{
    auto *surface = new EditorSurface(editorTabs);
    setDocumentIdForSurface(surface, id);
    applyFontToSurface(surface);
    applyWorkspaceSettingsToSurface(surface);
    int index = -1;
    {
        const QSignalBlocker blocker(editorTabs);
        index = editorTabs->addTab(surface, title);
        editorTabs->setCurrentIndex(index);
    }
    workbenchState.addEditorSession(surface->currentFilePath());
    activateSurface(surface, true, emitCurrentEditor);

    connect(surface, &EditorSurface::filePathChanged, this, [this, surface](const QString &path) {
        const DocumentId id = documentIdForSurface(surface);
        syncEditorSession(surface);
        updateEditorTabTitle(surface);
        emit pathChanged(id, path);
    });
    connect(surface, &EditorSurface::dirtyStateChanged, this, [this, surface](bool dirty) {
        syncDocumentRegistryFromSurface(surface);
        syncEditorSession(surface);
        updateEditorTabTitle(surface);
        emit dirtyStateChanged(documentIdForSurface(surface), dirty);
    });
    connect(surface->document(), &QTextDocument::contentsChanged, this, [this, surface]() {
        syncDocumentRegistryFromSurface(surface);
    });

    updateEditorTabTitle(surface);
    emit tabAdded(surface, id);
    return surface;
}

void EditorTabsController::activateSurface(EditorSurface *surface, bool forceSignal, bool emitSignal)
{
    if (!surface) {
        return;
    }

    const bool changed = currentEditor != surface;
    const int currentIndex = editorTabs ? editorTabs->indexOf(surface) : -1;
    workbenchState.setCurrentEditorSessionIndex(currentIndex);
    syncEditorSession(surface);

    for (int i = 0; editorTabs && i < editorTabs->count(); ++i) {
        if (auto *tabEditor = qobject_cast<EditorSurface *>(editorTabs->widget(i))) {
            tabEditor->setObjectName(tabEditor == surface ? QStringLiteral("editorSurface") : QStringLiteral("editorSurfaceInactive"));
        }
    }

    currentEditor = surface;
    if (emitSignal && (changed || forceSignal)) {
        emit currentEditorChanged(surface);
    }
}

void EditorTabsController::setDocumentIdForSurface(EditorSurface *surface, DocumentId id)
{
    if (!surface) {
        return;
    }

    if (!id.isValid()) {
        surface->setProperty(DocumentIdProperty, QVariant());
        return;
    }
    surface->setProperty(DocumentIdProperty, id.value());
}

void EditorTabsController::syncDocumentRegistryFromSurface(EditorSurface *surface)
{
    const DocumentId id = documentIdForSurface(surface);
    if (!id.isValid()) {
        return;
    }

    DocumentRecord record = documentRegistry.document(id);
    if (!record.id.isValid()) {
        return;
    }

    const QString text = surface->toPlainText();
    if (record.text != text) {
        documentRegistry.setText(id, text);
        record = documentRegistry.document(id);
    }
    if (!surface->isDirty() && record.dirty) {
        documentRegistry.markClean(id);
    }
}

void EditorTabsController::markDocumentSaved(EditorSurface *surface)
{
    if (!surface) {
        return;
    }

    DocumentId id = documentIdForSurface(surface);
    if (!id.isValid()) {
        id = documentRegistry.createUntitled(surface->toPlainText());
        setDocumentIdForSurface(surface, id);
    }

    const DocumentRecord record = documentRegistry.document(id);
    const QString text = surface->toPlainText();
    if (record.id.isValid() && record.text != text) {
        documentRegistry.setText(id, text);
    }

    if (!surface->currentFilePath().isEmpty()) {
        documentRegistry.setPathAfterSave(id, surface->currentFilePath());
        emit pathChanged(id, surface->currentFilePath());
    } else {
        documentRegistry.markClean(id);
    }
    syncEditorSession(surface);
    updateEditorTabTitle(surface);
    emit dirtyStateChanged(id, false);
}

void EditorTabsController::syncEditorSession(EditorSurface *surface)
{
    if (!surface || !editorTabs) {
        return;
    }

    const int index = editorTabs->indexOf(surface);
    if (index < 0) {
        return;
    }

    const DocumentRecord record = documentRegistry.document(documentIdForSurface(surface));
    workbenchState.setEditorSessionPath(index, record.id.isValid() ? record.path : surface->currentFilePath());
    workbenchState.setEditorSessionDirty(index, record.id.isValid() ? record.dirty : surface->isDirty());
}

void EditorTabsController::updateEditorTabTitle(EditorSurface *surface)
{
    if (!surface || !editorTabs) {
        return;
    }

    const int index = editorTabs->indexOf(surface);
    if (index < 0) {
        return;
    }

    syncEditorSession(surface);
    const auto sessions = workbenchState.editorSessions();
    const EditorSessionState session = index < sessions.size() ? sessions.at(index) : EditorSessionState {};

    QString title = session.path.isEmpty()
        ? QString::fromUtf8("ملف جديد")
        : QFileInfo(session.path).fileName();
    if (session.dirty) {
        title.prepend(QLatin1Char('*'));
    }
    editorTabs->setTabText(index, title);
    emit titleChanged(surface, title);
}

void EditorTabsController::applyFontToSurface(EditorSurface *surface) const
{
    if (!surface || !hasEditorFont) {
        return;
    }

    surface->setFont(editorFont);
    QString stylesheetFamily = editorFont.family();
    stylesheetFamily.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    stylesheetFamily.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    surface->setStyleSheet(QStringLiteral("font-family: \"%1\"; font-size: %2pt;")
        .arg(stylesheetFamily)
        .arg(editorFont.pointSize()));
    surface->setTabStopDistance(surface->fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
}

void EditorTabsController::applyWorkspaceSettingsToSurface(EditorSurface *surface) const
{
    if (!surface) {
        return;
    }

    surface->setTrimTrailingWhitespaceOnSave(workspaceSettings.trimTrailingWhitespaceOnSave);
}
