#include <QtTest/QtTest>

#include "CommandRegistry.h"

class TestCommandRegistry : public QObject
{
    Q_OBJECT

private slots:
    void registersMetadataAndTriggersEnabledCommand();
    void rejectsDuplicateCommandIds();
    void disabledCommandDoesNotTrigger();
};

static CommandDefinition makeCommand(const QString &id, bool *triggered, bool enabled = true)
{
    CommandDefinition command;
    command.id = id;
    command.title = QString::fromUtf8("ملف جديد");
    command.category = QString::fromUtf8("ملف");
    command.defaultShortcut = QKeySequence::New;
    command.keywords = QString::fromUtf8("ملف جديد new file");
    command.isEnabled = [enabled]() { return enabled; };
    command.trigger = [triggered]() { *triggered = true; };
    return command;
}

void TestCommandRegistry::registersMetadataAndTriggersEnabledCommand()
{
    bool triggered = false;
    CommandRegistry registry;
    QVERIFY(registry.registerCommand(makeCommand(QStringLiteral("new-file"), &triggered)));

    const auto commands = registry.commands();
    QCOMPARE(commands.size(), 1);
    QCOMPARE(commands.first().id, QStringLiteral("new-file"));
    QCOMPARE(commands.first().title, QString::fromUtf8("ملف جديد"));
    QCOMPARE(commands.first().category, QString::fromUtf8("ملف"));
    QCOMPARE(commands.first().defaultShortcut, QKeySequence(QKeySequence::New));
    QCOMPARE(commands.first().keywords, QString::fromUtf8("ملف جديد new file"));

    QVERIFY(registry.contains(QStringLiteral("new-file")));
    QVERIFY(registry.isCommandEnabled(QStringLiteral("new-file")));
    QVERIFY(registry.triggerCommand(QStringLiteral("new-file")));
    QVERIFY(triggered);
}

void TestCommandRegistry::rejectsDuplicateCommandIds()
{
    bool firstTriggered = false;
    bool secondTriggered = false;
    CommandRegistry registry;
    QVERIFY(registry.registerCommand(makeCommand(QStringLiteral("new-file"), &firstTriggered)));
    QVERIFY(!registry.registerCommand(makeCommand(QStringLiteral("new-file"), &secondTriggered)));

    QCOMPARE(registry.commands().size(), 1);
    QVERIFY(registry.triggerCommand(QStringLiteral("new-file")));
    QVERIFY(firstTriggered);
    QVERIFY(!secondTriggered);
}

void TestCommandRegistry::disabledCommandDoesNotTrigger()
{
    bool triggered = false;
    CommandRegistry registry;
    QVERIFY(registry.registerCommand(makeCommand(QStringLiteral("run-current-file"), &triggered, false)));

    QVERIFY(registry.contains(QStringLiteral("run-current-file")));
    QVERIFY(!registry.isCommandEnabled(QStringLiteral("run-current-file")));
    QVERIFY(!registry.triggerCommand(QStringLiteral("run-current-file")));
    QVERIFY(!triggered);
}

QTEST_MAIN(TestCommandRegistry)
#include "TestCommandRegistry.moc"
