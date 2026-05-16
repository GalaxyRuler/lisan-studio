#pragma once

#include "RuntimeRunner.h"

#include <QVector>

struct RuntimeHistoryEntry
{
    RuntimeAction action = RuntimeAction::Run;
    QString title;
    QString filePath;
    QString workingDirectory;
    RuntimeCommand command;
};

class RuntimeHistory final
{
public:
    explicit RuntimeHistory(int maxEntries = 20);

    void recordLaunch(const RuntimeLaunchPlan &plan);
    QVector<RuntimeHistoryEntry> entries() const;
    void clear();

private:
    int capacity = 20;
    QVector<RuntimeHistoryEntry> recordedEntries;
};
