#include "GitRepository.h"

#include <git2.h>

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <memory>

namespace {
struct GitDiffDeleter
{
    void operator()(git_diff *diff) const { git_diff_free(diff); }
};

struct GitTreeDeleter
{
    void operator()(git_tree *tree) const { git_tree_free(tree); }
};

struct GitCommitDeleter
{
    void operator()(git_commit *commit) const { git_commit_free(commit); }
};

QString normalizeRootPath(const char *path)
{
    if (!path || !*path) {
        return QString();
    }
    return QDir::cleanPath(QDir::fromNativeSeparators(QString::fromUtf8(path)));
}

GitFileState stateFromStatus(unsigned int status, bool staged)
{
    if (status & GIT_STATUS_CONFLICTED) {
        return GitFileState::Conflicted;
    }
    if (staged) {
        if (status & GIT_STATUS_INDEX_NEW) {
            return GitFileState::Added;
        }
        if (status & GIT_STATUS_INDEX_DELETED) {
            return GitFileState::Deleted;
        }
        if (status & GIT_STATUS_INDEX_RENAMED) {
            return GitFileState::Renamed;
        }
        if (status & GIT_STATUS_INDEX_TYPECHANGE) {
            return GitFileState::TypeChanged;
        }
        return GitFileState::Modified;
    }

    if (status & GIT_STATUS_WT_NEW) {
        return GitFileState::Untracked;
    }
    if (status & GIT_STATUS_WT_DELETED) {
        return GitFileState::Deleted;
    }
    if (status & GIT_STATUS_WT_RENAMED) {
        return GitFileState::Renamed;
    }
    if (status & GIT_STATUS_WT_TYPECHANGE) {
        return GitFileState::TypeChanged;
    }
    return GitFileState::Modified;
}

QString deltaPath(const git_diff_delta *delta)
{
    if (!delta) {
        return QString();
    }
    const char *path = delta->new_file.path ? delta->new_file.path : delta->old_file.path;
    return path ? QString::fromUtf8(path) : QString();
}

QString oidToString(const git_oid *oid, qsizetype length = GIT_OID_HEXSZ)
{
    if (!oid) {
        return QString();
    }

    char buffer[GIT_OID_HEXSZ + 1] = {};
    git_oid_tostr(buffer, sizeof(buffer), oid);
    return QString::fromLatin1(buffer).left(length);
}

QString commitSummary(git_commit *commit)
{
    if (!commit) {
        return QString();
    }
    const char *summary = git_commit_summary(commit);
    return summary ? QString::fromUtf8(summary) : QString();
}

QString commitAuthorName(git_commit *commit)
{
    if (!commit) {
        return QString();
    }
    const git_signature *author = git_commit_author(commit);
    return author && author->name ? QString::fromUtf8(author->name) : QString();
}

QString commitSummaryForOid(git_repository *repository, const git_oid *oid)
{
    if (!repository || !oid) {
        return QString();
    }

    git_commit *commit = nullptr;
    if (git_commit_lookup(&commit, repository, oid) != 0) {
        return QString();
    }
    const QString summary = commitSummary(commit);
    git_commit_free(commit);
    return summary;
}

int appendDiffLine(const git_diff_delta *, const git_diff_hunk *, const git_diff_line *line, void *payload)
{
    auto *text = static_cast<QByteArray *>(payload);
    if (!text || !line || !line->content) {
        return 0;
    }
    text->append(line->origin);
    text->append(line->content, static_cast<qsizetype>(line->content_len));
    return 0;
}

}

GitRepository::GitRepository()
{
    git_libgit2_init();
}

GitRepository::~GitRepository()
{
    close();
    git_libgit2_shutdown();
}

bool GitRepository::open(const QString &path, QString *error)
{
    if (error) {
        error->clear();
    }

    close();

    git_repository *opened = nullptr;
    const QByteArray pathUtf8 = QFileInfo(path).absoluteFilePath().toUtf8();
    if (git_repository_open_ext(&opened, pathUtf8.constData(), 0, nullptr) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر فتح مستودع Git."));
        }
        return false;
    }

    const QString workdir = normalizeRootPath(git_repository_workdir(opened));
    root = workdir.isEmpty() ? normalizeRootPath(git_repository_path(opened)) : workdir;
    repository = opened;
    return true;
}

