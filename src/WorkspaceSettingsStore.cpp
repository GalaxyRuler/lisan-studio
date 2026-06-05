#include "WorkspaceSettingsStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

WorkspaceSettingsStore::WorkspaceSettingsStore(const QString &workspaceRoot)
    : rootPath(QDir(workspaceRoot).absolutePath())
{
}

QString WorkspaceSettingsStore::settingsFilePath() const
{
    return QDir(rootPath).filePath(QStringLiteral(".lisan-workspace/settings.json"));
}

QString WorkspaceSettingsStore::trustAuditFilePath() const
{
    return QDir(rootPath).filePath(QStringLiteral(".lisan-workspace/trust-audit.jsonl"));
}

WorkspaceSettings WorkspaceSettingsStore::load(QString *error) const
{
    if (error) {
        error->clear();
    }

    WorkspaceSettings settings;
    QFile file(settingsFilePath());
    if (!file.exists()) {
        return settings;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) {
            *error = file.errorString();
        }
        return settings;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) {
            *error = parseError.errorString();
        }
        return settings;
    }

    const QJsonObject root = document.object();
    settings.trusted = root.value(QStringLiteral("trusted")).toBool(false);

    const QJsonObject editor = root.value(QStringLiteral("editor")).toObject();
    settings.trimTrailingWhitespaceOnSave = editor.value(QStringLiteral("trimTrailingWhitespaceOnSave")).toBool(false);

    const QJsonObject runtime = root.value(QStringLiteral("runtime")).toObject();
    settings.defaultRunWorkingDirectory = runtime.value(QStringLiteral("defaultRunWorkingDirectory")).toString();
    settings.defaultRunWorkingDirectory = QDir::fromNativeSeparators(settings.defaultRunWorkingDirectory);

    const QJsonObject terminal = root.value(QStringLiteral("terminal")).toObject();
    settings.terminalProfileId = terminal.value(QStringLiteral("defaultProfileId")).toString();
    return settings;
}

bool WorkspaceSettingsStore::save(const WorkspaceSettings &settings, QString *error) const
{
    if (error) {
        error->clear();
    }

    const QFileInfo info(settingsFilePath());
    if (!QDir().mkpath(info.absolutePath())) {
        if (error) {
            *error = QString::fromUtf8("تعذر إنشاء مجلد إعدادات مساحة العمل.");
        }
        return false;
    }

    QJsonObject editor;
    editor.insert(QStringLiteral("trimTrailingWhitespaceOnSave"), settings.trimTrailingWhitespaceOnSave);

    QJsonObject runtime;
    runtime.insert(QStringLiteral("defaultRunWorkingDirectory"), QDir::fromNativeSeparators(settings.defaultRunWorkingDirectory));

    QJsonObject terminal;
    terminal.insert(QStringLiteral("defaultProfileId"), settings.terminalProfileId);

    QJsonObject root;
    root.insert(QStringLiteral("trusted"), settings.trusted);
    root.insert(QStringLiteral("editor"), editor);
    root.insert(QStringLiteral("runtime"), runtime);
    root.insert(QStringLiteral("terminal"), terminal);

    QSaveFile file(settingsFilePath());
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
