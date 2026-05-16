#pragma once

#include "RuntimeRunner.h"

#include <QString>
#include <QVector>

struct RuntimeRunConfiguration
{
    QString id;
    QString name;
    RuntimeAction action = RuntimeAction::Run;
    QString filePath;
    QString workingDirectory;
    bool reloadAfterSuccess = false;
};

class RuntimeRunConfigurationModel final
{
public:
    static RuntimeRunConfiguration currentFileRunConfiguration(const QString &filePath, const QString &workingDirectory);

    bool addConfiguration(const RuntimeRunConfiguration &configuration, QString *error = nullptr);
    bool removeConfiguration(const QString &id);
    const RuntimeRunConfiguration *findById(const QString &id) const;
    QVector<RuntimeRunConfiguration> configurations() const;

private:
    QVector<RuntimeRunConfiguration> entries;
};
