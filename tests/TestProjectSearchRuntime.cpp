#include <QtTest/QtTest>

#include "DocumentFileIO.h"
#include "EditorFindService.h"
#include "ProjectModel.h"
#include "ProjectFileOperations.h"
#include "RuntimeProblemParser.h"
#include "RuntimeRunner.h"
#include "SearchService.h"
#include "SettingsDialogModel.h"
#include "SettingsStore.h"

class TestProjectSearchRuntime : public QObject
{
    Q_OBJECT

private slots:
    void projectModelIgnoresBuildAndCacheDirectories();
    void projectModelValidatesSafeChildNames();
    void projectModelDetectsPathsCoveredByProjectOperations();
    void projectFileOperationsRejectUnsafeTargetsAndCollisions();
    void projectFileOperationsProtectRootAndResolveContainingFolder();
    void searchServiceFindsUtf8ArabicMatches();
    void searchServiceFindsUnsavedEditorMatches();
    void searchServiceMergesImmediateRowsBeforeProjectRows();
    void searchServiceStopsBeforeHugeProjectTail();
    void runtimeRunnerBuildsExplicitArgumentList();
    void runtimeRunnerBuildsLaunchPlanWithSummaryText();
    void runtimeRunnerCanUseExplicitProjectWorkingDirectory();
    void runtimeRunnerUsesIsolatedUtf8PythonEnvironment();
    void runtimeRunnerReportsMissingBundledPythonDiagnostics();
    void runtimeProblemParserExtractsArabicSyntaxLine();
    void settingsStorePersistsArabicFontAndRecentProject();
    void settingsDialogModelBuildsUiStateFromStoreAndDiagnostics();
    void documentFileIoReadsUtf8AndRecordsIdentity();
    void documentFileIoWritesAtomicallyAndPreservesUtf8();
    void documentFileIoRejectsInvalidUtf8ByPolicy();
    void editorFindServiceFindsArabicEnglishAndMixedMatches();
    void editorFindServiceReplacesCurrentAndAllMatches();
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

void TestProjectSearchRuntime::settingsDialogModelBuildsUiStateFromStoreAndDiagnostics()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SettingsStore store(temp.path() + QStringLiteral("/settings.ini"));
    store.setEditorFontFamily(QString::fromUtf8("Cascadia Code"));
    store.setEditorFontSize(18);
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

    QCOMPARE(state.categories, QStringList({QString::fromUtf8("المحرر"), QString::fromUtf8("التشغيل"), QString::fromUtf8("المشاريع")}));
    QCOMPARE(state.editorFontFamilies, QStringList({QStringLiteral("Segoe UI"), QStringLiteral("Tahoma")}));
    QCOMPARE(state.selectedEditorFontFamily, QStringLiteral("Segoe UI"));
    QCOMPARE(state.editorFontSize, 18);
    QCOMPARE(state.themeLabel, QString::fromUtf8("داكن مستقبلي"));
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

QTEST_MAIN(TestProjectSearchRuntime)
#include "TestProjectSearchRuntime.moc"
