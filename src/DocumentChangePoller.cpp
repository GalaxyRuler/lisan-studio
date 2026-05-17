#include "DocumentChangePoller.h"

DocumentChangePoller::DocumentChangePoller(DocumentRegistry *documentRegistry)
    : registry(documentRegistry)
{
}

DocumentChangeSnapshot DocumentChangePoller::poll()
{
    DocumentChangeSnapshot snapshot;
    if (!registry) {
        return snapshot;
    }

    registry->refreshAllFileStates();
    snapshot.changedDocuments = registry->externallyChangedDocuments();
    return snapshot;
}
