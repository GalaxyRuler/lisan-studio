#include <QtTest/QtTest>

#include "MainWindow.h"
#include "SettingsStore.h"

#include <QCoreApplication>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>

namespace {

QString settingsPathFor(const QTemporaryDir &temp)
{
    return temp.filePath(QStringLiteral("settings.ini"));
}

QStringList storedUntitledDrafts(const QString &settingsPath)
{
    QSettings settings(settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("session/untitledDrafts")).toStringList();
}

bool containsUntitledDraftsKey(const QString &settingsPath)
{
    QSettings settings(settingsPath, QSettings::IniFormat);
    return settings.contains(QStringLiteral("session/untitledDrafts"));
}

} // namespace

class TestUntitledDraftRecovery : public QObject
{
    Q_OBJECT

private slots:
    void roundtripMultiLineArabicDraftPreservesContent();
    void roundtripPreservesDraftOrderingAcrossManyDrafts();
    void roundtripDropsEmptyDraftEntries();
    void roundtripPreservesRtlMarksAndBidiContent();
    void firstKeystrokeTransitionPersistsButSubsequentEditsDoNotUntilNextTransition();
};

void TestUntitledDraftRecovery::roundtripMultiLineArabicDraftPreservesContent()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    const QString draft = QString::fromUtf8("سطر أول\nprint عدد123\tkeyword\nاطبع(\"نهاية\")");
    SavedWorkbenchSession session;
    session.untitledDrafts = {draft};

    SettingsStore(settingsPathFor(temp)).saveWorkbenchSession(session);

    const SavedWorkbenchSession loaded = SettingsStore(settingsPathFor(temp)).savedWorkbenchSession();
    QCOMPARE(loaded.untitledDrafts.size(), 1);
    QCOMPARE(loaded.untitledDrafts.first().toUtf8(), draft.toUtf8());
}

void TestUntitledDraftRecovery::roundtripPreservesDraftOrderingAcrossManyDrafts()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SavedWorkbenchSession session;
    for (int i = 0; i < 10; ++i) {
        session.untitledDrafts.append(QString::fromUtf8("مسودة %1 / draft-%1").arg(i));
    }

    SettingsStore(settingsPathFor(temp)).saveWorkbenchSession(session);

    const SavedWorkbenchSession loaded = SettingsStore(settingsPathFor(temp)).savedWorkbenchSession();
    QCOMPARE(loaded.untitledDrafts, session.untitledDrafts);
}

void TestUntitledDraftRecovery::roundtripDropsEmptyDraftEntries()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SavedWorkbenchSession session;
    session.untitledDrafts = {
        QString(),
        QString::fromUtf8("المسودة الوحيدة"),
        QString(),
    };

    SettingsStore(settingsPathFor(temp)).saveWorkbenchSession(session);

    const SavedWorkbenchSession loaded = SettingsStore(settingsPathFor(temp)).savedWorkbenchSession();
    QCOMPARE(loaded.untitledDrafts, QStringList({QString::fromUtf8("المسودة الوحيدة")}));
}

void TestUntitledDraftRecovery::roundtripPreservesRtlMarksAndBidiContent()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    const QString draft = QString(QChar(0x200F))
        + QString::fromUtf8("مرحبا alpha ")
        + QString(QChar(0x200E))
        + QString::fromUtf8(" beta عالم");
    SavedWorkbenchSession session;
    session.untitledDrafts = {draft};

    SettingsStore(settingsPathFor(temp)).saveWorkbenchSession(session);

    const SavedWorkbenchSession loaded = SettingsStore(settingsPathFor(temp)).savedWorkbenchSession();
    QCOMPARE(loaded.untitledDrafts.size(), 1);
    QCOMPARE(loaded.untitledDrafts.first().toUtf8(), draft.toUtf8());
}

void TestUntitledDraftRecovery::firstKeystrokeTransitionPersistsButSubsequentEditsDoNotUntilNextTransition()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString settingsPath = settingsPathFor(temp);

    MainWindow window(nullptr, settingsPath);
    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->toPlainText(), QString());
    QVERIFY(!editor->isDirty());

    QVERIFY(!containsUntitledDraftsKey(settingsPath) || storedUntitledDrafts(settingsPath).isEmpty());

    editor->insertPlainText(QStringLiteral("X"));
    QCoreApplication::processEvents();

    const QStringList firstTransitionDrafts = storedUntitledDrafts(settingsPath);
    QCOMPARE(firstTransitionDrafts, QStringList({editor->toPlainText()}));
    QCOMPARE(firstTransitionDrafts, QStringList({QStringLiteral("X")}));

    const QStringList snapshot = firstTransitionDrafts;
    editor->insertPlainText(QStringLiteral("YZ"));
    QCoreApplication::processEvents();

    QCOMPARE(editor->toPlainText(), QStringLiteral("XYZ"));
    QCOMPARE(storedUntitledDrafts(settingsPath), snapshot);
    QVERIFY(storedUntitledDrafts(settingsPath) != QStringList({QStringLiteral("XYZ")}));
}

QTEST_MAIN(TestUntitledDraftRecovery)
#include "TestUntitledDraftRecovery.moc"
