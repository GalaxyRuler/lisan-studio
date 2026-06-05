#include <QtTest/QtTest>

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QTabWidget>

#include "BottomPanelController.h"
#include "DocumentFileIO.h"
#include "EditorFindService.h"
#include "ApySnippetService.h"
#include "OutputTranscript.h"
#include "ProjectModel.h"
#include "ProjectFileOperations.h"
#include "ProjectReplaceService.h"
#include "RuntimeProblemParser.h"
#include "RuntimeHistory.h"
#include "RuntimeRunConfiguration.h"
#include "RuntimeRunConfigurationStore.h"
#include "RuntimeRunner.h"
#include "SearchService.h"
#include "SettingsDialogModel.h"
#include "SettingsStore.h"
#include "TerminalLinkParser.h"
#include "TerminalProfileModel.h"
#include "WorkspaceSettingsStore.h"

class TestProjectSearchRuntime : public QObject
{
    Q_OBJECT

private slots:
    void projectModelIgnoresBuildAndCacheDirectories();
    void projectModelValidatesSafeChildNames();
    void projectModelDetectsPathsCoveredByProjectOperations();
    void projectFileOperationsRejectUnsafeTargetsAndCollisions();
    void projectFileOperationsProtectRootAndResolveContainingFolder();
    void projectFileOperationsBuildsExplorerRevealArguments();
    void searchServiceFindsUtf8ArabicMatches();
    void searchServiceFindsUnsavedEditorMatches();
    void searchServiceMergesImmediateRowsBeforeProjectRows();
    void searchServiceStopsBeforeHugeProjectTail();
    void searchServiceClipsAtMaxScannedFilesWithStableOrdering();
    void searchServiceReportsTruncationWhenCapFires();
    void searchServiceDoesNotReportTruncationWhenLimitHit();
    void searchServiceDoesNotReportTruncationWhenScanCompletes();
    void runtimeRunnerBuildsExplicitArgumentList();
    void runtimeRunnerBuildsLaunchPlanWithSummaryText();
    void runtimeRunnerCanUseExplicitProjectWorkingDirectory();
    void runtimeRunnerUsesIsolatedUtf8PythonEnvironment();
    void runtimeRunnerReportsMissingBundledPythonDiagnostics();
    void runtimeHistoryRecordsMostRecentLaunchesFirst();
    void runtimeHistoryCapsEntriesAndCanClear();
    void runtimeRunConfigurationModelCreatesCurrentFileDefault();
    void runtimeRunConfigurationModelRejectsInvalidAndDuplicateEntries();
    void runtimeRunConfigurationStoreDefaultsWhenMissingOrInvalid();
    void runtimeRunConfigurationStorePersistsConfigurations();
    void terminalProfileModelBuildsExplicitPowerShellProfile();
    void terminalProfileModelRequiresWorkspaceTrustForLaunch();
    void terminalLinkParserExtractsWindowsPathsWithLineAndColumn();
    void terminalLinkParserExtractsForwardSlashPaths();
    void outputTranscriptRendersAndFiltersByChannel();
    void outputTranscriptFiltersByCaseInsensitiveText();
    void outputTranscriptExposesLinksFromRenderedText();
    void bottomPanelControllerMapsStableIdsAndSelections();
    void runtimeProblemParserExtractsArabicSyntaxLine();
    void settingsStorePersistsArabicFontAndRecentProject();
    void settingsStorePersistsRecentFilesMostRecentFirst();
    void settingsStorePersistsThemePreference();
    void settingsStorePersistsWorkbenchSession();
    void settingsStorePersistsShortcutSettingsJson();
    void packageScriptDerivesMsiArtifactNameFromProductVersion();
    void packageScriptStampsInstallerBuildId();
    void releaseEvidenceScriptDerivesMsiPathFromProductVersion();
    void workspaceSettingsStoreDefaultsWhenMissingOrInvalid();
    void workspaceSettingsStorePersistsTrustAndEditorPreferences();
    void settingsDialogModelBuildsUiStateFromStoreAndDiagnostics();
    void documentFileIoReadsUtf8AndRecordsIdentity();
    void documentFileIoWritesAtomicallyAndPreservesUtf8();
    void documentFileIoRejectsInvalidUtf8ByPolicy();
    void editorFindServiceFindsArabicEnglishAndMixedMatches();
    void editorFindServiceReplacesCurrentAndAllMatches();
    void apySnippetServiceListsArabicFirstSnippets();
    void apySnippetServiceFindsSnippetById();
    void projectReplaceServicePreviewsArabicMixedMatches();
    void projectReplaceServiceSkipsIgnoredDirectories();
    void projectReplaceServiceReportsTruncationWhenCapFires();
    void projectReplaceServiceMergesImmediateRowsBeforeDiskRows();
    void projectReplaceSelectionAcceptsRowsByDefault();
    void projectReplaceSelectionRejectsSingleRowsAndWholeFiles();
    void projectReplaceServiceAppliesAcceptedRowsAtomically();
    void projectReplacePreservesLeadingWhitespace();
    void projectReplacePreservesCRLF();
    void projectReplaceRejectsStaleWhitespaceChange();
    void projectReplaceServicePreservesIndentedMatchedLines();
    void projectReplaceServicePreservesCrlfLineEndings();
    void projectReplaceServiceRejectsStaleRowsWithoutWriting();
};

static QString writeFile(const QDir &root, const QString &relative, const QString &text)
{
    const QFileInfo info(root.filePath(relative));
    QDir().mkpath(info.absolutePath());
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qFatal("could not write test file");
    }
    file.write(text.toUtf8());
    return info.absoluteFilePath();
}

static QString writeRawFile(const QDir &root, const QString &relative, const QByteArray &bytes)
{
    const QFileInfo info(root.filePath(relative));
    QDir().mkpath(info.absolutePath());
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qFatal("could not write raw test file");
    }
    file.write(bytes);
    return info.absoluteFilePath();
}

static QByteArray readRawFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("could not read raw test file");
    }
    return file.readAll();
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

void TestProjectSearchRuntime::projectModelValidatesSafeChildNames()
{
    QVERIFY(ProjectModel::isValidChildName(QStringLiteral("main.apy")));
    QVERIFY(ProjectModel::isValidChildName(QString::fromUtf8("ملف جديد.apy")));
    QVERIFY(!ProjectModel::isValidChildName(QString()));
    QVERIFY(!ProjectModel::isValidChildName(QStringLiteral("..")));
    QVERIFY(!ProjectModel::isValidChildName(QStringLiteral("nested/file.apy")));
    QVERIFY(!ProjectModel::isValidChildName(QStringLiteral("nested\\file.apy")));
}

void TestProjectSearchRuntime::projectModelDetectsPathsCoveredByProjectOperations()
{
    QVERIFY(ProjectModel::pathIsSameOrInside(
        QStringLiteral("C:/Users/Admin/project/src/main.apy"),
        QStringLiteral("C:/Users/Admin/project/src")));
    QVERIFY(ProjectModel::pathIsSameOrInside(
        QStringLiteral("C:\\Users\\Admin\\project\\src"),
        QStringLiteral("C:/Users/Admin/project/src")));
    QVERIFY(!ProjectModel::pathIsSameOrInside(
        QStringLiteral("C:/Users/Admin/project/src-other/main.apy"),
        QStringLiteral("C:/Users/Admin/project/src")));
}

