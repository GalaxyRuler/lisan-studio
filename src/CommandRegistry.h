#pragma once

#include <QKeySequence>
#include <QString>
#include <QVector>

#include <functional>

struct CommandDefinition
{
    QString id;
    QString title;
    QString category;
    QKeySequence defaultShortcut;
    QString keywords;
    std::function<bool()> isEnabled;
    std::function<void()> trigger;
};

class CommandRegistry final
{
public:
    bool registerCommand(const CommandDefinition &command);
    QVector<CommandDefinition> commands() const;
    bool contains(const QString &id) const;
    bool isCommandEnabled(const QString &id) const;
    bool triggerCommand(const QString &id) const;

private:
    QVector<CommandDefinition> registeredCommands;

    const CommandDefinition *findCommand(const QString &id) const;
};
