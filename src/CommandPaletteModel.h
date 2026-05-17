#pragma once

#include "CommandRegistry.h"
#include "ShortcutSettingsModel.h"

#include <QString>
#include <QVector>

struct CommandPaletteRow
{
    QString id;
    QString title;
    QString shortcutText;
    QString keywords;
};

class CommandPaletteModel final
{
public:
    static QVector<CommandPaletteRow> rows(
        const CommandRegistry &registry,
        const ShortcutSettingsModel *shortcutSettings = nullptr);
    static bool matchesQuery(const CommandPaletteRow &row, const QString &query);
};
