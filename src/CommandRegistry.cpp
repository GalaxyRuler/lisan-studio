#include "CommandRegistry.h"

bool CommandRegistry::registerCommand(const CommandDefinition &command)
{
    if (command.id.trimmed().isEmpty() || contains(command.id)) {
        return false;
    }

    registeredCommands.push_back(command);
    return true;
}

QVector<CommandDefinition> CommandRegistry::commands() const
{
    return registeredCommands;
}

bool CommandRegistry::contains(const QString &id) const
{
    return findCommand(id) != nullptr;
}

bool CommandRegistry::isCommandEnabled(const QString &id) const
{
    const CommandDefinition *command = findCommand(id);
    if (!command) {
        return false;
    }

    return command->isEnabled ? command->isEnabled() : true;
}

bool CommandRegistry::triggerCommand(const QString &id) const
{
    const CommandDefinition *command = findCommand(id);
    if (!command || !isCommandEnabled(id) || !command->trigger) {
        return false;
    }

    command->trigger();
    return true;
}

const CommandDefinition *CommandRegistry::findCommand(const QString &id) const
{
    for (const CommandDefinition &command : registeredCommands) {
        if (command.id == id) {
            return &command;
        }
    }

    return nullptr;
}