void GitRepository::close()
{
    if (repository) {
        git_repository_free(repository);
        repository = nullptr;
    }
    root.clear();
}

bool GitRepository::isOpen() const
{
    return repository != nullptr;
}

QString GitRepository::rootPath() const
{
    return root;
}

QString GitRepository::currentBranch(QString *error) const
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return QString();
    }

    git_reference *head = nullptr;
    const int result = git_repository_head(&head, repository);
    if (result == GIT_EUNBORNBRANCH) {
        return QStringLiteral("HEAD");
    }
    if (result != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة فرع Git الحالي."));
        }
        return QString();
    }

    const char *branchName = nullptr;
    QString displayName;
    if (git_reference_is_branch(head) && git_branch_name(&branchName, head) == 0 && branchName) {
        displayName = QString::fromUtf8(branchName);
    } else {
        const char *shorthand = git_reference_shorthand(head);
        displayName = shorthand ? QString::fromUtf8(shorthand) : QStringLiteral("HEAD");
    }
    git_reference_free(head);
    return displayName;
}

QVector<GitStatusEntry> GitRepository::statusEntries(QString *error) const
{
    if (error) {
        error->clear();
    }
    QVector<GitStatusEntry> entries;
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return entries;
    }

    git_status_options options = GIT_STATUS_OPTIONS_INIT;
    options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED
        | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS
        | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX
        | GIT_STATUS_OPT_SORT_CASE_INSENSITIVELY;

    git_status_list *statusList = nullptr;
    if (git_status_list_new(&statusList, repository, &options) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة حالة Git."));
        }
        return entries;
    }

    const size_t count = git_status_list_entrycount(statusList);
    entries.reserve(static_cast<int>(count));
    for (size_t i = 0; i < count; ++i) {
        const git_status_entry *entry = git_status_byindex(statusList, i);
        if (!entry) {
            continue;
        }

        if (entry->head_to_index) {
            const QString path = deltaPath(entry->head_to_index);
            if (!path.isEmpty()) {
                entries.push_back({path, stateFromStatus(entry->status, true), true});
            }
        }
        if (entry->index_to_workdir) {
            const QString path = deltaPath(entry->index_to_workdir);
            if (!path.isEmpty()) {
                entries.push_back({path, stateFromStatus(entry->status, false), false});
            }
        }
    }
    git_status_list_free(statusList);

    std::sort(entries.begin(), entries.end(), [](const GitStatusEntry &left, const GitStatusEntry &right) {
        const int pathCompare = QString::compare(left.relativePath, right.relativePath, Qt::CaseInsensitive);
        if (pathCompare != 0) {
            return pathCompare < 0;
        }
        return left.staged && !right.staged;
    });
    return entries;
}

QString GitRepository::diffForFile(const QString &relativePath, QString *error) const
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return QString();
    }

    const QString comparablePath = QDir::fromNativeSeparators(relativePath);

    git_diff_options options = GIT_DIFF_OPTIONS_INIT;
    options.flags = GIT_DIFF_INCLUDE_UNTRACKED
        | GIT_DIFF_RECURSE_UNTRACKED_DIRS
        | GIT_DIFF_SHOW_UNTRACKED_CONTENT;

    git_diff *diff = nullptr;
    if (git_diff_index_to_workdir(&diff, repository, nullptr, &options) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة فرق Git."));
        }
        return QString();
    }

    QString diffText;
    const size_t deltaCount = git_diff_num_deltas(diff);
    for (size_t i = 0; i < deltaCount; ++i) {
        const git_diff_delta *delta = git_diff_get_delta(diff, i);
        if (deltaPath(delta) != comparablePath) {
            continue;
        }

        git_patch *patch = nullptr;
        if (git_patch_from_diff(&patch, diff, i) != 0) {
            if (error) {
                *error = lastError(QStringLiteral("تعذر عرض فرق Git."));
            }
            git_diff_free(diff);
            return QString();
        }

        git_buf buffer = {};
        if (git_patch_to_buf(&buffer, patch) != 0) {
            if (error) {
                *error = lastError(QStringLiteral("تعذر عرض فرق Git."));
            }
            git_patch_free(patch);
            git_diff_free(diff);
            return QString();
        }
        diffText = QString::fromUtf8(buffer.ptr, qsizetype(buffer.size));
        git_buf_dispose(&buffer);
        git_patch_free(patch);
        break;
    }
    git_diff_free(diff);
    return diffText;
}

