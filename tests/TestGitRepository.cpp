#include <QtTest/QtTest>

#include "GitRepository.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>

class TestGitRepository : public QObject
{
    Q_OBJECT

private slots:
    void reportsCurrentBranchForCleanRepository();
    void reportsUtf8DirtyAndUntrackedFiles();
    void returnsUnifiedDiffForModifiedUtf8File();
    void stagesAndUnstagesUtf8File();
    void commitsStagedUtf8FileAndCleansRepository();
};

static void runGit(const QDir &root, const QStringList &arguments)
{
    QProcess git;
    git.setWorkingDirectory(root.absolutePath());
    git.start(QStringLiteral("git"), arguments);
    QVERIFY2(git.waitForFinished(10000), qPrintable(QStringLiteral("git timed out: %1").arg(arguments.join(QLatin1Char(' ')))));
    QCOMPARE(git.exitCode(), 0);
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

QTEST_MAIN(TestGitRepository)
#include "TestGitRepository.moc"
