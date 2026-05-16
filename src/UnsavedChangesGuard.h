#pragma once

#include "DocumentRegistry.h"

#include <QString>
#include <QVector>

enum class UnsavedChangesOperation
{
    CloseDocument,
    OpenFile,
    ProjectSwitch,
    Run,
    Exit
};

enum class UnsavedChangesChoice
{
    Save,
    Discard,
    Cancel
};

struct UnsavedChangesRequest
{
    bool required = false;
    UnsavedChangesOperation operation = UnsavedChangesOperation::CloseDocument;
    QVector<DocumentRecord> dirtyDocuments;
    QString title;
    QString message;
};

class UnsavedChangesGuard final
{
public:
    static UnsavedChangesRequest requestFor(
        const QVector<DocumentRecord> &documents,
        UnsavedChangesOperation operation);
    static bool allowsOperation(UnsavedChangesChoice choice);
};