void TestProjectSearchRuntime::projectFileOperationsRejectUnsafeTargetsAndCollisions()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString existing = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("اطبع(\"مرحبا\")\n"));

    QString error;
    const ProjectFileOperationTarget safe = ProjectFileOperations::childTarget(
        root.absolutePath(),
        root.absolutePath(),
        QString::fromUtf8("ملف.apy"),
        true,
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(safe.path, root.filePath(QString::fromUtf8("ملف.apy")));

    const ProjectFileOperationTarget duplicate = ProjectFileOperations::childTarget(
        root.absolutePath(),
        root.absolutePath(),
        QStringLiteral("main.apy"),
        true,
        &error);
    QVERIFY(!duplicate.allowed);
    QVERIFY(error.contains(QString::fromUtf8("موجود")));

    const ProjectFileOperationTarget escaped = ProjectFileOperations::childTarget(
        root.absolutePath(),
        root.absolutePath(),
        QStringLiteral("../escape.apy"),
        true,
        &error);
    QVERIFY(!escaped.allowed);
    QVERIFY(error.contains(QString::fromUtf8("اسم")));

    const ProjectFileOperationTarget renameCollision = ProjectFileOperations::renameTarget(
        existing,
        QStringLiteral("main.apy"),
        root.absolutePath(),
        &error);
    QVERIFY(!renameCollision.allowed);
    QVERIFY(error.contains(QString::fromUtf8("مستخدم")) || error.contains(QString::fromUtf8("موجود")));
}

void TestProjectSearchRuntime::projectFileOperationsProtectRootAndResolveContainingFolder()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("src/main.apy"), QString::fromUtf8("اطبع(\"مرحبا\")\n"));

    QString error;
    QVERIFY(!ProjectFileOperations::canDelete(root.absolutePath(), root.absolutePath(), &error));
    QVERIFY(error.contains(QString::fromUtf8("جذر")));
    QVERIFY(ProjectFileOperations::canDelete(filePath, root.absolutePath(), &error));

    QCOMPARE(ProjectFileOperations::pathForClipboard(filePath), QDir::toNativeSeparators(QFileInfo(filePath).absoluteFilePath()));
    QCOMPARE(ProjectFileOperations::containingFolder(filePath), QFileInfo(filePath).absolutePath());
    QCOMPARE(ProjectFileOperations::containingFolder(root.absolutePath()), QFileInfo(root.absolutePath()).absoluteFilePath());
}

void TestProjectSearchRuntime::projectFileOperationsBuildsExplorerRevealArguments()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("src/main.apy"), QString::fromUtf8("اطبع(\"مرحبا\")\n"));

    const QStringList fileArguments = ProjectFileOperations::explorerRevealArguments(filePath);
    QCOMPARE(fileArguments.size(), 1);
    QCOMPARE(fileArguments.first(), QStringLiteral("/select,%1").arg(QDir::toNativeSeparators(QFileInfo(filePath).absoluteFilePath())));

    const QStringList folderArguments = ProjectFileOperations::explorerRevealArguments(root.absolutePath());
    QCOMPARE(folderArguments, QStringList({QDir::toNativeSeparators(QFileInfo(root.absolutePath()).absoluteFilePath())}));
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

void TestProjectSearchRuntime::searchServiceFindsUnsavedEditorMatches()
{
    SearchService search;
    const auto rows = search.searchText(
        QStringLiteral("C:/project/current.apy"),
        QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"),
        QString::fromUtf8("اطبع"));

    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.first().path, QStringLiteral("C:/project/current.apy"));
    QCOMPARE(rows.first().line, 2);
    QCOMPARE(rows.first().preview, QString::fromUtf8("اطبع(عدد)"));
}

void TestProjectSearchRuntime::searchServiceMergesImmediateRowsBeforeProjectRows()
{
    const QVector<SearchResultRow> immediateRows = {
        {QStringLiteral("C:/project/main.apy"), 2, QString::fromUtf8("اطبع(عدد)")},
    };
    const QVector<SearchResultRow> projectRows = {
        {QStringLiteral("C:/project/main.apy"), 2, QString::fromUtf8("اطبع(عدد)")},
        {QStringLiteral("C:/project/other.apy"), 4, QString::fromUtf8("اطبع(\"آخر\")")},
    };

    const auto rows = SearchService::mergeRows(immediateRows, projectRows);

    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.at(0).path, QStringLiteral("C:/project/main.apy"));
    QCOMPARE(rows.at(1).path, QStringLiteral("C:/project/other.apy"));
}

void TestProjectSearchRuntime::searchServiceStopsBeforeHugeProjectTail()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());

    constexpr int maxScannedFiles = 10;
    for (int i = 0; i < 12; ++i) {
        writeFile(
            root,
            QStringLiteral("src/%1.apy").arg(i, 2, 10, QLatin1Char('0')),
            i == 11 ? QStringLiteral("adult\n") : QStringLiteral("لا يوجد\n"));
    }

    SearchService search;
    const auto rows = search.search(root.absolutePath(), QStringLiteral("adult"), 250, maxScannedFiles);

    QVERIFY(rows.isEmpty());
}

void TestProjectSearchRuntime::searchServiceClipsAtMaxScannedFilesWithStableOrdering()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());

    constexpr int maxScannedFiles = 10;
    for (int i = 0; i < 12; ++i) {
        writeFile(
            root,
            QStringLiteral("file-%1.apy").arg(i, 2, 10, QLatin1Char('0')),
            QStringLiteral("needle\n"));
    }

    SearchService search;
    QElapsedTimer timer;
    timer.start();
    const auto rows = search.search(root.absolutePath(), QStringLiteral("needle"), 1000, maxScannedFiles);
    const qint64 elapsedMs = timer.elapsed();

    QCOMPARE(rows.size(), maxScannedFiles);
    QVERIFY(!rows.isEmpty());
    QVERIFY2(elapsedMs <= 2000,
        qPrintable(QStringLiteral("search cap sweep exceeded 2000 ms budget: %1 ms").arg(elapsedMs)));
    QCOMPARE(QFileInfo(rows.first().path).fileName(), QStringLiteral("file-00.apy"));
    QCOMPARE(QFileInfo(rows.last().path).fileName(), QStringLiteral("file-09.apy"));

    const SearchResults result = search.searchWithMetadata(root.absolutePath(), QStringLiteral("needle"), 1000, maxScannedFiles);
    QCOMPARE(result.rows.size(), maxScannedFiles);
    QVERIFY(result.truncatedAtFileCap);
}

void TestProjectSearchRuntime::searchServiceReportsTruncationWhenCapFires()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());

    constexpr int maxScannedFiles = 10;
    for (int i = 0; i < 12; ++i) {
        writeFile(
            root,
            QStringLiteral("file-%1.apy").arg(i, 2, 10, QLatin1Char('0')),
            QStringLiteral("needle\n"));
    }

    SearchService search;
    const SearchResults result = search.searchWithMetadata(root.absolutePath(), QStringLiteral("needle"), 1000, maxScannedFiles);

    QCOMPARE(result.rows.size(), maxScannedFiles);
    QVERIFY(result.truncatedAtFileCap);
}

void TestProjectSearchRuntime::searchServiceDoesNotReportTruncationWhenLimitHit()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());

    for (int i = 0; i < 50; ++i) {
        writeFile(
            root,
            QStringLiteral("file-%1.apy").arg(i, 3, 10, QLatin1Char('0')),
            QStringLiteral("needle\n"));
    }

    SearchService search;
    const SearchResults result = search.searchWithMetadata(root.absolutePath(), QStringLiteral("needle"), 10);

    QVERIFY(!result.truncatedAtFileCap);
    QCOMPARE(result.rows.size(), 10);
}

void TestProjectSearchRuntime::searchServiceDoesNotReportTruncationWhenScanCompletes()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());

    for (int i = 0; i < 50; ++i) {
        writeFile(
            root,
            QStringLiteral("file-%1.apy").arg(i, 3, 10, QLatin1Char('0')),
            QStringLiteral("needle\n"));
    }

    SearchService search;
    const SearchResults result = search.searchWithMetadata(root.absolutePath(), QStringLiteral("needle"), 1000);

    QVERIFY(!result.truncatedAtFileCap);
    QCOMPARE(result.rows.size(), 50);
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

