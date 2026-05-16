#include "RuntimeRunConfigurationStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace
{
QString actionToString(RuntimeAction action)
{
    switch (action) {
    case RuntimeAction::Run:
        return QStringLiteral("run");
    case RuntimeAction::Lint:
        return QStringLiteral("lint");
    case RuntimeAction::Format:
        return QStringLiteral("format");
    }
    return QStringLiteral("run");
}

RuntimeAction actionFromString(const QString &text)
{
    if (text == QStringLiteral("lint")) {
        return RuntimeAction::Lint;
    }
    if (text == QStringLiteral("format")) {
        return RuntimeAction::Format;
    }
    return RuntimeAction::Run;
}

QJsonObject toJson(const RuntimeRunConfiguration &configuration)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), configuration.id);
    object.insert(QStringLiteral("name"), configuration.name);
    object.insert(QStringLiteral("action"), actionToString(configuration.action));
    object.insert(QStringLiteral("filePath"), QDir::fromNativeSeparators(configuration.filePath));
    object.insert(QStringLiteral("workingDirectory"), QDir::fromNativeSeparators(configuration.workingDirectory));
    object.insert(QStringLiteral("reloadAfterSuccess"), configuration.reloadAfterSuccess);
    return object;
}

RuntimeRunConfiguration fromJson(const QJsonObject &object)
{
    RuntimeRunConfiguration configuration;
    configuration.id = object.value(QStringLiteral("id")).toString();
    configuration.name = object.value(QStringLiteral("name")).toString();
    configuration.action = actionFromString(object.value(QStringLiteral("action")).toString());
    configuration.filePath = QDir::fromNativeSeparators(object.value(QStringLiteral("filePath")).toString());
    configuration.workingDirectory = QDir::fromNativeSeparators(object.value(QStringLiteral("workingDirectory")).toString());
    configuration.reloadAfterSuccess = object.value(QStringLiteral("reloadAfterSuccess")).toBool(false);
    return configuration;
}
}

RuntimeRunConfigurationStore::RuntimeRunConfigurationStore(const QString &workspaceRoot)
    : rootPath(QDir(workspaceRoot).absolutePath())
{
}

QString RuntimeRunConfigurationStore::configurationsFilePath() const
{
    return QDir(rootPath).filePath(QStringLiteral(".lisan-workspace/run-configurations.json"));
}

QVector<RuntimeRunConfiguration> RuntimeRunConfigurationStore::load(QString *error) const
{
    if (error) {
        error->clear();
    }

    QFile file(configurationsFilePath());
    if (!file.exists()) {
        return {};
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) {
            *error = file.errorString();
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) {
            *error = parseError.errorString();
        }
        return {};
    }

    QVector<RuntimeRunConfiguration> configurations;
    const QJsonArray items = document.object().value(QStringLiteral("configurations")).toArray();
    configurations.reserve(items.size());
    for (const QJsonValue &item : items) {
        if (item.isObject()) {
            configurations.push_back(fromJson(item.toObject()));
        }
    }
    return configurations;
}

bool RuntimeRunConfigurationStore::save(const QVector<RuntimeRunConfiguration> &configurations, QString *error) const
{
    if (error) {
        error->clear();
    }

    const QFileInfo info(configurationsFilePath());
    if (!QDir().mkpath(info.absolutePath())) {
        if (error) {
            *error = QString::fromUtf8("تعذر إنشاء مجلد إعدادات مساحة العمل.");
        }
        return false;
    }

    QJsonArray items;
    for (const RuntimeRunConfiguration &configuration : configurations) {
        items.append(toJson(configuration));
    }

    QJsonObject root;
    root.insert(QStringLiteral("configurations"), items);

    QSaveFile file(configurationsFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}