bool GitRepository::stageFile(const QString &relativePath, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return false;
    }

    git_index *index = nullptr;
    if (git_repository_index(&index, repository) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر فتح فهرس Git."));
        }
        return false;
    }

    const QByteArray pathUtf8 = QDir::fromNativeSeparators(relativePath).toUtf8();
    const bool added = git_index_add_bypath(index, pathUtf8.constData()) == 0
        && git_index_write(index) == 0;
    if (!added && error) {
        *error = lastError(QStringLiteral("تعذر تجهيز الملف في Git."));
    }
    git_index_free(index);
    return added;
}

bool GitRepository::unstageFile(const QString &relativePath, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return false;
    }

    const QByteArray pathUtf8 = QDir::fromNativeSeparators(relativePath).toUtf8();
    char *pathSpecValue = const_cast<char *>(pathUtf8.constData());
    git_strarray pathSpec = {&pathSpecValue, 1};

    git_object *headCommit = nullptr;
    if (git_revparse_single(&headCommit, repository, "HEAD") != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة HEAD لإلغاء التجهيز."));
        }
        return false;
    }

    const bool reset = git_reset_default(repository, headCommit, &pathSpec) == 0;
    if (!reset && error) {
        *error = lastError(QStringLiteral("تعذر إلغاء تجهيز الملف في Git."));
    }
    git_object_free(headCommit);
    return reset;
}

bool GitRepository::commitStaged(const QString &message, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return false;
    }

    const QString trimmedMessage = message.trimmed();
    if (trimmedMessage.isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("رسالة الالتزام مطلوبة.");
        }
        return false;
    }

    git_index *index = nullptr;
    if (git_repository_index(&index, repository) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر فتح فهرس Git."));
        }
        return false;
    }

    git_oid treeOid;
    if (git_index_write_tree(&treeOid, index) != 0 || git_index_write(index) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر كتابة شجرة Git."));
        }
        git_index_free(index);
        return false;
    }
    git_index_free(index);

    git_tree *tree = nullptr;
    if (git_tree_lookup(&tree, repository, &treeOid) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة شجرة Git."));
        }
        return false;
    }

    git_commit *parent = nullptr;
    git_reference *head = nullptr;
    int parentCount = 0;
    if (git_repository_head(&head, repository) == 0) {
        const git_oid *parentOid = git_reference_target(head);
        if (parentOid && git_commit_lookup(&parent, repository, parentOid) == 0) {
            parentCount = 1;
        }
    }

    git_signature *signature = nullptr;
    if (git_signature_default(&signature, repository) != 0) {
        git_signature_now(&signature, "Lisan Studio", "lisanstudio@example.invalid");
    }

    git_oid commitOid;
    const QByteArray messageUtf8 = trimmedMessage.toUtf8();
    const int commitResult = parentCount == 1
        ? git_commit_create_v(&commitOid, repository, "HEAD", signature, signature, nullptr, messageUtf8.constData(), tree, 1, parent)
        : git_commit_create_v(&commitOid, repository, "HEAD", signature, signature, nullptr, messageUtf8.constData(), tree, 0);

    if (commitResult != 0 && error) {
        *error = lastError(QStringLiteral("تعذر إنشاء التزام Git."));
    }

    git_signature_free(signature);
    if (head) {
        git_reference_free(head);
    }
    if (parent) {
        git_commit_free(parent);
    }
    git_tree_free(tree);
    return commitResult == 0;
}

