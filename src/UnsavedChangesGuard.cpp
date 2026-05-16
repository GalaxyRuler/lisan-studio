#include "UnsavedChangesGuard.h"

#include <QFileInfo>
#include <QStringList>

namespace {
QString operationTitle(UnsavedChangesOperation operation)
{
    switch (operation) {
    case UnsavedChangesOperation::CloseDocument:
        return QString::fromUtf8("حفظ التغييرات");
    case UnsavedChangesOperation::OpenFile:
        return QString::fromUtf8("حفظ قبل فتح ملف");
    case UnsavedChangesOperation::ProjectSwitch:
        return QString::fromUtf8("حفظ قبل تغيير المشروع");
    case UnsavedChangesOperation::Run:
        return QString::fromUtf8("حفظ قبل التشغيل");
    case UnsavedChangesOperation::Exit:
        return QString::fromUtf8("حفظ قبل الخروج");
    }
    return QString::fromUtf8("حفظ التغييرات");
}

QString displayName(const DocumentRecord &document)
{
    if (!document.path.isEmpty()) {
        return QFileInfo(document.path).fileName();
    }
    return QString::fromUtf8("ملف جديد");
}
}

UnsavedChangesRequest UnsavedChangesGuard::requestFor(
    const QVector<DocumentRecord> &documents,
    UnsavedChangesOperation operation)
{
    UnsavedChangesRequest request;
    request.operation = operation;
    request.title = operationTitle(operation);

    for (const DocumentRecord &document : documents) {
        if (document.dirty) {
            request.dirtyDocuments.push_back(document);
        }
    }

    request.required = !request.dirtyDocuments.isEmpty();
    if (request.required) {
        QStringList names;
        for (const DocumentRecord &document : request.dirtyDocuments) {
            names.append(displayName(document));
        }
        request.message = QString::fromUtf8("هناك تغييرات غير محفوظة في: %1").arg(names.join(QStringLiteral(", ")));
    }
    return request;
}

bool UnsavedChangesGuard::allowsOperation(UnsavedChangesChoice choice)
{
    return choice == UnsavedChangesChoice::Save || choice == UnsavedChangesChoice::Discard;
}