void TestProjectSearchRuntime::runtimeRunnerBuildsLaunchPlanWithSummaryText()
{
    RuntimeRunner runner;
    runner.setRuntimeRoot(QStringLiteral("C:/app/runtime"));

    const auto plan = runner.buildLaunchPlan(
        RuntimeAction::Run,
        QString::fromUtf8("تشغيل"),
        QStringLiteral("C:/project/main.apy"),
        QStringLiteral("C:/project"),
        true);

    QCOMPARE(plan.title, QString::fromUtf8("تشغيل"));
    QCOMPARE(plan.filePath, QStringLiteral("C:/project/main.apy"));
    QCOMPARE(plan.command.workingDirectory, QStringLiteral("C:/project"));
    QVERIFY(plan.reloadAfterSuccess);
    QVERIFY(plan.initialOutput.contains(QString::fromUtf8("الأمر: تشغيل")));
    QVERIFY(plan.initialOutput.contains(QString::fromUtf8("ملف:")));
    QVERIFY(plan.initialOutput.contains(QString::fromUtf8("مجلد العمل:")));
    QCOMPARE(plan.runningStatus, QString::fromUtf8("تشغيل..."));
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

void TestProjectSearchRuntime::runtimeHistoryRecordsMostRecentLaunchesFirst()
{
    RuntimeRunner runner;
    RuntimeHistory history;

    const RuntimeLaunchPlan first = runner.buildLaunchPlan(
        RuntimeAction::Run,
        QString::fromUtf8("تشغيل"),
        QStringLiteral("C:/project/main.apy"),
        QStringLiteral("C:/project"));
    const RuntimeLaunchPlan second = runner.buildLaunchPlan(
        RuntimeAction::Lint,
        QString::fromUtf8("فحص"),
        QStringLiteral("C:/project/src/other.apy"),
        QStringLiteral("C:/project/src"));

    history.recordLaunch(first);
    history.recordLaunch(second);

    const QVector<RuntimeHistoryEntry> entries = history.entries();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0).action, RuntimeAction::Lint);
    QCOMPARE(entries.at(0).title, QString::fromUtf8("فحص"));
    QCOMPARE(entries.at(0).filePath, QStringLiteral("C:/project/src/other.apy"));
    QCOMPARE(entries.at(0).workingDirectory, QStringLiteral("C:/project/src"));
    QCOMPARE(entries.at(0).command.program, second.command.program);
    QVERIFY(!entries.at(0).reloadAfterSuccess);
    QCOMPARE(entries.at(1).action, RuntimeAction::Run);
}

void TestProjectSearchRuntime::runtimeHistoryCapsEntriesAndCanClear()
{
    RuntimeRunner runner;
    RuntimeHistory history(3);

    for (int index = 0; index < 5; ++index) {
        history.recordLaunch(runner.buildLaunchPlan(
            RuntimeAction::Run,
            QString::fromUtf8("تشغيل %1").arg(index),
            QStringLiteral("C:/project/%1.apy").arg(index),
            QStringLiteral("C:/project")));
    }

    const QVector<RuntimeHistoryEntry> entries = history.entries();
    QCOMPARE(entries.size(), 3);
    QCOMPARE(entries.at(0).filePath, QStringLiteral("C:/project/4.apy"));
    QCOMPARE(entries.at(2).filePath, QStringLiteral("C:/project/2.apy"));

    history.clear();
    QVERIFY(history.entries().isEmpty());
}

void TestProjectSearchRuntime::runtimeRunConfigurationModelCreatesCurrentFileDefault()
{
    const RuntimeRunConfiguration config = RuntimeRunConfigurationModel::currentFileRunConfiguration(
        QStringLiteral("C:/project/main.apy"),
        QStringLiteral("C:/project"));

    QCOMPARE(config.id, QStringLiteral("current-file"));
    QCOMPARE(config.name, QString::fromUtf8("تشغيل الملف الحالي"));
    QCOMPARE(config.action, RuntimeAction::Run);
    QCOMPARE(config.filePath, QStringLiteral("C:/project/main.apy"));
    QCOMPARE(config.workingDirectory, QStringLiteral("C:/project"));
    QVERIFY(!config.reloadAfterSuccess);

    RuntimeRunConfigurationModel model;
    QString error;
    QVERIFY2(model.addConfiguration(config, &error), qPrintable(error));
    QCOMPARE(model.configurations().size(), 1);
    QVERIFY(model.findById(QStringLiteral("current-file")) != nullptr);
    QCOMPARE(model.findById(QStringLiteral("current-file"))->name, config.name);

    QVERIFY(model.removeConfiguration(QStringLiteral("current-file")));
    QVERIFY(model.configurations().isEmpty());
}

void TestProjectSearchRuntime::runtimeRunConfigurationModelRejectsInvalidAndDuplicateEntries()
{
    RuntimeRunConfigurationModel model;
    QString error;

    RuntimeRunConfiguration blankName;
    blankName.id = QStringLiteral("blank");
    blankName.filePath = QStringLiteral("C:/project/main.apy");
    QVERIFY(!model.addConfiguration(blankName, &error));
    QVERIFY(error.contains(QString::fromUtf8("اسم")));

    RuntimeRunConfiguration first = RuntimeRunConfigurationModel::currentFileRunConfiguration(
        QStringLiteral("C:/project/main.apy"),
        QStringLiteral("C:/project"));
    QVERIFY(model.addConfiguration(first, &error));

    RuntimeRunConfiguration duplicateId = first;
    duplicateId.name = QString::fromUtf8("تشغيل آخر");
    QVERIFY(!model.addConfiguration(duplicateId, &error));
    QVERIFY(error.contains(QStringLiteral("id")));

    RuntimeRunConfiguration duplicateName = first;
    duplicateName.id = QStringLiteral("other");
    duplicateName.name = first.name.toUpper();
    QVERIFY(!model.addConfiguration(duplicateName, &error));
    QVERIFY(error.contains(QString::fromUtf8("الاسم")));
}

void TestProjectSearchRuntime::runtimeRunConfigurationStoreDefaultsWhenMissingOrInvalid()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    RuntimeRunConfigurationStore store(temp.path());
    QString error;
    QVERIFY(store.load(&error).isEmpty());
    QVERIFY(error.isEmpty());

    const QFileInfo info(store.configurationsFilePath());
    QDir().mkpath(info.absolutePath());
    QFile invalid(info.absoluteFilePath());
    QVERIFY(invalid.open(QIODevice::WriteOnly | QIODevice::Text));
    invalid.write("{not json");
    invalid.close();

    const QVector<RuntimeRunConfiguration> loaded = store.load(&error);
    QVERIFY(loaded.isEmpty());
    QVERIFY(!error.isEmpty());
}

void TestProjectSearchRuntime::runtimeRunConfigurationStorePersistsConfigurations()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    RuntimeRunConfiguration configuration;
    configuration.id = QStringLiteral("format-main");
    configuration.name = QString::fromUtf8("تنسيق الملف الرئيسي");
    configuration.action = RuntimeAction::Format;
    configuration.filePath = QStringLiteral("C:/project/main.apy");
    configuration.workingDirectory = QStringLiteral("C:/project");
    configuration.reloadAfterSuccess = true;

    RuntimeRunConfigurationStore store(temp.path());
    QString error;
    QVERIFY2(store.save({configuration}, &error), qPrintable(error));

    const QVector<RuntimeRunConfiguration> loaded = store.load(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.first().id, configuration.id);
    QCOMPARE(loaded.first().name, configuration.name);
    QCOMPARE(loaded.first().action, RuntimeAction::Format);
    QCOMPARE(loaded.first().filePath, configuration.filePath);
    QCOMPARE(loaded.first().workingDirectory, configuration.workingDirectory);
    QVERIFY(loaded.first().reloadAfterSuccess);
}

