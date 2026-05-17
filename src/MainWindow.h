#pragma once

#include "CommandRegistry.h"
#include "DocumentRegistry.h"
#include "EditorSurface.h"
#include "OutputTranscript.h"
#include "ProjectModel.h"
#include "ProjectReplaceService.h"
#include "RuntimeHistory.h"
#include "RuntimeRunner.h"
#include "SearchService.h"
#include "SettingsDialogModel.h"
#include "SettingsStore.h"
#include "TerminalProfileModel.h"
#include "UnsavedChangesGuard.h"
#include "WorkbenchState.h"
#include "WorkspaceSettingsStore.h"

#include <QFileSystemModel>
#include <QAction>
#include <QCloseEvent>
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
    MainWindow(QWidget *parent, const QString &settingsPath);
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
    void rerunLastRuntimeAction();
    void cancelRuntimeProcess();
    void copyOutputPanel();
    void clearOutputPanel();
    void saveOutputPanel();
    bool saveOutputPanelToPath(const QString &path);
    bool exportShortcutSettingsToPath(const QString &path);
    bool importShortcutSettingsFromPath(const QString &path);
    bool openOutputLinkAtCursor();
    void showAllOutput();
    void showOnlyStdoutOutput();
    void showOnlyStderrOutput();
    void showOnlySystemOutput();
    void openPowerShellTerminal();
    void trustCurrentWorkspace();
    void appendRuntimeStdout();
    void appendRuntimeStderr();
    void finishRuntimeProcess(int exitCode, QProcess::ExitStatus exitStatus);
    void handleRuntimeProcessError(QProcess::ProcessError error);
    void handleRuntimeTimeout();
    void openCommandPalette();
    void findInProject();
    void previewProjectReplace();
    void applyAcceptedProjectReplaceRows();
    void insertPrintSnippet();
    void toggleVisibleWhitespace();
    void toggleTrimTrailingWhitespace();
    void openInFileFind();
    void updateInFileFindMatches();
    void selectNextInFileMatch();
    void selectPreviousInFileMatch();
    void replaceCurrentInFileMatch();
    void replaceAllInFileMatches();
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
    void copyProjectTreeItemPath();
    void openProjectTreeContainingFolder();
    void refreshProjectTree();
    void openSettings();

