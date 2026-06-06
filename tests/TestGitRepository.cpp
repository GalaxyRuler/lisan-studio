#include <QtTest/QtTest>

#include "GitRepository.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QUrl>

class TestGitRepository : public QObject
{
    Q_OBJECT

private slots:
    void reportsCurrentBranchForCleanRepository();
    void reportsUtf8DirtyAndUntrackedFiles();
    void returnsUnifiedDiffForModifiedUtf8File();
    void stagesAndUnstagesUtf8File();
    void commitsStagedUtf8FileAndCleansRepository();
    void pushesCurrentBranchToLocalBareRemote();
    void fetchesRemoteTrackingBranchFromLocalBareRemote();
    void pullsFastForwardFromLocalBareRemote();
    void createsSwitchesAndDeletesLocalBranch();
    void fastForwardMergesLocalBranch();
    void returnsCommitHistoryForCurrentBranch();
    void returnsBlameLinesForUtf8File();
    void returnsDiffForHistoricalCommit();
};

static void runGit(const QDir &root, const QStringList &arguments)
{
    QProcess git;
    git.setWorkingDirectory(root.absolutePath());
    git.start(QStringLiteral("git"), arguments);
    QVERIFY2(git.waitForFinished(10000), qPrintable(QStringLiteral("git timed out: %1").arg(arguments.join(QLatin1Char(' ')))));
    if (git.exitCode() != 0) {
        qFatal("%s", qPrintable(QStringLiteral("git failed: %1\n%2")
            .arg(arguments.join(QLatin1Char(' ')), QString::fromUtf8(git.readAllStandardError()))));
    }
}

static QString runGitOutput(const QDir &root, const QStringList &arguments)
{
    QProcess git;
    git.setWorkingDirectory(root.absolutePath());
    git.start(QStringLiteral("git"), arguments);
    if (!git.waitForFinished(10000)) {
        qFatal("git command timed out");
    }
    if (git.exitCode() != 0) {
        qFatal("git command failed");
    }
    return QString::fromUtf8(git.readAllStandardOutput()).trimmed();
}

static QString writeFile(const QDir &root, const QString &relativePath, const QString &text)
{
    const QFileInfo info(root.filePath(relativePath));
    QDir().mkpath(info.absolutePath());
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qFatal("could not write test file");
    }
    file.write(text.toUtf8());
    return info.absoluteFilePath();
}

static void createInitialCommit(const QDir &root)
{
    runGit(root, {QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main")});
    runGit(root, {QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Lisan Tester")});
    runGit(root, {QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("tester@example.invalid")});
    writeFile(root, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"أول\")\n"));
    runGit(root, {QStringLiteral("add"), QStringLiteral(".")});
    runGit(root, {QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("initial")});
}

void TestGitRepository::reportsCurrentBranchForCleanRepository()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));

    QCOMPARE(QDir::toNativeSeparators(repository.rootPath()), QDir::toNativeSeparators(root.absolutePath()));
    QCOMPARE(repository.currentBranch(), QStringLiteral("main"));
    QVERIFY(!repository.hasChanges());
    QCOMPARE(repository.statusEntries().size(), 0);
}

void TestGitRepository::reportsUtf8DirtyAndUntrackedFiles()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);
    writeFile(root, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"تعديل\")\n"));
    writeFile(root, QStringLiteral("src/جديد.apy"), QString::fromUtf8("اطبع(\"جديد\")\n"));

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));

    const QVector<GitStatusEntry> entries = repository.statusEntries();
    QCOMPARE(entries.size(), 2);
    QVERIFY(std::any_of(entries.cbegin(), entries.cend(), [](const GitStatusEntry &entry) {
        return entry.relativePath == QString::fromUtf8("src/برنامج.apy")
            && entry.state == GitFileState::Modified
            && !entry.staged;
    }));
    QVERIFY(std::any_of(entries.cbegin(), entries.cend(), [](const GitStatusEntry &entry) {
        return entry.relativePath == QString::fromUtf8("src/جديد.apy")
            && entry.state == GitFileState::Untracked
            && !entry.staged;
    }));
    QVERIFY(repository.hasChanges());
}