void TestProjectSearchRuntime::terminalProfileModelBuildsExplicitPowerShellProfile()
{
    const TerminalProfile profile = TerminalProfileModel::defaultPowerShellProfile(QStringLiteral("C:/project"));

    QCOMPARE(profile.id, QStringLiteral("powershell"));
    QCOMPARE(profile.name, QStringLiteral("PowerShell"));
    QCOMPARE(profile.program, QStringLiteral("powershell.exe"));
    QCOMPARE(profile.arguments, QStringList({QStringLiteral("-NoLogo")}));
    QCOMPARE(profile.workingDirectory, QStringLiteral("C:/project"));
    QVERIFY(profile.requiresTrustedWorkspace);
    QVERIFY(!profile.arguments.join(QLatin1Char(' ')).contains(QStringLiteral("&&")));
}

void TestProjectSearchRuntime::terminalProfileModelRequiresWorkspaceTrustForLaunch()
{
    const TerminalProfile profile = TerminalProfileModel::defaultPowerShellProfile(QStringLiteral("C:/project"));

    const TerminalLaunchPlan blocked = TerminalProfileModel::buildLaunchPlan(profile, false);
    QVERIFY(!blocked.allowed);
    QVERIFY(blocked.reason.contains(QString::fromUtf8("الثقة")));
    QCOMPARE(blocked.command.program, QString());

    const TerminalLaunchPlan allowed = TerminalProfileModel::buildLaunchPlan(profile, true);
    QVERIFY(allowed.allowed);
    QCOMPARE(allowed.command.program, QStringLiteral("powershell.exe"));
    QCOMPARE(allowed.command.arguments, QStringList({QStringLiteral("-NoLogo")}));
    QCOMPARE(allowed.command.workingDirectory, QStringLiteral("C:/project"));
}

void TestProjectSearchRuntime::terminalLinkParserExtractsWindowsPathsWithLineAndColumn()
{
    const QString text = QString::fromUtf8("خطأ في C:\\Users\\Admin\\مشروع\\main.apy:12:7 عند التشغيل");

    const QVector<TerminalLink> links = TerminalLinkParser::linksForText(text);

    QCOMPARE(links.size(), 1);
    QCOMPARE(links.first().path, QString::fromUtf8("C:\\Users\\Admin\\مشروع\\main.apy"));
    QCOMPARE(links.first().line, 12);
    QCOMPARE(links.first().column, 7);
    QCOMPARE(text.mid(links.first().start, links.first().length), QString::fromUtf8("C:\\Users\\Admin\\مشروع\\main.apy:12:7"));
}

void TestProjectSearchRuntime::terminalLinkParserExtractsForwardSlashPaths()
{
    const QString text = QStringLiteral("opened C:/project/src/app.apy:42 and ignored relative/path.apy:2");

    const QVector<TerminalLink> links = TerminalLinkParser::linksForText(text);

    QCOMPARE(links.size(), 1);
    QCOMPARE(links.first().path, QStringLiteral("C:/project/src/app.apy"));
    QCOMPARE(links.first().line, 42);
    QCOMPARE(links.first().column, 0);
}

void TestProjectSearchRuntime::outputTranscriptRendersAndFiltersByChannel()
{
    OutputTranscript transcript;
    transcript.append(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), QString::fromUtf8("بدأ التشغيل"));
    transcript.append(OutputTranscriptChannel::Stdout, QStringLiteral("stdout"), QString::fromUtf8("مرحبا"));
    transcript.append(OutputTranscriptChannel::Stderr, QStringLiteral("stderr"), QString::fromUtf8("خطأ"));

    QCOMPARE(
        transcript.render(),
        QString::fromUtf8("[النظام]\nبدأ التشغيل\n[stdout]\nمرحبا\n[stderr]\nخطأ"));

    OutputTranscriptFilter filter;
    filter.includeStdout = false;
    filter.includeSystem = false;
    QCOMPARE(transcript.render(filter), QString::fromUtf8("[stderr]\nخطأ"));
}

void TestProjectSearchRuntime::outputTranscriptFiltersByCaseInsensitiveText()
{
    OutputTranscript transcript;
    transcript.append(OutputTranscriptChannel::Stdout, QStringLiteral("stdout"), QString::fromUtf8("Result: مرحبا"));
    transcript.append(OutputTranscriptChannel::Stderr, QStringLiteral("stderr"), QString::fromUtf8("warning: خافت"));
    transcript.append(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), QString::fromUtf8("انتهى"));

    OutputTranscriptFilter filter;
    filter.query = QStringLiteral("RESULT");
    QCOMPARE(transcript.render(filter), QString::fromUtf8("[stdout]\nResult: مرحبا"));

    filter.query = QString::fromUtf8("خافت");
    QCOMPARE(transcript.render(filter), QString::fromUtf8("[stderr]\nwarning: خافت"));

    transcript.clear();
    QCOMPARE(transcript.render(), QString());
}

void TestProjectSearchRuntime::outputTranscriptExposesLinksFromRenderedText()
{
    OutputTranscript transcript;
    transcript.append(OutputTranscriptChannel::System, QString::fromUtf8("النظام"), QString::fromUtf8("بدأ التشغيل"));
    transcript.append(OutputTranscriptChannel::Stderr, QStringLiteral("stderr"), QStringLiteral("C:/project/app.apy:9:2: syntax error"));

    const QVector<TerminalLink> links = transcript.links();
    const QString rendered = transcript.render();

    QCOMPARE(links.size(), 1);
    QCOMPARE(links.first().path, QStringLiteral("C:/project/app.apy"));
    QCOMPARE(links.first().line, 9);
    QCOMPARE(links.first().column, 2);
    QCOMPARE(rendered.mid(links.first().start, links.first().length), QStringLiteral("C:/project/app.apy:9:2"));

    OutputTranscriptFilter filter;
    filter.includeStderr = false;
    QVERIFY(transcript.links(filter).isEmpty());
}

void TestProjectSearchRuntime::bottomPanelControllerMapsStableIdsAndSelections()
{
    QTabWidget tabs;
    QPlainTextEdit terminal;
    QPlainTextEdit output;
    QListWidget problems;
    QListWidget search;
    QListWidget references;
    QPlainTextEdit debug;

    tabs.addTab(&terminal, QString::fromUtf8("الطرفية"));
    tabs.addTab(&output, QString::fromUtf8("الإخراج"));
    tabs.addTab(&problems, QString::fromUtf8("المشاكل"));
    tabs.addTab(&search, QString::fromUtf8("نتائج البحث"));
    tabs.addTab(&references, QString::fromUtf8("المراجع"));
    tabs.addTab(&debug, QString::fromUtf8("التصحيح"));

    BottomPanelController controller(&tabs, &output, &terminal, &problems, &search, &references, &debug);

    QCOMPARE(controller.panelId(&terminal), QStringLiteral("terminal"));
    QCOMPARE(controller.panelId(&output), QStringLiteral("output"));
    QCOMPARE(controller.panelId(&problems), QStringLiteral("problems"));
    QCOMPARE(controller.panelId(&search), QStringLiteral("search"));
    QCOMPARE(controller.panelId(&references), QStringLiteral("references"));
    QCOMPARE(controller.panelId(&debug), QStringLiteral("debug"));
    QCOMPARE(controller.panelId(nullptr), QStringLiteral("terminal"));

    QCOMPARE(controller.panelForId(QStringLiteral("terminal")), &terminal);
    QCOMPARE(controller.panelForId(QStringLiteral("output")), &output);
    QCOMPARE(controller.panelForId(QStringLiteral("problems")), &problems);
    QCOMPARE(controller.panelForId(QStringLiteral("search")), &search);
    QCOMPARE(controller.panelForId(QStringLiteral("references")), &references);
    QCOMPARE(controller.panelForId(QStringLiteral("debug")), &debug);
    QCOMPARE(controller.panelForId(QStringLiteral("missing")), &terminal);

    controller.showOutputPanel();
    QCOMPARE(tabs.currentWidget(), &output);
    controller.showTerminalPanel();
    QCOMPARE(tabs.currentWidget(), &terminal);
    controller.showProblemsPanel();
    QCOMPARE(tabs.currentWidget(), &problems);
    controller.showSearchResultsPanel();
    QCOMPARE(tabs.currentWidget(), &search);
    controller.showReferencesPanel();
    QCOMPARE(tabs.currentWidget(), &references);
}