bool GitRepository::fetchRemote(const QString &remoteName, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return false;
    }

    git_remote *remote = nullptr;
    const QByteArray remoteUtf8 = remoteName.toUtf8();
    if (git_remote_lookup(&remote, repository, remoteUtf8.constData()) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر العثور على remote في Git."));
        }
        return false;
    }

    git_fetch_options options = GIT_FETCH_OPTIONS_INIT;
    const bool fetched = git_remote_fetch(remote, nullptr, &options, nullptr) == 0;
    if (!fetched && error) {
        *error = lastError(QStringLiteral("تعذر جلب تحديثات Git."));
    }
    git_remote_free(remote);
    return fetched;
}

bool GitRepository::pushCurrentBranch(const QString &remoteName, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return false;
    }

    const QString branch = currentBranch(error);
    if (branch.isEmpty()) {
        return false;
    }

    git_remote *remote = nullptr;
    const QByteArray remoteUtf8 = remoteName.toUtf8();
    if (git_remote_lookup(&remote, repository, remoteUtf8.constData()) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر العثور على remote في Git."));
        }
        return false;
    }

    const QByteArray refSpecUtf8 = QStringLiteral("refs/heads/%1:refs/heads/%1").arg(branch).toUtf8();
    char *refSpecValue = const_cast<char *>(refSpecUtf8.constData());
    git_strarray refSpecs = {&refSpecValue, 1};
    git_push_options options = GIT_PUSH_OPTIONS_INIT;
    const bool pushed = git_remote_push(remote, &refSpecs, &options) == 0;
    if (!pushed && error) {
        *error = lastError(QStringLiteral("تعذر دفع Git."));
    }
    git_remote_free(remote);
    return pushed;
}

bool GitRepository::pullFastForward(const QString &remoteName, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!fetchRemote(remoteName, error)) {
        return false;
    }

    const QString branch = currentBranch(error);
    if (branch.isEmpty()) {
        return false;
    }

    const QString remoteRefName = QStringLiteral("refs/remotes/%1/%2").arg(remoteName, branch);
    git_reference *remoteRef = nullptr;
    const QByteArray remoteRefUtf8 = remoteRefName.toUtf8();
    if (git_reference_lookup(&remoteRef, repository, remoteRefUtf8.constData()) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة فرع remote في Git."));
        }
        return false;
    }

    git_annotated_commit *remoteCommit = nullptr;
    if (git_annotated_commit_from_ref(&remoteCommit, repository, remoteRef) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر تحليل تحديث Git."));
        }
        git_reference_free(remoteRef);
        return false;
    }

    const git_annotated_commit *heads[] = {remoteCommit};
    git_merge_analysis_t analysis = GIT_MERGE_ANALYSIS_NONE;
    git_merge_preference_t preference = GIT_MERGE_PREFERENCE_NONE;
    if (git_merge_analysis(&analysis, &preference, repository, heads, 1) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر تحليل دمج Git."));
        }
        git_annotated_commit_free(remoteCommit);
        git_reference_free(remoteRef);
        return false;
    }

    if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        git_annotated_commit_free(remoteCommit);
        git_reference_free(remoteRef);
        return true;
    }
    if (!(analysis & GIT_MERGE_ANALYSIS_FASTFORWARD)) {
        if (error) {
            *error = QString::fromUtf8("السحب يتطلب دمجا غير سريع. افتح Git خارجي لحل التعارض.");
        }
        git_annotated_commit_free(remoteCommit);
        git_reference_free(remoteRef);
        return false;
    }

    QString statusError;
    if (!statusEntries(&statusError).isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("احفظ أو التزم بتغييراتك المحلية قبل السحب.");
        }
        git_annotated_commit_free(remoteCommit);
        git_reference_free(remoteRef);
        return false;
    }
    const git_oid *targetOid = git_reference_target(remoteRef);
    git_object *targetCommit = nullptr;
    if (!targetOid || git_object_lookup(&targetCommit, repository, targetOid, GIT_OBJECT_COMMIT) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة التزام remote."));
        }
        git_annotated_commit_free(remoteCommit);
        git_reference_free(remoteRef);
        return false;
    }

    git_checkout_options checkoutOptions = GIT_CHECKOUT_OPTIONS_INIT;
    checkoutOptions.checkout_strategy = GIT_CHECKOUT_FORCE;
    const bool reset = git_reset(repository, targetCommit, GIT_RESET_HARD, &checkoutOptions) == 0;
    if (!reset && error) {
        *error = lastError(QStringLiteral("تعذر تحديث الفرع المحلي."));
    }

    git_object_free(targetCommit);
    git_annotated_commit_free(remoteCommit);
    git_reference_free(remoteRef);
    return reset;
}

