#include "RuntimeRunConfiguration.h"

RuntimeRunConfiguration RuntimeRunConfigurationModel::currentFileRunConfiguration(
    const QString &filePath,
    const QString &workingDirectory)
{
    RuntimeRunConfiguration configuration;
    configuration.id = QStringLiteral("current-file");
    configuration.name = QString::fromUtf8("تشغيل الملف الحالي");
    configuration.action = RuntimeAction::Run;
    configuration.filePath = filePath;
    configuration.workingDirectory = workingDirectory;
    return configuration;
}

bool RuntimeRunConfigurationModel::addConfiguration(const RuntimeRunConfiguration &configuration, QString *error)
{
    if (configuration.id.trimmed().isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("معرف إعداد التشغيل مطلوب.");
        }
        return false;
    }
    if (configuration.name.trimmed().isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("اسم إعداد التشغيل مطلوب.");
        }
        return false;
    }
    if (configuration.filePath.trimmed().isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("ملف التشغيل مطلوب.");
        }
        return false;
    }

    for (const RuntimeRunConfiguration &entry : entries) {
        if (entry.id == configuration.id) {
            if (error) {
                *error = QStringLiteral("duplicate id: %1").arg(configuration.id);
            }
            return false;
        }
        if (entry.name.compare(configuration.name, Qt::CaseInsensitive) == 0) {
            if (error) {
                *error = QString::fromUtf8("الاسم مستخدم: %1").arg(configuration.name);
            }
            return false;
        }
    }

    entries.push_back(configuration);
    if (error) {
        error->clear();
    }
    return true;
}

bool RuntimeRunConfigurationModel::removeConfiguration(const QString &id)
{
    for (int index = 0; index < entries.size(); ++index) {
        if (entries.at(index).id == id) {
            entries.removeAt(index);
            return true;
        }
    }
    return false;
}

const RuntimeRunConfiguration *RuntimeRunConfigurationModel::findById(const QString &id) const
{
    for (const RuntimeRunConfiguration &entry : entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

QVector<RuntimeRunConfiguration> RuntimeRunConfigurationModel::configurations() const
{
    return entries;
}
