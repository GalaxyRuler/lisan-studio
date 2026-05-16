#include "RuntimeHistory.h"

#include <algorithm>

RuntimeHistory::RuntimeHistory(int maxEntries)
    : capacity(std::max(1, maxEntries))
{
}

void RuntimeHistory::recordLaunch(const RuntimeLaunchPlan &plan)
{
    RuntimeHistoryEntry entry;
    entry.action = plan.action;
    entry.title = plan.title;
    entry.filePath = plan.filePath;
    entry.workingDirectory = plan.command.workingDirectory;
    entry.command = plan.command;
    entry.reloadAfterSuccess = plan.reloadAfterSuccess;

    recordedEntries.prepend(entry);
    while (recordedEntries.size() > capacity) {
        recordedEntries.removeLast();
    }
}

QVector<RuntimeHistoryEntry> RuntimeHistory::entries() const
{
    return recordedEntries;
}

void RuntimeHistory::clear()
{
    recordedEntries.clear();
}