QStringList GitRepository::localBranches(QString *error) const
{
    if (error) {
        error->clear();
    }
    QStringList branches;
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return branches;
    }

    git_branch_iterator *iterator = nullptr;
    if (git_branch_iterator_new(&iterator, repository, GIT_BRANCH_LOCAL) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة فروع Git."));
        }
        return branches;
    }

    git_reference *reference = nullptr;
    git_branch_t type = GIT_BRANCH_LOCAL;
    while (git_branch_next(&reference, &type, iterator) == 0) {
        const char *name = nullptr;
        if (git_branch_name(&name, reference) == 0 && name) {
            branches.append(QString::fromUtf8(name));
        }
        git_reference_free(reference);
        reference = nullptr;
    }
    git_branch_iterator_free(iterator);
    branches.sort(Qt::CaseInsensitive);
    return branches;
}

bool GitRepository::createBranch(const QString &branchName, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return false;
    }
    const QString trimmed = branchName.trimmed();
    if (trimmed.isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("اسم الفرع مطلوب.");
        }
        return false;
    }

    git_reference *head = nullptr;
    git_commit *headCommit = nullptr;
    if (git_repository_head(&head, repository) != 0
        || !git_reference_target(head)
        || git_commit_lookup(&headCommit, repository, git_reference_target(head)) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة HEAD لإنشاء الفرع."));
        }
        if (head) {
            git_reference_free(head);
        }
        return false;
    }

    git_reference *created = nullptr;
    const QByteArray nameUtf8 = trimmed.toUtf8();
    const bool ok = git_branch_create(&created, repository, nameUtf8.constData(), headCommit, 0) == 0;
    if (!ok && error) {
        *error = lastError(QStringLiteral("تعذر إنشاء فرع Git."));
    }
    if (created) {
        git_reference_free(created);
    }
    git_commit_free(headCommit);
    git_reference_free(head);
    return ok;
}

bool GitRepository::checkoutBranch(const QString &branchName, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return false;
    }
    QString statusError;
    if (!statusEntries(&statusError).isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("احفظ أو التزم بتغييراتك المحلية قبل تبديل الفرع.");
        }
        return false;
    }

    git_reference *branch = nullptr;
    const QByteArray branchUtf8 = branchName.toUtf8();
    if (git_branch_lookup(&branch, repository, branchUtf8.constData(), GIT_BRANCH_LOCAL) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر العثور على فرع Git."));
        }
        return false;
    }

    git_checkout_options options = GIT_CHECKOUT_OPTIONS_INIT;
    options.checkout_strategy = GIT_CHECKOUT_FORCE;
    const bool ok = git_repository_set_head(repository, git_reference_name(branch)) == 0
        && git_checkout_head(repository, &options) == 0;
    if (!ok && error) {
        *error = lastError(QStringLiteral("تعذر تبديل فرع Git."));
    }
    git_reference_free(branch);
    return ok;
}

bool GitRepository::deleteBranch(const QString &branchName, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return false;
    }

    git_reference *branch = nullptr;
    const QByteArray branchUtf8 = branchName.toUtf8();
    if (git_branch_lookup(&branch, repository, branchUtf8.constData(), GIT_BRANCH_LOCAL) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر العثور على فرع Git."));
        }
        return false;
    }

    const bool ok = git_branch_delete(branch) == 0;
    if (!ok && error) {
        *error = lastError(QStringLiteral("تعذر حذف فرع Git."));
    }
    git_reference_free(branch);
    return ok;
}

