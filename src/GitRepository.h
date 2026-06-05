#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct git_repository;

enum class GitFileState
{
    Added,
    Modified,
    Deleted,
    Renamed,
    TypeChanged,
    Untracked,
    Conflicted,
};

struct GitStatusEntry
{
    QString relativePath;
    GitFileState state = GitFileState::Modified;
    bool staged = false;
};

class GitRepository final
{
public:
    GitRepository();
    ~GitRepository();

    GitRepository(const GitRepository &) = delete;
    GitRepository &operator=(const GitRepository &) = delete;

    bool open(const QString &path, QString *error = nullptr);
    void close();
    bool isOpen() const;
    QString rootPath() const;
    QString currentBranch(QString *error = nullptr) const;
    QVector<GitStatusEntry> statusEntries(QString *error = nullptr) const;
    QString diffForFile(const QString &relativePath, QString *error = nullptr) const;
    bool stageFile(const QString &relativePath, QString *error = nullptr);
    bool unstageFile(const QString &relativePath, QString *error = nullptr);
    bool commitStaged(const QString &message, QString *error = nullptr);
    bool fetchRemote(const QString &remoteName = QStringLiteral("origin"), QString *error = nullptr);
    bool pushCurrentBranch(const QString &remoteName = QStringLiteral("origin"), QString *error = nullptr);
    bool pullFastForward(const QString &remoteName = QStringLiteral("origin"), QString *error = nullptr);
    QStringList localBranches(QString *error = nullptr) const;
    bool createBranch(const QString &branchName, QString *error = nullptr);
    bool checkoutBranch(const QString &branchName, QString *error = nullptr);
    bool deleteBranch(const QString &branchName, QString *error = nullptr);
    bool mergeFastForward(const QString &branchName, QString *error = nullptr);
    bool hasChanges(QString *error = nullptr) const;

    static bool isRepository(const QString &path);

private:
    git_repository *repository = nullptr;
    QString root;

    static QString lastError(const QString &fallback);
};