private:
    EditorSurface *editor = nullptr;
    QTabWidget *editorTabs = nullptr;
    QTreeView *projectTree = nullptr;
    QFileSystemModel *fileSystemModel = nullptr;
    QWidget *inFileFindPanel = nullptr;
    QLineEdit *inFileFindInput = nullptr;
    QLineEdit *inFileReplaceInput = nullptr;
    QLabel *inFileFindStatusLabel = nullptr;
    QLabel *breadcrumbPathLabel = nullptr;
    QLabel *breadcrumbSymbolLabel = nullptr;
    QLineEdit *commandBox = nullptr;
    QLineEdit *projectReplaceInput = nullptr;
    QPushButton *projectReplacePreviewButton = nullptr;
    QPushButton *projectReplaceApplyButton = nullptr;
    QPlainTextEdit *outputPanel = nullptr;
    QPlainTextEdit *terminalPanel = nullptr;
    QListWidget *problemsPanel = nullptr;
    QListWidget *searchResultsPanel = nullptr;
    QPlainTextEdit *debugPanel = nullptr;
    QTabWidget *bottomPanelTabs = nullptr;
    QDockWidget *outputDock = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *statusEncodingLabel = nullptr;
    QLabel *statusLineEndingLabel = nullptr;
    QLabel *statusIndentationLabel = nullptr;
    QLabel *statusLanguageModeLabel = nullptr;
    QLabel *statusRuntimeLabel = nullptr;
    QLabel *statusGitLabel = nullptr;
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
    QAction *projectTreeCopyPathAction = nullptr;
    QAction *projectTreeOpenContainingFolderAction = nullptr;
    QAction *projectTreeRefreshAction = nullptr;
    QProcess *activeRuntimeProcess = nullptr;
    QTimer *runtimeTimeoutTimer = nullptr;
    QFutureWatcher<QVector<SearchResultRow>> *activeSearchWatcher = nullptr;
    QElapsedTimer activeRuntimeTimer;
    QString activeRuntimeTitle;
    QString activeRuntimeStdout;
    QString activeRuntimeStderr;
    QString darkThemeStyleSheet;
    QVector<ProjectReplacePreviewRow> currentProjectReplacePreviewRows;
    QModelIndex projectTreeContextIndex;
    bool activeRuntimeHandledError = false;
    SettingsStore settings;
    RuntimeRunner runtime;
    RuntimeHistory runtimeHistory;
    CommandRegistry commandRegistry;
    WorkbenchState workbenchState;
    WorkspaceSettings workspaceSettings;
    OutputTranscript outputTranscript;
    OutputTranscriptFilter outputFilter;
    QString projectRoot;

    void buildUi();
    void registerWorkbenchCommands();
    void setStatus(const QString &text);
    bool loadProject(const QString &path);
    bool openEditorFile(const QString &path);
    QModelIndex activeProjectTreeIndex() const;
    QString activeProjectTreePath() const;
    QString activeProjectTreeFolderPath() const;
    void clearEditorsForDeletedPath(const QString &path);
    EditorSurface *createEditorTab(const QString &title);
    void applyEditorFont(EditorSurface *surface);
    void applyWorkspaceSettings(EditorSurface *surface);
    void applyWorkspaceSettingsToOpenEditors();
    void applyThemePreference();
    void applyShortcutSettings();
    void updateBreadcrumbBar();
    void updateStatusIndicators();
    void syncEditorSession(EditorSurface *surface);
    void setCurrentEditor(EditorSurface *surface);
    void updateEditorTabTitle(EditorSurface *surface);
    void closeEditorTab(int index);
    void writeOutput(const QString &title, const QString &text);
    void showOutputPanel();
    void showTerminalPanel();
    void showProblemsPanel();
    void showSearchResultsPanel();
    QVector<SearchResultRow> currentEditorSearchResults(const QString &query) const;
    QVector<ProjectReplacePreviewRow> currentEditorReplacePreviewRows(const QString &query, const QString &replacement) const;
    void renderSearchResults(const QVector<SearchResultRow> &rows);
    void renderProjectReplacePreview(const QVector<ProjectReplacePreviewRow> &rows);
    void setProjectReplaceFileAccepted(const QString &path, bool accepted);
    QVector<ProjectReplacePreviewRow> acceptedProjectReplaceRows() const;
    bool hasDirtyOpenDocumentForReplaceRows(const QVector<ProjectReplacePreviewRow> &rows) const;
    bool insertSnippetById(const QString &id);
    void refreshEditorProblems();
    void addProblem(const QString &severity, const QString &message, const QString &path = QString(), int line = 0);
    void goToEditorLine(int line);
    void goToEditorLocation(int line, int column);
    QString runtimeWorkingDirectory() const;
    bool confirmSaveIfDirty();
    QVector<DocumentRecord> openDocumentRecords() const;
    bool confirmUnsavedDocuments(UnsavedChangesOperation operation);
    void runRuntimeAction(RuntimeAction action, const QString &title, bool reloadAfterSuccess = false);
    void startRuntimeLaunchPlan(const RuntimeLaunchPlan &plan, bool recordHistory);
    void appendRuntimeOutput(const QString &label, const QString &text);
    void setOutputFilter(const OutputTranscriptFilter &filter);
    void renderOutputTranscript();
    void completeRuntimeProcess(const QString &statusText);
    void setRuntimeActionsRunning(bool running);
    void restoreWorkbenchSession();
    void saveWorkbenchSession();
    QString bottomPanelId(QWidget *panel) const;
    QWidget *bottomPanelForId(const QString &id) const;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
};