bool GitRepository::mergeFastForward(const QString &branchName, QString *error)
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return false;
    }
    QString statusError;
    if (!statusEntries(&statusError).isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("احفظ أو التزم بتغييراتك المحلية قبل الدمج.");
        }
        return false;
    }

    git_reference *branch = nullptr;
    const QByteArray branchUtf8 = branchName.toUtf8();
    if (git_branch_lookup(&branch, repository, branchUtf8.constData(), GIT_BRANCH_LOCAL) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر العثور على فرع Git."));
        }
        return false;
    }

    const git_oid *targetOid = git_reference_target(branch);
    git_object *targetCommit = nullptr;
    if (!targetOid || git_object_lookup(&targetCommit, repository, targetOid, GIT_OBJECT_COMMIT) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة التزام الفرع."));
        }
        git_reference_free(branch);
        return false;
    }

    git_annotated_commit *annotated = nullptr;
    if (git_annotated_commit_from_ref(&annotated, repository, branch) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر تحليل دمج الفرع."));
        }
        git_object_free(targetCommit);
        git_reference_free(branch);
        return false;
    }

    const git_annotated_commit *heads[] = {annotated};
    git_merge_analysis_t analysis = GIT_MERGE_ANALYSIS_NONE;
    git_merge_preference_t preference = GIT_MERGE_PREFERENCE_NONE;
    if (git_merge_analysis(&analysis, &preference, repository, heads, 1) != 0
        || !(analysis & (GIT_MERGE_ANALYSIS_FASTFORWARD | GIT_MERGE_ANALYSIS_UP_TO_DATE))) {
        if (error) {
            *error = QString::fromUtf8("الدمج يتطلب دمجا غير سريع أو حل تعارض.");
        }
        git_annotated_commit_free(annotated);
        git_object_free(targetCommit);
        git_reference_free(branch);
        return false;
    }

    git_checkout_options checkoutOptions = GIT_CHECKOUT_OPTIONS_INIT;
    checkoutOptions.checkout_strategy = GIT_CHECKOUT_FORCE;
    const bool ok = (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE)
        || git_reset(repository, targetCommit, GIT_RESET_HARD, &checkoutOptions) == 0;
    if (!ok && error) {
        *error = lastError(QStringLiteral("تعذر إكمال الدمج السريع."));
    }
    git_annotated_commit_free(annotated);
    git_object_free(targetCommit);
    git_reference_free(branch);
    return ok;
}

QVector<GitCommitSummary> GitRepository::commitHistory(int maxCount, QString *error) const
{
    if (error) {
        error->clear();
    }
    QVector<GitCommitSummary> history;
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return history;
    }
    if (maxCount <= 0) {
        return history;
    }

    git_revwalk *walk = nullptr;
    if (git_revwalk_new(&walk, repository) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة سجل Git."));
        }
        return history;
    }
    git_revwalk_sorting(walk, GIT_SORT_TIME | GIT_SORT_TOPOLOGICAL);
    if (git_revwalk_push_head(walk) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة HEAD لسجل Git."));
        }
        git_revwalk_free(walk);
        return history;
    }

    git_oid oid;
    while (history.size() < maxCount && git_revwalk_next(&oid, walk) == 0) {
        git_commit *commit = nullptr;
        if (git_commit_lookup(&commit, repository, &oid) != 0) {
            continue;
        }
        history.push_back({
            oidToString(&oid),
            oidToString(&oid, 7),
            commitSummary(commit),
            commitAuthorName(commit),
        });
        git_commit_free(commit);
    }

    git_revwalk_free(walk);
    return history;
}

QVector<GitBlameLine> GitRepository::blameFile(const QString &relativePath, QString *error) const
{
    if (error) {
        error->clear();
    }
    QVector<GitBlameLine> blameLines;
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return blameLines;
    }

    git_blame_options options = GIT_BLAME_OPTIONS_INIT;
    git_blame *blame = nullptr;
    const QByteArray pathUtf8 = QDir::fromNativeSeparators(relativePath).toUtf8();
    if (git_blame_file(&blame, repository, pathUtf8.constData(), &options) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة نسب أسطر Git."));
        }
        return blameLines;
    }

    const quint32 hunkCount = git_blame_get_hunk_count(blame);
    for (quint32 hunkIndex = 0; hunkIndex < hunkCount; ++hunkIndex) {
        const git_blame_hunk *hunk = git_blame_get_hunk_byindex(blame, hunkIndex);
        if (!hunk) {
            continue;
        }
        const QString id = oidToString(&hunk->final_commit_id);
        const QString shortId = oidToString(&hunk->final_commit_id, 7);
        const QString summary = commitSummaryForOid(repository, &hunk->final_commit_id);
        for (quint16 offset = 0; offset < hunk->lines_in_hunk; ++offset) {
            blameLines.push_back({
                static_cast<int>(hunk->final_start_line_number + offset),
                id,
                shortId,
                summary,
            });
        }
    }

    git_blame_free(blame);
    return blameLines;
}

