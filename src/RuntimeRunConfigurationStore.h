#pragma once

#include "RuntimeRunConfiguration.h"

#include <QString>
#include <QVector>

class RuntimeRunConfigurationStore final
{
public:
    explicit RuntimeRunConfigurationStore(const QString &workspaceRoot);

    QString configurationsFilePath() const;
    QVector<RuntimeRunConfiguration> load(QString *error = nullptr) const;
    bool save(const QVector<RuntimeRunConfiguration> &configurations, QString *error = nullptr) const;

private:
    QString rootPath;
};