void TestProjectSearchRuntime::runtimeProblemParserExtractsArabicSyntaxLine()
{
    const QString stderrText = QString::fromUtf8(
        "تتبع_الأخطاء (المكدس الأحدث آخرا):\n"
        "  ملف \"C:\\Users\\Admin\\AppData\\Local\\LisanStudio\\runtime\\python\\Lib\\site-packages\\arabicpython\\translate.py\", سطر 89, في translate\n"
        "خطأ_صياغة: حرف تحكم باتجاه النص غير مسموح خارج النصوص الحرفية: U+202E (RIGHT-TO-LEFT OVERRIDE)، السطر 11، العمود 15. راجع https://trojansource.codes لمعرفة السبب.\n");

    const RuntimeProblemDetail detail = parseRuntimeProblemDetail(stderrText, QString::fromUtf8("تشغيل"), 1);

    QCOMPARE(detail.line, 11);
    QVERIFY(detail.message.contains(QString::fromUtf8("خطأ_صياغة")));
    QVERIFY(detail.message.contains(QStringLiteral("RIGHT-TO-LEFT OVERRIDE")));
    QVERIFY(detail.message.contains(QString::fromUtf8("رمز الخروج 1")));
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

void TestProjectSearchRuntime::settingsStorePersistsRecentFilesMostRecentFirst()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SettingsStore store(temp.path() + QStringLiteral("/settings.ini"));
    for (int i = 0; i < 22; ++i) {
        store.addRecentFile(QStringLiteral("C:/project/%1.apy").arg(i));
    }
    store.addRecentFile(QStringLiteral("C:/project/5.apy"));

    SettingsStore reloaded(temp.path() + QStringLiteral("/settings.ini"));
    const QStringList files = reloaded.recentFiles();
    QCOMPARE(files.size(), 20);
    QCOMPARE(files.first(), QStringLiteral("C:/project/5.apy"));
    QCOMPARE(files.count(QStringLiteral("C:/project/5.apy")), 1);
    QVERIFY(!files.contains(QStringLiteral("C:/project/0.apy")));
}

void TestProjectSearchRuntime::settingsStorePersistsThemePreference()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SettingsStore store(temp.path() + QStringLiteral("/settings.ini"));
    QCOMPARE(store.themePreference(), QStringLiteral("dark"));

    store.setThemePreference(QStringLiteral("light"));

    SettingsStore reloaded(temp.path() + QStringLiteral("/settings.ini"));
    QCOMPARE(reloaded.themePreference(), QStringLiteral("light"));

    reloaded.setThemePreference(QStringLiteral("solarized"));
    QCOMPARE(reloaded.themePreference(), QStringLiteral("dark"));
}

void TestProjectSearchRuntime::settingsStorePersistsWorkbenchSession()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SettingsStore empty(temp.path() + QStringLiteral("/settings.ini"));
    const SavedWorkbenchSession emptySession = empty.savedWorkbenchSession();
    QVERIFY(emptySession.projectRoot.isEmpty());
    QVERIFY(emptySession.openFiles.isEmpty());
    QVERIFY(emptySession.untitledDrafts.isEmpty());
    QCOMPARE(emptySession.activeFileIndex, -1);
    QVERIFY(emptySession.bottomPanelId.isEmpty());

    SavedWorkbenchSession session;
    session.projectRoot = QString::fromUtf8("C:/مشروع");
    session.openFiles = {
        QString::fromUtf8("C:/مشروع/main.apy"),
        QString::fromUtf8("C:/مشروع/src/ثانوي.apy"),
    };
    session.untitledDrafts = {
        QString::fromUtf8("اطبع(\"مسودة\")\n"),
        QString::fromUtf8("عدد = 1\n"),
    };
    session.activeFileIndex = 1;
    session.bottomPanelId = QStringLiteral("search");
    empty.saveWorkbenchSession(session);

    SettingsStore reloaded(temp.path() + QStringLiteral("/settings.ini"));
    const SavedWorkbenchSession loaded = reloaded.savedWorkbenchSession();
    QCOMPARE(loaded.projectRoot, QString::fromUtf8("C:/مشروع"));
    QCOMPARE(loaded.openFiles, session.openFiles);
    QCOMPARE(loaded.untitledDrafts, session.untitledDrafts);
    QCOMPARE(loaded.activeFileIndex, 1);
    QCOMPARE(loaded.bottomPanelId, QStringLiteral("search"));
}

void TestProjectSearchRuntime::settingsStorePersistsShortcutSettingsJson()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SettingsStore empty(temp.path() + QStringLiteral("/settings.ini"));
    QVERIFY(empty.shortcutSettingsJson().isEmpty());

    QJsonObject shortcuts;
    shortcuts.insert(QStringLiteral("file.save"), QStringLiteral("Ctrl+Alt+S"));
    shortcuts.insert(QStringLiteral("run.currentFile"), QStringLiteral("F6"));

    QJsonObject exported;
    exported.insert(QStringLiteral("version"), 1);
    exported.insert(QStringLiteral("shortcuts"), shortcuts);
    exported.insert(QStringLiteral("futureMetadata"), QJsonArray {QStringLiteral("kept")});

    empty.saveShortcutSettingsJson(exported);

    SettingsStore reloaded(temp.path() + QStringLiteral("/settings.ini"));
    const QJsonObject loaded = reloaded.shortcutSettingsJson();
    QCOMPARE(loaded.value(QStringLiteral("version")).toInt(), 1);
    QCOMPARE(loaded.value(QStringLiteral("shortcuts")).toObject().value(QStringLiteral("file.save")).toString(), QStringLiteral("Ctrl+Alt+S"));
    QCOMPARE(loaded.value(QStringLiteral("futureMetadata")).toArray().first().toString(), QStringLiteral("kept"));
}

void TestProjectSearchRuntime::packageScriptDerivesMsiArtifactNameFromProductVersion()
{
    QString scriptPath = QDir::current().absoluteFilePath(QStringLiteral("scripts/package.ps1"));
    if (!QFileInfo::exists(scriptPath)) {
        scriptPath = QDir::current().absoluteFilePath(QStringLiteral("../scripts/package.ps1"));
    }
    QFile script(scriptPath);
    QVERIFY2(script.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(scriptPath));

    const QString source = QString::fromUtf8(script.readAll());

    QVERIFY(!source.contains(QStringLiteral("LisanStudio-0.1.0-beta.msi")));
    QVERIFY(source.contains(QStringLiteral("ProductVersion")));
    QVERIFY(source.contains(QStringLiteral("LisanStudio-$ProductVersion-beta.msi")));
}