void TestGitRepository::returnsUnifiedDiffForModifiedUtf8File()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);
    writeFile(root, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"تعديل\")\n"));

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));

    const QString diff = repository.diffForFile(QString::fromUtf8("src/برنامج.apy"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY2(diff.contains(QStringLiteral("diff --git")), qPrintable(diff));
    QVERIFY2(diff.contains(QStringLiteral("--- ")), qPrintable(diff));
    QVERIFY2(diff.contains(QStringLiteral("+++ ")), qPrintable(diff));
    QVERIFY2(diff.contains(QString::fromUtf8("-اطبع(\"أول\")")), qPrintable(diff));
    QVERIFY2(diff.contains(QString::fromUtf8("+اطبع(\"تعديل\")")), qPrintable(diff));
}

void TestGitRepository::stagesAndUnstagesUtf8File()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);
    writeFile(root, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"تعديل\")\n"));

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));
    QVERIFY2(repository.stageFile(QString::fromUtf8("src/برنامج.apy"), &error), qPrintable(error));

    QVector<GitStatusEntry> entries = repository.statusEntries(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().relativePath, QString::fromUtf8("src/برنامج.apy"));
    QCOMPARE(entries.first().state, GitFileState::Modified);
    QVERIFY(entries.first().staged);

    QVERIFY2(repository.unstageFile(QString::fromUtf8("src/برنامج.apy"), &error), qPrintable(error));
    entries = repository.statusEntries(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().relativePath, QString::fromUtf8("src/برنامج.apy"));
    QVERIFY(!entries.first().staged);
}

void TestGitRepository::commitsStagedUtf8FileAndCleansRepository()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);
    writeFile(root, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"تعديل\")\n"));

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));
    QVERIFY2(repository.stageFile(QString::fromUtf8("src/برنامج.apy"), &error), qPrintable(error));
    QVERIFY2(repository.commitStaged(QString::fromUtf8("تعديل عربي"), &error), qPrintable(error));

    QVERIFY(!repository.hasChanges(&error));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    runGit(root, {QStringLiteral("log"), QStringLiteral("-1"), QStringLiteral("--format=%s")});
}

void TestGitRepository::pushesCurrentBranchToLocalBareRemote()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir workspace(temp.path());
    QDir root(workspace.filePath(QStringLiteral("repo")));
    QDir bare(workspace.filePath(QStringLiteral("origin.git")));
    QDir().mkpath(root.absolutePath());
    runGit(workspace, {QStringLiteral("init"), QStringLiteral("--bare"), bare.absolutePath()});
    createInitialCommit(root);
    runGit(root, {QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), QUrl::fromLocalFile(bare.absolutePath()).toString()});

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));
    QVERIFY2(repository.pushCurrentBranch(QStringLiteral("origin"), &error), qPrintable(error));

    QCOMPARE(runGitOutput(workspace, {QStringLiteral("--git-dir"), bare.absolutePath(), QStringLiteral("log"), QStringLiteral("-1"), QStringLiteral("--format=%s"), QStringLiteral("main")}), QStringLiteral("initial"));
}

void TestGitRepository::fetchesRemoteTrackingBranchFromLocalBareRemote()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir workspace(temp.path());
    QDir seed(workspace.filePath(QStringLiteral("seed")));
    QDir local(workspace.filePath(QStringLiteral("local")));
    QDir bare(workspace.filePath(QStringLiteral("origin.git")));
    QDir().mkpath(seed.absolutePath());
    runGit(workspace, {QStringLiteral("init"), QStringLiteral("--bare"), bare.absolutePath()});
    createInitialCommit(seed);
    runGit(seed, {QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), bare.absolutePath()});
    runGit(seed, {QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), QStringLiteral("main")});
    runGit(workspace, {QStringLiteral("--git-dir"), bare.absolutePath(), QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"), QStringLiteral("refs/heads/main")});
    runGit(workspace, {QStringLiteral("clone"), bare.absolutePath(), local.absolutePath()});
    runGit(local, {QStringLiteral("remote"), QStringLiteral("set-url"), QStringLiteral("origin"), QUrl::fromLocalFile(bare.absolutePath()).toString()});

    writeFile(seed, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"ثان\")\n"));
    runGit(seed, {QStringLiteral("add"), QStringLiteral(".")});
    runGit(seed, {QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("second")});
    runGit(seed, {QStringLiteral("push"), QStringLiteral("origin"), QStringLiteral("main")});

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(local.absolutePath(), &error), qPrintable(error));
    QVERIFY2(repository.fetchRemote(QStringLiteral("origin"), &error), qPrintable(error));

    QCOMPARE(runGitOutput(local, {QStringLiteral("log"), QStringLiteral("-1"), QStringLiteral("--format=%s"), QStringLiteral("origin/main")}), QStringLiteral("second"));
}

