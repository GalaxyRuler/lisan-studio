#include <QtTest/QtTest>

#include "MainWindow.h"
#include "SettingsStore.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>

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

bool nonOrderlyShutdownSentinel(const QString &settingsPath)
{
    QSettings settings(settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("session/nonOrderlyShutdown"), false).toBool();
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
    void dirtyUntitledBufferAutosavesEveryFiveSecondsOfContinuousEditing();
    void gracefulShutdownClearsTheNonOrderlySentinel();
    void forceKillSimulationPersistsDraftsAndPromptsOnRelaunch();
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

void TestUntitledDraftRecovery::dirtyUntitledBufferAutosavesEveryFiveSecondsOfContinuousEditing()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString settingsPath = settingsPathFor(temp);

    MainWindow window(nullptr, settingsPath);
    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->toPlainText(), QString());
    QVERIFY(!editor->isDirty());
    auto *autosaveTimer = window.findChild<QTimer *>(QStringLiteral("untitledDraftAutosaveTimer"));
    QVERIFY(autosaveTimer != nullptr);
    QCOMPARE(autosaveTimer->interval(), 5000);
    autosaveTimer->setInterval(10);

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

    QTRY_COMPARE_WITH_TIMEOUT(storedUntitledDrafts(settingsPath), QStringList({QStringLiteral("XYZ")}), 1000);
    QVERIFY(storedUntitledDrafts(settingsPath) != snapshot);
}

void TestUntitledDraftRecovery::gracefulShutdownClearsTheNonOrderlySentinel()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString settingsPath = settingsPathFor(temp);

    {
        MainWindow window(nullptr, settingsPath);
        QVERIFY(nonOrderlyShutdownSentinel(settingsPath));
        QVERIFY(window.close());
    }

    QVERIFY(!nonOrderlyShutdownSentinel(settingsPath));
}

void TestUntitledDraftRecovery::forceKillSimulationPersistsDraftsAndPromptsOnRelaunch()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString settingsPath = settingsPathFor(temp);
    const QString draft = QString::fromUtf8("اطبع(\"استعادة بعد انقطاع\")");

    {
        MainWindow interrupted(nullptr, settingsPath);
        auto *editor = interrupted.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
        QVERIFY(editor != nullptr);
        auto *autosaveTimer = interrupted.findChild<QTimer *>(QStringLiteral("untitledDraftAutosaveTimer"));
        QVERIFY(autosaveTimer != nullptr);
        autosaveTimer->setInterval(10);

        editor->insertPlainText(draft);
        QTRY_COMPARE_WITH_TIMEOUT(storedUntitledDrafts(settingsPath), QStringList({draft}), 1000);
        QVERIFY(nonOrderlyShutdownSentinel(settingsPath));
    }

    bool recoveryPromptObserved = false;
    QTimer::singleShot(0, [&recoveryPromptObserved]() {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box != nullptr);
        recoveryPromptObserved = box->windowTitle().contains(QString::fromUtf8("استعادة"))
            || box->text().contains(QString::fromUtf8("استعادة"))
            || box->informativeText().contains(QString::fromUtf8("استعادة"));
        box->button(QMessageBox::Yes)->click();
    });

    MainWindow relaunched(nullptr, settingsPath);
    QVERIFY(recoveryPromptObserved);
    auto *tabs = relaunched.findChild<QTabWidget *>(QStringLiteral("editorTabs"));
    QVERIFY(tabs != nullptr);
    auto *editor = qobject_cast<EditorSurface *>(tabs->currentWidget());
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->toPlainText(), draft);
    QVERIFY(editor->isDirty());
}

QTEST_MAIN(TestUntitledDraftRecovery)
#include "TestUntitledDraftRecovery.moc"
