#pragma once

#include <QString>
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
    bool hasChanges(QString *error = nullptr) const;

    static bool isRepository(const QString &path);

private:
    git_repository *repository = nullptr;
    QString root;

    static QString lastError(const QString &fallback);
};
