#include <QtTest/QtTest>

#include "CommandPaletteModel.h"
#include "CommandRegistry.h"
#include "ShortcutSettingsModel.h"

#include <QJsonObject>

class TestCommandRegistry : public QObject
{
    Q_OBJECT

private slots:
    void registersMetadataAndTriggersEnabledCommand();
    void rejectsDuplicateCommandIds();
    void disabledCommandDoesNotTrigger();
    void shortcutSettingsRoundTripsOverrides();
    void shortcutSettingsRejectsUnknownCommandsAndConflicts();
    void commandPaletteModelBuildsRowsWithShortcutOverrides();
    void commandPaletteModelFiltersByTitleShortcutAndKeywords();
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

void TestCommandRegistry::shortcutSettingsRoundTripsOverrides()
{
    bool saveTriggered = false;
    bool runTriggered = false;
    CommandRegistry registry;
    QVERIFY(registry.registerCommand(makeCommand(QStringLiteral("save-file"), &saveTriggered)));
    QVERIFY(registry.registerCommand(makeCommand(QStringLiteral("run-current-file"), &runTriggered)));

    ShortcutSettingsModel settings;
    QString error;
    QVERIFY2(settings.setOverride(registry, QStringLiteral("save-file"), QKeySequence(QStringLiteral("Ctrl+Alt+S")), &error), qPrintable(error));

    QCOMPARE(settings.effectiveShortcut(registry, QStringLiteral("save-file")), QKeySequence(QStringLiteral("Ctrl+Alt+S")));
    QCOMPARE(settings.effectiveShortcut(registry, QStringLiteral("run-current-file")), QKeySequence(QKeySequence::New));

    const QJsonObject exported = settings.toJson();
    ShortcutSettingsModel imported;
    QVERIFY2(imported.loadJson(exported, registry, &error), qPrintable(error));

    QCOMPARE(imported.effectiveShortcut(registry, QStringLiteral("save-file")), QKeySequence(QStringLiteral("Ctrl+Alt+S")));
    QCOMPARE(imported.effectiveShortcut(registry, QStringLiteral("run-current-file")), QKeySequence(QKeySequence::New));
}

void TestCommandRegistry::shortcutSettingsRejectsUnknownCommandsAndConflicts()
{
    bool firstTriggered = false;
    bool secondTriggered = false;
    CommandRegistry registry;
    QVERIFY(registry.registerCommand(makeCommand(QStringLiteral("save-file"), &firstTriggered)));
    QVERIFY(registry.registerCommand(makeCommand(QStringLiteral("open-file"), &secondTriggered)));

    ShortcutSettingsModel settings;
    QString error;
    QVERIFY(!settings.setOverride(registry, QStringLiteral("missing-command"), QKeySequence(QStringLiteral("Ctrl+M")), &error));
    QVERIFY2(error.contains(QStringLiteral("missing-command")), qPrintable(error));

    QVERIFY2(settings.setOverride(registry, QStringLiteral("save-file"), QKeySequence(QStringLiteral("Ctrl+Alt+S")), &error), qPrintable(error));
    QVERIFY(!settings.setOverride(registry, QStringLiteral("open-file"), QKeySequence(QStringLiteral("Ctrl+Alt+S")), &error));
    QVERIFY2(error.contains(QStringLiteral("Ctrl+Alt+S")), qPrintable(error));
}

void TestCommandRegistry::commandPaletteModelBuildsRowsWithShortcutOverrides()
{
    bool saveTriggered = false;
    bool runTriggered = false;
    CommandRegistry registry;
    QVERIFY(registry.registerCommand(makeCommand(QStringLiteral("save-file"), &saveTriggered)));
    QVERIFY(registry.registerCommand(makeCommand(QStringLiteral("run-current-file"), &runTriggered)));

    ShortcutSettingsModel settings;
    QString error;
    QVERIFY2(settings.setOverride(registry, QStringLiteral("save-file"), QKeySequence(QStringLiteral("Ctrl+Alt+S")), &error), qPrintable(error));

    const QVector<CommandPaletteRow> rows = CommandPaletteModel::rows(registry, &settings);

    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.first().id, QStringLiteral("save-file"));
    QCOMPARE(rows.first().title, QString::fromUtf8("ملف جديد"));
    QCOMPARE(rows.first().shortcutText, QKeySequence(QStringLiteral("Ctrl+Alt+S")).toString(QKeySequence::NativeText));
    QCOMPARE(rows.first().keywords, QString::fromUtf8("ملف جديد new file"));
    QCOMPARE(rows.last().shortcutText, QKeySequence(QKeySequence::New).toString(QKeySequence::NativeText));
}

void TestCommandRegistry::commandPaletteModelFiltersByTitleShortcutAndKeywords()
{
    CommandPaletteRow row;
    row.id = QStringLiteral("run-current-file");
    row.title = QString::fromUtf8("تشغيل الملف الحالي");
    row.shortcutText = QStringLiteral("F5");
    row.keywords = QStringLiteral("run current file");

    QVERIFY(CommandPaletteModel::matchesQuery(row, QString()));
    QVERIFY(CommandPaletteModel::matchesQuery(row, QString::fromUtf8("تشغيل")));
    QVERIFY(CommandPaletteModel::matchesQuery(row, QStringLiteral("f5")));
    QVERIFY(CommandPaletteModel::matchesQuery(row, QStringLiteral("CURRENT")));
    QVERIFY(!CommandPaletteModel::matchesQuery(row, QStringLiteral("missing")));
}

QTEST_MAIN(TestCommandRegistry)
#include "TestCommandRegistry.moc"
