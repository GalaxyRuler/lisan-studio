#include "CommandPaletteModel.h"

#include <QKeySequence>

namespace {
QString normalized(QString text)
{
    return text.trimmed().toCaseFolded();
}
}

QVector<CommandPaletteRow> CommandPaletteModel::rows(
    const CommandRegistry &registry,
    const ShortcutSettingsModel *shortcutSettings)
{
    QVector<CommandPaletteRow> paletteRows;
    for (const CommandDefinition &command : registry.commands()) {
        const QKeySequence shortcut = shortcutSettings
            ? shortcutSettings->effectiveShortcut(registry, command.id)
            : command.defaultShortcut;

        CommandPaletteRow row;
        row.id = command.id;
        row.title = command.title;
        row.shortcutText = shortcut.toString(QKeySequence::NativeText);
        row.keywords = command.keywords;
        paletteRows.push_back(row);
    }
    return paletteRows;
}

bool CommandPaletteModel::matchesQuery(const CommandPaletteRow &row, const QString &query)
{
    const QString normalizedQuery = normalized(query);
    if (normalizedQuery.isEmpty()) {
        return true;
    }

    const QString haystack = normalized(
        row.title
        + QLatin1Char(' ')
        + row.shortcutText
        + QLatin1Char(' ')
        + row.keywords);
    return haystack.contains(normalizedQuery);
}