void TestProjectSearchRuntime::packageScriptStampsInstallerBuildId()
{
    QString scriptPath = QDir::current().absoluteFilePath(QStringLiteral("scripts/package.ps1"));
    if (!QFileInfo::exists(scriptPath)) {
        scriptPath = QDir::current().absoluteFilePath(QStringLiteral("../scripts/package.ps1"));
    }
    QFile script(scriptPath);
    QVERIFY2(script.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(scriptPath));
    const QString packageSource = QString::fromUtf8(script.readAll());

    QVERIFY(packageSource.contains(QStringLiteral("[string]$BuildId")));
    QVERIFY(packageSource.contains(QStringLiteral("rev-parse --short")));
    QVERIFY(packageSource.contains(QStringLiteral("-define \"BuildId=$BuildId\"")));

    QString wxsPath = QDir::current().absoluteFilePath(QStringLiteral("packaging/wix/LisanStudio.wxs"));
    if (!QFileInfo::exists(wxsPath)) {
        wxsPath = QDir::current().absoluteFilePath(QStringLiteral("../packaging/wix/LisanStudio.wxs"));
    }
    QFile wxs(wxsPath);
    QVERIFY2(wxs.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(wxsPath));
    const QString wxsSource = QString::fromUtf8(wxs.readAll());

    QVERIFY(wxsSource.contains(QStringLiteral("Name=\"BuildId\"")));
    QVERIFY(wxsSource.contains(QStringLiteral("$(var.BuildId)")));
}

void TestProjectSearchRuntime::releaseEvidenceScriptDerivesMsiPathFromProductVersion()
{
    QString scriptPath = QDir::current().absoluteFilePath(QStringLiteral("scripts/release-evidence.ps1"));
    if (!QFileInfo::exists(scriptPath)) {
        scriptPath = QDir::current().absoluteFilePath(QStringLiteral("../scripts/release-evidence.ps1"));
    }
    QFile script(scriptPath);
    QVERIFY2(script.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(scriptPath));

    const QString source = QString::fromUtf8(script.readAll());

    QVERIFY(!source.contains(QStringLiteral("LisanStudio-0.1.0-beta.msi")));
    QVERIFY(source.contains(QStringLiteral("ProductVersion")));
    QVERIFY(source.contains(QStringLiteral("LisanStudio-$ProductVersion-beta.msi")));
    QVERIFY(source.contains(QStringLiteral("-MsiPath $msiPath")));
}

void TestProjectSearchRuntime::workspaceSettingsStoreDefaultsWhenMissingOrInvalid()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    WorkspaceSettingsStore store(temp.path());
    QCOMPARE(QDir::toNativeSeparators(store.settingsFilePath()),
        QDir::toNativeSeparators(QDir(temp.path()).filePath(QStringLiteral(".lisan-workspace/settings.json"))));

    QString error;
    WorkspaceSettings settings = store.load(&error);
    QVERIFY(error.isEmpty());
    QVERIFY(!settings.trusted);
    QVERIFY(!settings.trimTrailingWhitespaceOnSave);
    QVERIFY(settings.defaultRunWorkingDirectory.isEmpty());

    QFile invalid(store.settingsFilePath());
    QVERIFY(QDir().mkpath(QFileInfo(invalid).absolutePath()));
    QVERIFY(invalid.open(QIODevice::WriteOnly | QIODevice::Text));
    invalid.write("{");
    invalid.close();

    settings = store.load(&error);
    QVERIFY(!error.isEmpty());
    QVERIFY(!settings.trusted);
    QVERIFY(!settings.trimTrailingWhitespaceOnSave);
    QVERIFY(settings.defaultRunWorkingDirectory.isEmpty());
}

void TestProjectSearchRuntime::workspaceSettingsStorePersistsTrustAndEditorPreferences()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    WorkspaceSettingsStore store(temp.path());
    WorkspaceSettings settings;
    settings.trusted = true;
    settings.trimTrailingWhitespaceOnSave = true;
    settings.defaultRunWorkingDirectory = QStringLiteral("src");

    QString error;
    QVERIFY2(store.save(settings, &error), qPrintable(error));

    WorkspaceSettingsStore reloaded(temp.path());
    const WorkspaceSettings loaded = reloaded.load(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(loaded.trusted);
    QVERIFY(loaded.trimTrailingWhitespaceOnSave);
    QCOMPARE(loaded.defaultRunWorkingDirectory, QStringLiteral("src"));
}

void TestProjectSearchRuntime::settingsDialogModelBuildsUiStateFromStoreAndDiagnostics()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SettingsStore store(temp.path() + QStringLiteral("/settings.ini"));
    store.setEditorFontFamily(QString::fromUtf8("Cascadia Code"));
    store.setEditorFontSize(18);
    store.setThemePreference(QStringLiteral("light"));
    store.addRecentProject(QString::fromUtf8("C:/مشروع"));

    RuntimeDiagnostics diagnostics;
    diagnostics.pythonExecutable = QStringLiteral("C:/runtime/python/python.exe");
    diagnostics.statusText = QString::fromUtf8("جاهز: lughat-althuban 1.0");
    diagnostics.runModuleAvailable = true;
    diagnostics.lintModuleAvailable = false;
    diagnostics.formatModuleAvailable = true;

    const SettingsDialogState state = SettingsDialogModel::build(
        store,
        diagnostics,
        {QStringLiteral("Segoe UI"), QStringLiteral("Tahoma")});

    QCOMPARE(state.categories, QStringList({
        QString::fromUtf8("المحرر"),
        QString::fromUtf8("التشغيل"),
        QString::fromUtf8("المشاريع"),
        QString::fromUtf8("الاختصارات"),
    }));
    QCOMPARE(state.editorFontFamilies, QStringList({QStringLiteral("Segoe UI"), QStringLiteral("Tahoma")}));
    QCOMPARE(state.selectedEditorFontFamily, QStringLiteral("Segoe UI"));
    QCOMPARE(state.editorFontSize, 18);
    QCOMPARE(state.themeLabel, QString::fromUtf8("فاتح"));
    QCOMPARE(state.runtimePythonPath, QStringLiteral("C:\\runtime\\python\\python.exe"));
    QCOMPARE(state.runtimePackageStatus, QString::fromUtf8("جاهز: lughat-althuban 1.0"));
    QCOMPARE(state.runtimeRunStatus, QString::fromUtf8("جاهز"));
    QCOMPARE(state.runtimeLintStatus, QString::fromUtf8("غير متوفر"));
    QCOMPARE(state.runtimeFormatStatus, QString::fromUtf8("جاهز"));
    QCOMPARE(state.recentProjects, QStringList({QString::fromUtf8("C:\\مشروع")}));
}

void TestProjectSearchRuntime::documentFileIoReadsUtf8AndRecordsIdentity()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = writeFile(QDir(temp.path()), QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    QString error;
    const DocumentLoadResult result = DocumentFileIO::loadUtf8(path, &error);

    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(result.text, QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));
    QCOMPARE(result.identity.path, QFileInfo(path).absoluteFilePath());
    QVERIFY(result.identity.sizeBytes > 0);
    QVERIFY(result.identity.lastModifiedUtc.isValid());
    QCOMPARE(result.encoding, DocumentEncoding::Utf8);
    QCOMPARE(result.lineEnding, DocumentLineEnding::Lf);
}

void TestProjectSearchRuntime::documentFileIoWritesAtomicallyAndPreservesUtf8()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = QDir(temp.path()).filePath(QStringLiteral("main.apy"));

    QString error;
    const bool saved = DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("اطبع(\"مرحبا\")\n"), &error);

    QVERIFY2(saved, qPrintable(error));
    QVERIFY(QFileInfo::exists(path));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(file.readAll()), QString::fromUtf8("اطبع(\"مرحبا\")\n"));
    QVERIFY(QDir(temp.path()).entryList(QStringList(QStringLiteral("*.tmp")), QDir::Files).isEmpty());
}

