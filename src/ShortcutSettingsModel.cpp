#include "ShortcutSettingsModel.h"

#include <QJsonValue>

namespace {
QString migratedCommandId(QString commandId)
{
    static const QMap<QString, QString> migrations = {
        {QStringLiteral("new-file"), QStringLiteral("file.new")},
        {QStringLiteral("open-file"), QStringLiteral("file.open")},
        {QStringLiteral("open-project"), QStringLiteral("project.open")},
        {QStringLiteral("save-file"), QStringLiteral("file.save")},
        {QStringLiteral("save-as"), QStringLiteral("file.saveAs")},
        {QStringLiteral("find-in-file"), QStringLiteral("editor.findInFile")},
        {QStringLiteral("run-current-file"), QStringLiteral("run.currentFile")},
        {QStringLiteral("lint-current-file"), QStringLiteral("run.lintCurrentFile")},
        {QStringLiteral("format-current-file"), QStringLiteral("run.formatCurrentFile")},
        {QStringLiteral("stop-run"), QStringLiteral("run.stop")},
        {QStringLiteral("search-project"), QStringLiteral("search.project")},
        {QStringLiteral("replace-in-project"), QStringLiteral("search.replacePreview")},
        {QStringLiteral("replace-in-project.applyAccepted"), QStringLiteral("search.replaceApplyAccepted")},
        {QStringLiteral("settings"), QStringLiteral("system.settings")},
        {QStringLiteral("command-palette"), QStringLiteral("system.commandPalette")},
    };
    return migrations.value(commandId, commandId);
}
}

bool ShortcutSettingsModel::setOverride(const CommandRegistry &registry, const QString &commandId, const QKeySequence &shortcut, QString *error)
{
    if (!registry.contains(commandId)) {
        if (error) {
            *error = QStringLiteral("Unknown command id: %1").arg(commandId);
        }
        return false;
    }
    if (shortcut.isEmpty()) {
        if (error) {
            *error = QStringLiteral("Shortcut is empty for command id: %1").arg(commandId);
        }
        return false;
    }

    QString conflictingCommandId;
    if (shortcutConflicts(registry, commandId, shortcut, &conflictingCommandId)) {
        if (error) {
            *error = QStringLiteral("Shortcut %1 already belongs to %2")
                .arg(shortcut.toString(QKeySequence::PortableText), conflictingCommandId);
        }
        return false;
    }

    shortcutOverrides.insert(commandId, shortcut);
    if (error) {
        error->clear();
    }
    return true;
}

QKeySequence ShortcutSettingsModel::effectiveShortcut(const CommandRegistry &registry, const QString &commandId) const
{
    const auto override = shortcutOverrides.constFind(commandId);
    if (override != shortcutOverrides.cend()) {
        return override.value();
    }

    const QVector<CommandDefinition> commands = registry.commands();
    for (const CommandDefinition &command : commands) {
        if (command.id == commandId) {
            return command.defaultShortcut;
        }
    }
    return QKeySequence();
}

QJsonObject ShortcutSettingsModel::toJson() const
{
    QJsonObject shortcuts;
    for (auto it = shortcutOverrides.cbegin(); it != shortcutOverrides.cend(); ++it) {
        shortcuts.insert(it.key(), it.value().toString(QKeySequence::PortableText));
    }

    QJsonObject object;
    object.insert(QStringLiteral("version"), 1);
    object.insert(QStringLiteral("shortcuts"), shortcuts);
    return object;
}

bool ShortcutSettingsModel::loadJson(const QJsonObject &object, const CommandRegistry &registry, QString *error)
{
    const QJsonValue shortcutsValue = object.value(QStringLiteral("shortcuts"));
    if (!shortcutsValue.isObject()) {
        if (error) {
            *error = QStringLiteral("Shortcut settings missing shortcuts object");
        }
        return false;
    }

    ShortcutSettingsModel loaded;
    const QJsonObject shortcuts = shortcutsValue.toObject();
    for (auto it = shortcuts.constBegin(); it != shortcuts.constEnd(); ++it) {
        const QKeySequence shortcut(it.value().toString());
        if (!loaded.setOverride(registry, migratedCommandId(it.key()), shortcut, error)) {
            return false;
        }
    }

    shortcutOverrides = loaded.shortcutOverrides;
    if (error) {
        error->clear();
    }
    return true;
}

bool ShortcutSettingsModel::shortcutConflicts(const CommandRegistry &registry, const QString &commandId, const QKeySequence &shortcut, QString *conflictingCommandId) const
{
    const QVector<CommandDefinition> commands = registry.commands();
    for (const CommandDefinition &command : commands) {
        if (command.id == commandId) {
            continue;
        }

        const QKeySequence otherShortcut = effectiveShortcut(registry, command.id);
        if (!otherShortcut.isEmpty() && otherShortcut == shortcut) {
            if (conflictingCommandId) {
                *conflictingCommandId = command.id;
            }
            return true;
        }
    }
    return false;
}