QString GitRepository::diffForCommit(const QString &commitId, QString *error) const
{
    if (error) {
        error->clear();
    }
    if (!repository) {
        if (error) {
            *error = QStringLiteral("لا يوجد مستودع Git مفتوح.");
        }
        return QString();
    }

    git_oid oid;
    const QByteArray commitUtf8 = commitId.toUtf8();
    if (git_oid_fromstrp(&oid, commitUtf8.constData()) != 0) {
        if (error) {
            *error = QString::fromUtf8("معرف الالتزام غير صالح.");
        }
        return QString();
    }

    git_commit *commitRaw = nullptr;
    if (git_commit_lookup(&commitRaw, repository, &oid) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة التزام Git."));
        }
        return QString();
    }
    std::unique_ptr<git_commit, GitCommitDeleter> commit(commitRaw);

    git_tree *commitTreeRaw = nullptr;
    if (git_commit_tree(&commitTreeRaw, commit.get()) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة شجرة الالتزام."));
        }
        return QString();
    }
    std::unique_ptr<git_tree, GitTreeDeleter> commitTree(commitTreeRaw);

    std::unique_ptr<git_tree, GitTreeDeleter> parentTree;
    if (git_commit_parentcount(commit.get()) > 0) {
        git_commit *parentCommitRaw = nullptr;
        if (git_commit_parent(&parentCommitRaw, commit.get(), 0) != 0) {
            if (error) {
                *error = lastError(QStringLiteral("تعذر قراءة الالتزام السابق."));
            }
            return QString();
        }
        std::unique_ptr<git_commit, GitCommitDeleter> parentCommit(parentCommitRaw);
        git_tree *parentTreeRaw = nullptr;
        if (git_commit_tree(&parentTreeRaw, parentCommit.get()) != 0) {
            if (error) {
                *error = lastError(QStringLiteral("تعذر قراءة شجرة الالتزام السابق."));
            }
            return QString();
        }
        parentTree.reset(parentTreeRaw);
    }

    git_diff_options options = GIT_DIFF_OPTIONS_INIT;
    git_diff *diffRaw = nullptr;
    if (git_diff_tree_to_tree(&diffRaw, repository, parentTree.get(), commitTree.get(), &options) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر قراءة فرق الالتزام."));
        }
        return QString();
    }
    std::unique_ptr<git_diff, GitDiffDeleter> diff(diffRaw);

    QByteArray text;
    if (git_diff_print(diff.get(), GIT_DIFF_FORMAT_PATCH, appendDiffLine, &text) != 0) {
        if (error) {
            *error = lastError(QStringLiteral("تعذر عرض فرق الالتزام."));
        }
        return QString();
    }

    return QString::fromUtf8(text);
}

bool GitRepository::hasChanges(QString *error) const
{
    return !statusEntries(error).isEmpty();
}

bool GitRepository::isRepository(const QString &path)
{
    git_libgit2_init();
    git_repository *probe = nullptr;
    const QByteArray pathUtf8 = QFileInfo(path).absoluteFilePath().toUtf8();
    const bool result = git_repository_open_ext(&probe, pathUtf8.constData(), 0, nullptr) == 0;
    if (probe) {
        git_repository_free(probe);
    }
    git_libgit2_shutdown();
    return result;
}

QString GitRepository::lastError(const QString &fallback)
{
    const git_error *error = git_error_last();
    if (!error || !error->message) {
        return fallback;
    }
    return QString::fromUtf8(error->message);
}
