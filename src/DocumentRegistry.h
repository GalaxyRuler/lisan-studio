#pragma once

#include "DocumentFileIO.h"

#include <QString>
#include <QUrl>
#include <QVector>

class DocumentId final
{
public:
    DocumentId() = default;
    explicit DocumentId(int value);

    bool isValid() const;
    int value() const;

    friend bool operator==(DocumentId left, DocumentId right) { return left.raw == right.raw; }
    friend bool operator!=(DocumentId left, DocumentId right) { return !(left == right); }

private:
    int raw = -1;
};

enum class DocumentExternalState
{
    Unchanged,
    Modified,
    Deleted
};

struct DocumentRecord
{
    DocumentId id;
    QUrl uri;
    QString path;
    QString text;
    bool dirty = false;
    int version = 0;
    DocumentEncoding encoding = DocumentEncoding::Utf8;
    DocumentLineEnding lineEnding = DocumentLineEnding::None;
    DocumentFileIdentity identity;
    DocumentExternalState externalState = DocumentExternalState::Unchanged;
};

struct DocumentTextEdit
{
    int expectedVersion = 0;
    int start = 0;
    int length = 0;
    QString replacement;
};

class DocumentRegistry final
{
public:
    DocumentId openPath(const QString &path, QString *error = nullptr);
    DocumentId createUntitled(const QString &text = QString());
    bool close(DocumentId id);
    int documentCount() const;
    DocumentRecord document(DocumentId id) const;
    DocumentId findByPath(const QString &path) const;
    void setText(DocumentId id, const QString &text);
    void markClean(DocumentId id);
    void setPathAfterSave(DocumentId id, const QString &path);
    void refreshFileState(DocumentId id);
    void refreshAllFileStates();
    QVector<DocumentRecord> externallyChangedDocuments() const;
    bool reloadFromDisk(DocumentId id, QString *error = nullptr);
    bool keepCurrentVersion(DocumentId id);
    bool applyTextEdit(DocumentId id, const DocumentTextEdit &edit, QString *error = nullptr);
    bool applyTextEdits(DocumentId id, const QVector<DocumentTextEdit> &edits, QString *error = nullptr);

private:
    QVector<DocumentRecord> records;
    int nextId = 1;

    int indexOf(DocumentId id) const;
    static QString normalizedPath(const QString &path);
};