void TestProjectSearchRuntime::documentFileIoRejectsInvalidUtf8ByPolicy()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = QDir(temp.path()).filePath(QStringLiteral("bad.apy"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray::fromHex("fffe4100"));
    file.close();

    QString error;
    const DocumentLoadResult result = DocumentFileIO::loadUtf8(path, &error);

    QVERIFY(result.text.isEmpty());
    QVERIFY(error.contains(QString::fromUtf8("UTF-8")));
}

void TestProjectSearchRuntime::editorFindServiceFindsArabicEnglishAndMixedMatches()
{
    const QString text = QString::fromUtf8(
        "عدد = 1\n"
        "path = \"C:/Users/Admin/مشروع/main.apy\"\n"
        "اطبع(عدد)\n"
        "اسم = \"سارة\"\u202E\n");

    const auto arabic = EditorFindService::findAll(text, QString::fromUtf8("عدد"));
    QCOMPARE(arabic.size(), 2);
    QCOMPARE(arabic.first().start, 0);
    QCOMPARE(arabic.first().length, QString::fromUtf8("عدد").size());

    const auto english = EditorFindService::findAll(text, QStringLiteral("path"));
    QCOMPARE(english.size(), 1);

    const auto mixed = EditorFindService::findAll(text, QString::fromUtf8("مشروع/main.apy"));
    QCOMPARE(mixed.size(), 1);

    const auto hiddenAdjacent = EditorFindService::findAll(text, QString::fromUtf8("سارة"));
    QCOMPARE(hiddenAdjacent.size(), 1);
}

void TestProjectSearchRuntime::editorFindServiceReplacesCurrentAndAllMatches()
{
    QString text = QString::fromUtf8("عدد = 1\nاطبع(عدد)\n");
    const auto matches = EditorFindService::findAll(text, QString::fromUtf8("عدد"));
    QCOMPARE(matches.size(), 2);

    QVERIFY(EditorFindService::replaceAt(&text, matches.first(), QString::fromUtf8("قيمة")));
    QCOMPARE(text, QString::fromUtf8("قيمة = 1\nاطبع(عدد)\n"));

    int replaced = 0;
    text = EditorFindService::replaceAll(text, QString::fromUtf8("عدد"), QString::fromUtf8("قيمة"), &replaced);
    QCOMPARE(replaced, 1);
    QCOMPARE(text, QString::fromUtf8("قيمة = 1\nاطبع(قيمة)\n"));
}

void TestProjectSearchRuntime::apySnippetServiceListsArabicFirstSnippets()
{
    const QVector<ApySnippet> snippets = ApySnippetService::snippets();

    QVERIFY(snippets.size() >= 2);
    QCOMPARE(snippets.first().id, QStringLiteral("apy.print"));
    QCOMPARE(snippets.first().title, QString::fromUtf8("اطبع"));
    QCOMPARE(snippets.first().body, QString::fromUtf8("اطبع(\"\")"));
    QCOMPARE(snippets.first().cursorOffset, QString::fromUtf8("اطبع(\"").size());
}

void TestProjectSearchRuntime::apySnippetServiceFindsSnippetById()
{
    ApySnippet snippet;

    QVERIFY(ApySnippetService::snippetById(QStringLiteral("apy.if"), &snippet));
    QCOMPARE(snippet.title, QString::fromUtf8("إذا"));
    QCOMPARE(snippet.body, QString::fromUtf8("اذا شرط:\n    \n"));
    QVERIFY(snippet.cursorOffset > 0);
    QVERIFY(!ApySnippetService::snippetById(QStringLiteral("apy.missing"), &snippet));
}

void TestProjectSearchRuntime::projectReplaceServicePreviewsArabicMixedMatches()
{
    const QString text = QString::fromUtf8(
        "عدد = 1\n"
        "path = \"C:/Users/Admin/مشروع/main.apy\"\n"
        "اطبع(عدد)\n");

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewText(
        QStringLiteral("main.apy"),
        text,
        QString::fromUtf8("عدد"),
        QString::fromUtf8("قيمة"));

    QCOMPARE(preview.rows.size(), 2);
    QCOMPARE(preview.totalMatches, 2);
    QCOMPARE(preview.summaries.size(), 1);
    QCOMPARE(preview.summaries.first().matchCount, 2);
    QCOMPARE(preview.rows.first().line, 1);
    QCOMPARE(preview.rows.first().before, QString::fromUtf8("عدد = 1"));
    QCOMPARE(preview.rows.first().after, QString::fromUtf8("قيمة = 1"));
    QCOMPARE(preview.rows.last().after, QString::fromUtf8("اطبع(قيمة)"));
}

void TestProjectSearchRuntime::projectReplaceServiceSkipsIgnoredDirectories()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString mainPath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("اطبع(عدد)\n"));
    writeFile(root, QStringLiteral("build/generated.apy"), QString::fromUtf8("اطبع(عدد)\n"));
    writeFile(root, QStringLiteral(".cache/hidden.apy"), QString::fromUtf8("اطبع(عدد)\n"));

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewProject(
        root.absolutePath(),
        QString::fromUtf8("عدد"),
        QString::fromUtf8("قيمة"));

    QCOMPARE(preview.rows.size(), 1);
    QCOMPARE(QDir::toNativeSeparators(preview.rows.first().path), QDir::toNativeSeparators(mainPath));
    QCOMPARE(preview.rows.first().after, QString::fromUtf8("اطبع(قيمة)"));
    QCOMPARE(preview.totalMatches, 1);
}

void TestProjectSearchRuntime::projectReplaceServiceReportsTruncationWhenCapFires()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());

    constexpr int maxScannedFiles = 10;
    for (int i = 0; i < 12; ++i) {
        writeFile(
            root,
            QStringLiteral("file-%1.apy").arg(i, 2, 10, QLatin1Char('0')),
            QStringLiteral("needle\n"));
    }

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewProject(
        root.absolutePath(),
        QStringLiteral("needle"),
        QStringLiteral("replacement"),
        1000,
        maxScannedFiles);

    QCOMPARE(preview.rows.size(), maxScannedFiles);
    QVERIFY(preview.truncatedAtFileCap);
}

void TestProjectSearchRuntime::projectReplaceServiceMergesImmediateRowsBeforeDiskRows()
{
    const QVector<ProjectReplacePreviewRow> immediateRows = {
        {QStringLiteral("main.apy"), 1, QString::fromUtf8("عدد = 1"), QString::fromUtf8("قيمة = 1"), 1},
        {QStringLiteral("scratch.apy"), 2, QString::fromUtf8("اطبع(عدد)"), QString::fromUtf8("اطبع(قيمة)"), 1},
    };
    const QVector<ProjectReplacePreviewRow> projectRows = {
        {QStringLiteral("main.apy"), 1, QString::fromUtf8("عدد = 1"), QString::fromUtf8("قيمة = 1"), 1},
        {QStringLiteral("other.apy"), 3, QString::fromUtf8("عدد"), QString::fromUtf8("قيمة"), 1},
    };

    const QVector<ProjectReplacePreviewRow> rows = ProjectReplaceService::mergePreviewRows(immediateRows, projectRows);

    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows.at(0).path, QStringLiteral("main.apy"));
    QCOMPARE(rows.at(1).path, QStringLiteral("scratch.apy"));
    QCOMPARE(rows.at(2).path, QStringLiteral("other.apy"));
}

void TestProjectSearchRuntime::projectReplaceSelectionAcceptsRowsByDefault()
{
    const QVector<ProjectReplacePreviewRow> rows = {
        {QStringLiteral("main.apy"), 1, QString::fromUtf8("عدد = 1"), QString::fromUtf8("قيمة = 1"), 1},
        {QStringLiteral("main.apy"), 2, QString::fromUtf8("اطبع(عدد)"), QString::fromUtf8("اطبع(قيمة)"), 1},
    };

    const ProjectReplaceSelectionState selection = ProjectReplaceService::selectionFromRows(rows);

    QVERIFY(selection.isRowAccepted(0));
    QVERIFY(selection.isRowAccepted(1));
    QCOMPARE(selection.acceptedRows().size(), 2);
    QCOMPARE(selection.acceptedMatchCount(), 2);
}

