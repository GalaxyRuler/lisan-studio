#pragma once

#include "CommandRegistry.h"

#include <QJsonObject>
#include <QMap>

class ShortcutSettingsModel final
{
public:
    bool setOverride(const CommandRegistry &registry, const QString &commandId, const QKeySequence &shortcut, QString *error = nullptr);
    QKeySequence effectiveShortcut(const CommandRegistry &registry, const QString &commandId) const;
    QJsonObject toJson() const;
    bool loadJson(const QJsonObject &object, const CommandRegistry &registry, QString *error = nullptr);

private:
    QMap<QString, QKeySequence> shortcutOverrides;

    bool shortcutConflicts(const CommandRegistry &registry, const QString &commandId, const QKeySequence &shortcut, QString *conflictingCommandId) const;
};