void TestGitRepository::pullsFastForwardFromLocalBareRemote()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir workspace(temp.path());
    QDir seed(workspace.filePath(QStringLiteral("seed")));
    QDir local(workspace.filePath(QStringLiteral("local")));
    QDir bare(workspace.filePath(QStringLiteral("origin.git")));
    QDir().mkpath(seed.absolutePath());
    runGit(workspace, {QStringLiteral("init"), QStringLiteral("--bare"), bare.absolutePath()});
    createInitialCommit(seed);
    runGit(seed, {QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), bare.absolutePath()});
    runGit(seed, {QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), QStringLiteral("main")});
    runGit(workspace, {QStringLiteral("--git-dir"), bare.absolutePath(), QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"), QStringLiteral("refs/heads/main")});
    runGit(workspace, {QStringLiteral("clone"), bare.absolutePath(), local.absolutePath()});
    runGit(local, {QStringLiteral("remote"), QStringLiteral("set-url"), QStringLiteral("origin"), QUrl::fromLocalFile(bare.absolutePath()).toString()});

    writeFile(seed, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"ثان\")\n"));
    runGit(seed, {QStringLiteral("add"), QStringLiteral(".")});
    runGit(seed, {QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("second")});
    runGit(seed, {QStringLiteral("push"), QStringLiteral("origin"), QStringLiteral("main")});

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(local.absolutePath(), &error), qPrintable(error));
    QVERIFY2(repository.pullFastForward(QStringLiteral("origin"), &error), qPrintable(error));

    QCOMPARE(runGitOutput(local, {QStringLiteral("log"), QStringLiteral("-1"), QStringLiteral("--format=%s")}), QStringLiteral("second"));
    QVERIFY(!repository.hasChanges(&error));
}

void TestGitRepository::createsSwitchesAndDeletesLocalBranch()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));
    QVERIFY2(repository.createBranch(QStringLiteral("feature/git-ui"), &error), qPrintable(error));
    QVERIFY(repository.localBranches().contains(QStringLiteral("feature/git-ui")));

    QVERIFY2(repository.checkoutBranch(QStringLiteral("feature/git-ui"), &error), qPrintable(error));
    QCOMPARE(repository.currentBranch(), QStringLiteral("feature/git-ui"));

    QVERIFY2(repository.checkoutBranch(QStringLiteral("main"), &error), qPrintable(error));
    QVERIFY2(repository.deleteBranch(QStringLiteral("feature/git-ui"), &error), qPrintable(error));
    QVERIFY(!repository.localBranches().contains(QStringLiteral("feature/git-ui")));
}

void TestGitRepository::fastForwardMergesLocalBranch()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));
    QVERIFY2(repository.createBranch(QStringLiteral("feature/git-ui"), &error), qPrintable(error));
    QVERIFY2(repository.checkoutBranch(QStringLiteral("feature/git-ui"), &error), qPrintable(error));
    writeFile(root, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"فرع\")\n"));
    QVERIFY2(repository.stageFile(QString::fromUtf8("src/برنامج.apy"), &error), qPrintable(error));
    QVERIFY2(repository.commitStaged(QStringLiteral("branch work"), &error), qPrintable(error));

    QVERIFY2(repository.checkoutBranch(QStringLiteral("main"), &error), qPrintable(error));
    QVERIFY2(repository.mergeFastForward(QStringLiteral("feature/git-ui"), &error), qPrintable(error));

    QCOMPARE(repository.currentBranch(), QStringLiteral("main"));
    QCOMPARE(runGitOutput(root, {QStringLiteral("log"), QStringLiteral("-1"), QStringLiteral("--format=%s")}), QStringLiteral("branch work"));
    QVERIFY(!repository.hasChanges(&error));
}

void TestGitRepository::returnsCommitHistoryForCurrentBranch()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);
    writeFile(root, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"ثان\")\n"));
    runGit(root, {QStringLiteral("add"), QStringLiteral(".")});
    runGit(root, {QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("second")});

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));

    const QVector<GitCommitSummary> history = repository.commitHistory(10, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(history.size() >= 2);
    QCOMPARE(history.first().summary, QStringLiteral("second"));
    QVERIFY(!history.first().shortId.isEmpty());
}

void TestGitRepository::returnsBlameLinesForUtf8File()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);
    writeFile(root, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"ثان\")\n"));
    runGit(root, {QStringLiteral("add"), QStringLiteral(".")});
    runGit(root, {QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("second")});

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));

    const QVector<GitBlameLine> blame = repository.blameFile(QString::fromUtf8("src/برنامج.apy"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(blame.size(), 1);
    QCOMPARE(blame.first().lineNumber, 1);
    QCOMPARE(blame.first().summary, QStringLiteral("second"));
}

void TestGitRepository::returnsDiffForHistoricalCommit()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    createInitialCommit(root);
    writeFile(root, QStringLiteral("src/برنامج.apy"), QString::fromUtf8("اطبع(\"ثان\")\n"));
    runGit(root, {QStringLiteral("add"), QStringLiteral(".")});
    runGit(root, {QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("second")});

    GitRepository repository;
    QString error;
    QVERIFY2(repository.open(root.absolutePath(), &error), qPrintable(error));
    const QVector<GitCommitSummary> history = repository.commitHistory(1, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(history.size(), 1);

    const QString diff = repository.diffForCommit(history.first().id, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY2(diff.contains(QString::fromUtf8("+اطبع(\"ثان\")")), qPrintable(diff));
}

QTEST_MAIN(TestGitRepository)
#include "TestGitRepository.moc"