void TestProjectSearchRuntime::projectReplaceSelectionRejectsSingleRowsAndWholeFiles()
{
    const QVector<ProjectReplacePreviewRow> rows = {
        {QStringLiteral("main.apy"), 1, QString::fromUtf8("عدد = 1"), QString::fromUtf8("قيمة = 1"), 1},
        {QStringLiteral("main.apy"), 2, QString::fromUtf8("اطبع(عدد)"), QString::fromUtf8("اطبع(قيمة)"), 1},
        {QStringLiteral("other.apy"), 1, QString::fromUtf8("عدد"), QString::fromUtf8("قيمة"), 1},
    };

    ProjectReplaceSelectionState selection = ProjectReplaceService::selectionFromRows(rows);

    selection.setRowAccepted(1, false);
    QVERIFY(selection.isRowAccepted(0));
    QVERIFY(!selection.isRowAccepted(1));
    QVERIFY(selection.isRowAccepted(2));
    QCOMPARE(selection.acceptedRows().size(), 2);
    QCOMPARE(selection.acceptedMatchCount(), 2);

    selection.setFileAccepted(QStringLiteral("main.apy"), false);
    QVERIFY(!selection.isRowAccepted(0));
    QVERIFY(!selection.isRowAccepted(1));
    QVERIFY(selection.isRowAccepted(2));
    QCOMPARE(selection.acceptedRows().size(), 1);
    QCOMPARE(selection.acceptedRows().first().path, QStringLiteral("other.apy"));

    selection.setFileAccepted(QStringLiteral("main.apy"), true);
    QVERIFY(selection.isRowAccepted(0));
    QVERIFY(selection.isRowAccepted(1));
    QVERIFY(selection.isRowAccepted(2));
    QCOMPARE(selection.acceptedRows().size(), 3);
}

void TestProjectSearchRuntime::projectReplaceServiceAppliesAcceptedRowsAtomically()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString path = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewProject(root.absolutePath(), QString::fromUtf8("عدد"), QString::fromUtf8("قيمة"));
    ProjectReplaceSelectionState selection = ProjectReplaceService::selectionFromRows(preview.rows);
    selection.setRowAccepted(1, false);

    QString error;
    const ProjectReplaceApplyResult result = ProjectReplaceService::applyAcceptedRows(selection.acceptedRows(), &error);

    QVERIFY2(result.succeeded, qPrintable(error));
    QCOMPARE(result.filesChanged, 1);
    QCOMPARE(result.rowsApplied, 1);

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QString::fromUtf8("قيمة = 1\nاطبع(عدد)\n").toUtf8());
}

void TestProjectSearchRuntime::projectReplacePreservesLeadingWhitespace()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString path = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("    print(عدد)\n"));

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewProject(root.absolutePath(), QStringLiteral("print"), QString::fromUtf8("اطبع"));

    QString error;
    const ProjectReplaceApplyResult result = ProjectReplaceService::applyAcceptedRows(preview.rows, &error);

    QVERIFY2(result.succeeded, qPrintable(error));
    QCOMPARE(readRawFile(path), QString::fromUtf8("    اطبع(عدد)\n").toUtf8());
}

void TestProjectSearchRuntime::projectReplacePreservesCRLF()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString path = writeRawFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\r\nاطبع(عدد)\r\n").toUtf8());

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewProject(root.absolutePath(), QString::fromUtf8("عدد"), QString::fromUtf8("قيمة"));

    QString error;
    const ProjectReplaceApplyResult result = ProjectReplaceService::applyAcceptedRows(preview.rows, &error);

    QVERIFY2(result.succeeded, qPrintable(error));
    const QByteArray diskBytes = readRawFile(path);
    QCOMPARE(diskBytes, QString::fromUtf8("قيمة = 1\r\nاطبع(قيمة)\r\n").toUtf8());
    for (int i = 0; i < diskBytes.size(); ++i) {
        if (diskBytes.at(i) == '\n') {
            QVERIFY(i > 0);
            QCOMPARE(diskBytes.at(i - 1), '\r');
        }
        if (diskBytes.at(i) == '\r') {
            QVERIFY(i + 1 < diskBytes.size());
            QCOMPARE(diskBytes.at(i + 1), '\n');
        }
    }
}

void TestProjectSearchRuntime::projectReplaceRejectsStaleWhitespaceChange()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString path = writeFile(root, QStringLiteral("main.apy"), QStringLiteral("    print(x)\n"));

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewProject(root.absolutePath(), QStringLiteral("print"), QString::fromUtf8("اطبع"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(path, QStringLiteral("\tprint(x)\n")));

    QString error;
    const ProjectReplaceApplyResult result = ProjectReplaceService::applyAcceptedRows(preview.rows, &error);

    QVERIFY(!result.succeeded);
    QVERIFY(error.contains(QString::fromUtf8("تغير")));
    QCOMPARE(readRawFile(path), QByteArray("\tprint(x)\n"));
}

void TestProjectSearchRuntime::projectReplaceServicePreservesIndentedMatchedLines()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString path = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8(
        "اذا شرط:\n"
        "    اطبع(عدد)\n"
        "    اطبع(\"باقي\")\n"));

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewProject(root.absolutePath(), QString::fromUtf8("عدد"), QString::fromUtf8("قيمة"));

    QString error;
    const ProjectReplaceApplyResult result = ProjectReplaceService::applyAcceptedRows(preview.rows, &error);

    QVERIFY2(result.succeeded, qPrintable(error));
    QCOMPARE(readRawFile(path), QString::fromUtf8(
        "اذا شرط:\n"
        "    اطبع(قيمة)\n"
        "    اطبع(\"باقي\")\n").toUtf8());
}

void TestProjectSearchRuntime::projectReplaceServicePreservesCrlfLineEndings()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString path = writeRawFile(root, QStringLiteral("main.apy"), QString::fromUtf8(
        "عدد = 1\r\n"
        "    اطبع(عدد)\r\n"
        "اطبع(\"انتهى\")\r\n").toUtf8());

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewProject(root.absolutePath(), QString::fromUtf8("عدد"), QString::fromUtf8("قيمة"));

    QString error;
    const ProjectReplaceApplyResult result = ProjectReplaceService::applyAcceptedRows(preview.rows, &error);

    QVERIFY2(result.succeeded, qPrintable(error));
    QCOMPARE(readRawFile(path), QString::fromUtf8(
        "قيمة = 1\r\n"
        "    اطبع(قيمة)\r\n"
        "اطبع(\"انتهى\")\r\n").toUtf8());
}

void TestProjectSearchRuntime::projectReplaceServiceRejectsStaleRowsWithoutWriting()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString path = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    ProjectReplaceService service;
    const ProjectReplacePreview preview = service.previewProject(root.absolutePath(), QString::fromUtf8("عدد"), QString::fromUtf8("قيمة"));
    QVERIFY(DocumentFileIO::saveUtf8Atomically(path, QString::fromUtf8("عدد = 2\nاطبع(عدد)\n")));

    QString error;
    const ProjectReplaceApplyResult result = ProjectReplaceService::applyAcceptedRows(preview.rows, &error);

    QVERIFY(!result.succeeded);
    QVERIFY(error.contains(QString::fromUtf8("تغير")));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QString::fromUtf8("عدد = 2\nاطبع(عدد)\n").toUtf8());
}

QTEST_MAIN(TestProjectSearchRuntime)
#include "TestProjectSearchRuntime.moc"
