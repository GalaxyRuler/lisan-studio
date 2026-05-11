#pragma once

#include "EditorSurface.h"
#include "ProjectModel.h"
#include "RuntimeRunner.h"
#include "SearchService.h"
#include "SettingsStore.h"

#include <QFileSystemModel>
#include <QAction>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QTreeView>

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool openPath(const QString &path);
    QString currentProjectRoot() const;
    QString currentEditorPath() const;
    QString materializeRunnableBuffer(QString *error = nullptr);

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void openFolder();
    void runCurrentFile();
    void lintCurrentFile();
    void formatCurrentFile();
    void cancelRuntimeProcess();
    void appendRuntimeStdout();
    void appendRuntimeStderr();
    void finishRuntimeProcess(int exitCode, QProcess::ExitStatus exitStatus);
    void handleRuntimeProcessError(QProcess::ProcessError error);
    void handleRuntimeTimeout();
    void openCommandPalette();
    void findInProject();
    void openSearchResult(QListWidgetItem *item);
    void openProblemResult(QListWidgetItem *item);
    void openSelectedProjectFile(const QModelIndex &index);
    void showProjectTreeContextMenu(const QPoint &pos);
    void createProjectTreeFile();
    void createProjectTreeFolder();
    void openProjectTreeItem();
    void renameProjectTreeItem();
    void deleteProjectTreeItem();
    void revealProjectTreeItem();
    void refreshProjectTree();
    void openSettings();

private:
    EditorSurface *editor = nullptr;
    QTabWidget *editorTabs = nullptr;
    QTreeView *projectTree = nullptr;
    QFileSystemModel *fileSystemModel = nullptr;
    QLineEdit *commandBox = nullptr;
    QPlainTextEdit *outputPanel = nullptr;
    QPlainTextEdit *terminalPanel = nullptr;
    QListWidget *problemsPanel = nullptr;
    QListWidget *searchResultsPanel = nullptr;
    QPlainTextEdit *debugPanel = nullptr;
    QTabWidget *bottomPanelTabs = nullptr;
    QDockWidget *outputDock = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *brandLogoLabel = nullptr;
    QAction *commandPaletteAction = nullptr;
    QAction *runAction = nullptr;
    QAction *lintAction = nullptr;
    QAction *formatAction = nullptr;
    QAction *cancelRunAction = nullptr;
    QAction *projectTreeNewFileAction = nullptr;
    QAction *projectTreeNewFolderAction = nullptr;
    QAction *projectTreeOpenAction = nullptr;
    QAction *projectTreeRenameAction = nullptr;
    QAction *projectTreeDeleteAction = nullptr;
    QAction *projectTreeRevealAction = nullptr;
    QAction *projectTreeRefreshAction = nullptr;
    QProcess *activeRuntimeProcess = nullptr;
    QTimer *runtimeTimeoutTimer = nullptr;
    QFutureWatcher<QVector<SearchResultRow>> *activeSearchWatcher = nullptr;
    QElapsedTimer activeRuntimeTimer;
    QString activeRuntimeTitle;
    QString activeRuntimeStdout;
    QString activeRuntimeStderr;
    QModelIndex projectTreeContextIndex;
    bool activeRuntimeHandledError = false;
    int searchGeneration = 0;
    SettingsStore settings;
    RuntimeRunner runtime;
    QString projectRoot;

    void buildUi();
    void setStatus(const QString &text);
    bool loadProject(const QString &path);
    bool openEditorFile(const QString &path);
    QModelIndex activeProjectTreeIndex() const;
    QString activeProjectTreePath() const;
    QString activeProjectTreeFolderPath() const;
    bool isValidProjectChildName(const QString &name) const;
    void clearEditorsForDeletedPath(const QString &path);
    EditorSurface *createEditorTab(const QString &title);
    void applyEditorFont(EditorSurface *surface);
    void setCurrentEditor(EditorSurface *surface);
    void updateEditorTabTitle(EditorSurface *surface);
    void closeEditorTab(int index);
    void writeOutput(const QString &title, const QString &text);
    void showOutputPanel();
    void showProblemsPanel();
    void showSearchResultsPanel();
    QVector<SearchResultRow> currentEditorSearchResults(const QString &query) const;
    void renderSearchResults(const QVector<SearchResultRow> &rows);
    void refreshEditorProblems();
    void addProblem(const QString &severity, const QString &message, const QString &path = QString(), int line = 0);
    void goToEditorLine(int line);
    QString runtimeWorkingDirectory() const;
    bool confirmSaveIfDirty();
    void runRuntimeAction(RuntimeAction action, const QString &title, bool reloadAfterSuccess = false);
    void appendRuntimeOutput(const QString &label, const QString &text);
    void completeRuntimeProcess(const QString &statusText);
    void setRuntimeActionsRunning(bool running);
};
