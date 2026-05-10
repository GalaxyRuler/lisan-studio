#include <QtTest/QtTest>

#include "ProjectModel.h"
#include "RuntimeRunner.h"
#include "SearchService.h"
#include "SettingsStore.h"

class TestProjectSearchRuntime : public QObject
{
    Q_OBJECT

private slots:
    void projectModelIgnoresBuildAndCacheDirectories();
    void searchServiceFindsUtf8ArabicMatches();
    void searchServiceStopsBeforeHugeProjectTail();
    void runtimeRunnerBuildsExplicitArgumentList();
    void runtimeRunnerCanUseExplicitProjectWorkingDirectory();
    void runtimeRunnerUsesIsolatedUtf8PythonEnvironment();
    void runtimeRunnerReportsMissingBundledPythonDiagnostics();
    void settingsStorePersistsArabicFontAndRecentProject();
};

static QString writeFile(const QDir &root, const QString &relative, const QString &text)
{
    const QFileInfo info(root.filePath(relative));
    QDir().mkpath(info.absolutePath());
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qFatal("could not write test file");
    }
    file.write(text.toUtf8());
    return info.absoluteFilePath();
}

void TestProjectSearchRuntime::projectModelIgnoresBuildAndCacheDirectories()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("اطبع(\"مرحبا\")\n"));
    writeFile(root, QStringLiteral("build/generated.apy"), QString::fromUtf8("اطبع(\"لا\")\n"));
    writeFile(root, QStringLiteral(".cache/hidden.apy"), QString::fromUtf8("اطبع(\"لا\")\n"));

    ProjectModel model;
    model.openRoot(root.absolutePath());

    const auto files = model.files();
    QCOMPARE(files.size(), 1);
    QCOMPARE(QFileInfo(files.first()).fileName(), QStringLiteral("main.apy"));
}

void TestProjectSearchRuntime::searchServiceFindsUtf8ArabicMatches()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString path = writeFile(root, QStringLiteral("src/main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    SearchService search;
    const auto rows = search.search(root.absolutePath(), QString::fromUtf8("اطبع"));

    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.first().path, path);
    QCOMPARE(rows.first().line, 2);
    QVERIFY(rows.first().preview.contains(QString::fromUtf8("اطبع")));
}

void TestProjectSearchRuntime::searchServiceStopsBeforeHugeProjectTail()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());

    for (int i = 0; i < 650; ++i) {
        writeFile(
            root,
            QStringLiteral("src/%1.apy").arg(i, 4, 10, QLatin1Char('0')),
            i == 640 ? QStringLiteral("adult\n") : QStringLiteral("لا يوجد\n"));
    }

    SearchService search;
    const auto rows = search.search(root.absolutePath(), QStringLiteral("adult"));

    QVERIFY(rows.isEmpty());
}

void TestProjectSearchRuntime::runtimeRunnerBuildsExplicitArgumentList()
{
    RuntimeRunner runner;
    runner.setRuntimeRoot(QStringLiteral("C:/app/runtime"));

    const auto command = runner.buildCommand(RuntimeAction::Run, QStringLiteral("C:/project/main.apy"));
    const auto lint = runner.buildCommand(RuntimeAction::Lint, QStringLiteral("C:/project/main.apy"));
    const auto format = runner.buildCommand(RuntimeAction::Format, QStringLiteral("C:/project/main.apy"));

    QVERIFY(command.program.endsWith(QStringLiteral("/python.exe")) || command.program.endsWith(QStringLiteral("\\python.exe")));
    QCOMPARE(command.arguments.at(0), QStringLiteral("-m"));
    QCOMPARE(command.arguments.at(1), QStringLiteral("arabicpython.cli"));
    QCOMPARE(command.arguments.last(), QStringLiteral("C:/project/main.apy"));
    QCOMPARE(lint.arguments.at(1), QStringLiteral("arabicpython.linter"));
    QCOMPARE(format.arguments.at(1), QStringLiteral("arabicpython.formatter"));
    QVERIFY(!command.program.contains(QStringLiteral(" ")));
}

void TestProjectSearchRuntime::runtimeRunnerCanUseExplicitProjectWorkingDirectory()
{
    RuntimeRunner runner;
    runner.setRuntimeRoot(QStringLiteral("C:/app/runtime"));

    const auto command = runner.buildCommand(
        RuntimeAction::Run,
        QStringLiteral("C:/project/.arabic-code-studio/run-buffer.apy"),
        QStringLiteral("C:/project"));

    QCOMPARE(command.workingDirectory, QStringLiteral("C:/project"));
    QCOMPARE(command.arguments.last(), QStringLiteral("C:/project/.arabic-code-studio/run-buffer.apy"));
}

void TestProjectSearchRuntime::runtimeRunnerUsesIsolatedUtf8PythonEnvironment()
{
    RuntimeRunner runner;
    const auto environment = runner.processEnvironment();

    QCOMPARE(environment.value(QStringLiteral("PYTHONNOUSERSITE")), QStringLiteral("1"));
    QCOMPARE(environment.value(QStringLiteral("PYTHONUTF8")), QStringLiteral("1"));
    QCOMPARE(environment.value(QStringLiteral("PYTHONIOENCODING")), QStringLiteral("utf-8"));
    QVERIFY(!environment.contains(QStringLiteral("PYTHONHOME")));
    QVERIFY(!environment.contains(QStringLiteral("PYTHONPATH")));
}

void TestProjectSearchRuntime::runtimeRunnerReportsMissingBundledPythonDiagnostics()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    RuntimeRunner runner;
    runner.setRuntimeRoot(temp.path() + QStringLiteral("/runtime"));

    const RuntimeDiagnostics diagnostics = runner.diagnostics(100);
    QVERIFY(diagnostics.pythonExecutable.endsWith(QStringLiteral("runtime\\python\\python.exe"))
        || diagnostics.pythonExecutable.endsWith(QStringLiteral("runtime/python/python.exe")));
    QVERIFY(!diagnostics.pythonExists);
    QVERIFY(!diagnostics.packageAvailable);
    QCOMPARE(diagnostics.packageVersion, QString());
    QVERIFY(diagnostics.statusText.contains(QString::fromUtf8("غير متوفر")));
}

void TestProjectSearchRuntime::settingsStorePersistsArabicFontAndRecentProject()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SettingsStore store(temp.path() + QStringLiteral("/settings.ini"));
    store.setEditorFontFamily(QString::fromUtf8("Cascadia Code"));
    store.addRecentProject(QString::fromUtf8("C:/مشروع"));

    SettingsStore reloaded(temp.path() + QStringLiteral("/settings.ini"));
    QCOMPARE(reloaded.editorFontFamily(), QString::fromUtf8("Cascadia Code"));
    QCOMPARE(reloaded.recentProjects().first(), QString::fromUtf8("C:/مشروع"));
}

QTEST_MAIN(TestProjectSearchRuntime)
#include "TestProjectSearchRuntime.moc"
