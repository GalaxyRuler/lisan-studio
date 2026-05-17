#include "DocumentRegistry.h"

#include <QDir>
#include <QFileInfo>

DocumentId::DocumentId(int value)
    : raw(value)
{
}

bool DocumentId::isValid() const
{
    return raw > 0;
}

int DocumentId::value() const
{
    return raw;
}

DocumentId DocumentRegistry::openPath(const QString &path, QString *error)
{
    if (error) {
        error->clear();
    }

    const DocumentId existing = findByPath(path);
    if (existing.isValid()) {
        return existing;
    }

    const DocumentLoadResult loaded = DocumentFileIO::loadUtf8(path, error);
    if (error && !error->isEmpty()) {
        return {};
    }

    DocumentRecord record;
    record.id = DocumentId(nextId++);
    record.path = loaded.identity.path;
    record.text = loaded.text;
    record.encoding = loaded.encoding;
    record.lineEnding = loaded.lineEnding;
    record.identity = loaded.identity;
    records.push_back(record);
    return record.id;
}

DocumentId DocumentRegistry::createUntitled(const QString &text)
{
    DocumentRecord record;
    record.id = DocumentId(nextId++);
    record.text = text;
    record.dirty = !text.isEmpty();
    record.lineEnding = DocumentFileIO::detectLineEnding(text);
    records.push_back(record);
    return record.id;
}

bool DocumentRegistry::close(DocumentId id)
{
    const int index = indexOf(id);
    if (index < 0) {
        return false;
    }

    records.removeAt(index);
    return true;
}

int DocumentRegistry::documentCount() const
{
    return records.size();
}

DocumentRecord DocumentRegistry::document(DocumentId id) const
{
    const int index = indexOf(id);
    return index < 0 ? DocumentRecord {} : records.at(index);
}

DocumentId DocumentRegistry::findByPath(const QString &path) const
{
    const QString target = normalizedPath(path);
    if (target.isEmpty()) {
        return {};
    }

    for (const DocumentRecord &record : records) {
        if (normalizedPath(record.path) == target) {
            return record.id;
        }
    }
    return {};
}

void DocumentRegistry::setText(DocumentId id, const QString &text)
{
    const int index = indexOf(id);
    if (index < 0) {
        return;
    }

    records[index].text = text;
    records[index].dirty = true;
    records[index].lineEnding = DocumentFileIO::detectLineEnding(text);
}

void DocumentRegistry::markClean(DocumentId id)
{
    const int index = indexOf(id);
    if (index < 0) {
        return;
    }

    records[index].dirty = false;
    records[index].externalState = DocumentExternalState::Unchanged;
}

void DocumentRegistry::setPathAfterSave(DocumentId id, const QString &path)
{
    const int index = indexOf(id);
    if (index < 0) {
        return;
    }

    records[index].path = DocumentFileIO::identityForPath(path).path;
    records[index].identity = DocumentFileIO::identityForPath(path);
    records[index].dirty = false;
    records[index].externalState = DocumentExternalState::Unchanged;
}

void DocumentRegistry::refreshFileState(DocumentId id)
{
    const int index = indexOf(id);
    if (index < 0) {
        return;
    }

    DocumentRecord &record = records[index];
    if (record.path.isEmpty()) {
        record.externalState = DocumentExternalState::Unchanged;
        return;
    }

    const DocumentFileIdentity current = DocumentFileIO::identityForPath(record.path);
    if (current.sizeBytes < 0 || !current.lastModifiedUtc.isValid()) {
        record.externalState = DocumentExternalState::Deleted;
        return;
    }

    record.externalState = (current.sizeBytes != record.identity.sizeBytes
            || current.lastModifiedUtc != record.identity.lastModifiedUtc)
        ? DocumentExternalState::Modified
        : DocumentExternalState::Unchanged;
}

void DocumentRegistry::refreshAllFileStates()
{
    const QVector<DocumentRecord> snapshot = records;
    for (const DocumentRecord &record : snapshot) {
        refreshFileState(record.id);
    }
}

QVector<DocumentRecord> DocumentRegistry::externallyChangedDocuments() const
{
    QVector<DocumentRecord> changed;
    for (const DocumentRecord &record : records) {
        if (record.externalState != DocumentExternalState::Unchanged) {
            changed.push_back(record);
        }
    }
    return changed;
}

bool DocumentRegistry::reloadFromDisk(DocumentId id, QString *error)
{
    if (error) {
        error->clear();
    }

    const int index = indexOf(id);
    if (index < 0) {
        if (error) {
            *error = QStringLiteral("Unknown document.");
        }
        return false;
    }

    DocumentRecord &record = records[index];
    if (record.path.isEmpty()) {
        if (error) {
            *error = QStringLiteral("Untitled documents cannot be reloaded from disk.");
        }
        return false;
    }

    const DocumentLoadResult loaded = DocumentFileIO::loadUtf8(record.path, error);
    if (error && !error->isEmpty()) {
        return false;
    }

    record.text = loaded.text;
    record.encoding = loaded.encoding;
    record.lineEnding = loaded.lineEnding;
    record.identity = loaded.identity;
    record.dirty = false;
    record.externalState = DocumentExternalState::Unchanged;
    return true;
}

bool DocumentRegistry::keepCurrentVersion(DocumentId id)
{
    const int index = indexOf(id);
    if (index < 0 || records[index].path.isEmpty()) {
        return false;
    }

    records[index].identity = DocumentFileIO::identityForPath(records[index].path);
    records[index].dirty = true;
    records[index].externalState = DocumentExternalState::Unchanged;
    return true;
}

int DocumentRegistry::indexOf(DocumentId id) const
{
    for (int i = 0; i < records.size(); ++i) {
        if (records.at(i).id == id) {
            return i;
        }
    }
    return -1;
}

QString DocumentRegistry::normalizedPath(const QString &path)
{
    if (path.isEmpty()) {
        return QString();
    }

    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}
