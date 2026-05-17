#pragma once

#include "DocumentRegistry.h"

struct DocumentChangeSnapshot
{
    QVector<DocumentRecord> changedDocuments;

    bool hasChanges() const { return !changedDocuments.isEmpty(); }
};

class DocumentChangePoller final
{
public:
    explicit DocumentChangePoller(DocumentRegistry *registry);

    DocumentChangeSnapshot poll();

private:
    DocumentRegistry *registry = nullptr;
};
