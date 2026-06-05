#pragma once

#include "BottomPanelController.h"
#include "CommandRegistry.h"
#include "DocumentChangePoller.h"
#include "DocumentRegistry.h"
#include "DapClient.h"
#include "EditorTabsController.h"
#include "EditorSurface.h"
#include "GitRepository.h"
#include "LspClient.h"
#include "OutputTranscript.h"
#include "ProjectModel.h"
#include "ProjectTreeController.h"
#include "ProjectReplaceService.h"
#include "RuntimeOrchestrator.h"
#include "SearchService.h"
#include "SettingsDialogModel.h"
#include "SettingsStore.h"
#include "TerminalBackend.h"
#include "TerminalProfileModel.h"
#include "UnsavedChangesGuard.h"
#include "WorkbenchState.h"
#include "WorkspaceSettingsStore.h"

#include <QFileSystemModel>
#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QTreeView>

#include <functional>
#include <memory>

class MainWindow final : public QMainWindow
{
    Q_OBJECT
    friend class TestMainWindow;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    MainWindow(QWidget *parent, const QString &settingsPath);
    bool openPath(const QString &path);
    QString currentProjectRoot() const;
    QString currentEditorPath() const;
    QString materializeRunnableBuffer(QString *error = nullptr);
    QVector<CommandDefinition> registeredCommandDefinitions() const { return commandRegistry.commands(); }

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
    bool resetShortcutSettingsToDefaults();
    bool setShortcutOverrideForCommand(const QString &commandId, const QKeySequence &shortcut);
    bool openOutputLinkAtCursor();
    void showAllOutput();
    void showOnlyStdoutOutput();
    void showOnlyStderrOutput();
    void showOnlySystemOutput();
    void openPowerShellTerminal();
    void sendTerminalInput();
    void stopTerminalProcess();
    void persistSelectedTerminalProfile();
    void trustCurrentWorkspace();
    void untrustCurrentWorkspace();
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
    void openReferenceResult(QListWidgetItem *item);
    void openOutlineResult(QListWidgetItem *item);
    void openSettings();
    void pollOpenDocumentChanges();
    void addCursorAboveAction();
    void addCursorBelowAction();
    void addCursorAtNextMatchAction();
    void selectAllCursorMatchesAction();
    void collapseToSingleCursorAction();
    void stageSelectedGitFile();
    void unstageSelectedGitFile();
    void commitStagedGitChanges();
    void fetchGitRemote();
    void pullGitRemote();
    void pushGitRemote();
    void switchSelectedGitBranch();
    void createGitBranch();
    void mergeSelectedGitBranch();
    void deleteSelectedGitBranch();
    void addWorkspaceRoot();
    void removeSelectedWorkspaceRoot();
    void switchSelectedWorkspaceRoot();

private:
    EditorSurface *editor = nullptr;
    QTabWidget *editorTabs = nullptr;
    QTreeView *projectTree = nullptr;
    QListWidget *workspaceRootsPanel = nullptr;
    QPushButton *workspaceAddRootButton = nullptr;
    QPushButton *workspaceRemoveRootButton = nullptr;
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
    QWidget *terminalContainerPanel = nullptr;
    QComboBox *terminalProfilePicker = nullptr;
    QLineEdit *terminalInput = nullptr;
    QPushButton *terminalSendButton = nullptr;
    QPushButton *terminalStopButton = nullptr;
    QPlainTextEdit *outputPanel = nullptr;
    QPlainTextEdit *terminalPanel = nullptr;
    QListWidget *problemsPanel = nullptr;
    QListWidget *searchResultsPanel = nullptr;
    QListWidget *referencesPanel = nullptr;
    QListWidget *outlinePanel = nullptr;
    QWidget *gitContainerPanel = nullptr;
    QListWidget *gitStatusPanel = nullptr;
    QListWidget *gitHistoryPanel = nullptr;
    QPlainTextEdit *gitDiffPanel = nullptr;
    QPushButton *gitStageButton = nullptr;
    QPushButton *gitUnstageButton = nullptr;
    QLineEdit *gitCommitMessageInput = nullptr;
    QPushButton *gitCommitButton = nullptr;
    QPushButton *gitFetchButton = nullptr;
    QPushButton *gitPullButton = nullptr;
    QPushButton *gitPushButton = nullptr;
    QComboBox *gitBranchPicker = nullptr;
    QLineEdit *gitBranchNameInput = nullptr;
    QPushButton *gitSwitchBranchButton = nullptr;
    QPushButton *gitCreateBranchButton = nullptr;
    QPushButton *gitMergeBranchButton = nullptr;
    QPushButton *gitDeleteBranchButton = nullptr;
    QPlainTextEdit *debugPanel = nullptr;
    QWidget *debugContainerPanel = nullptr;
    QListWidget *debugVariablesPanel = nullptr;
    QListWidget *debugWatchPanel = nullptr;
    QListWidget *debugCallStackPanel = nullptr;
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
    QTimer *documentChangePollTimer = nullptr;
    QTimer *untitledDraftAutosaveTimer = nullptr;
    QFutureWatcher<SearchResults> *activeSearchWatcher = nullptr;
    bool multiCursorSoftCapNoticeShown = false;
    int searchScanCap = SearchService::MaxScannedFiles;
    QVector<ProjectReplacePreviewRow> currentProjectReplacePreviewRows;
    SettingsStore settings;
    std::function<RuntimeDiagnostics(int)> runtimeDiagnosticsProvider;
    RuntimeOrchestrator runtimeOrchestrator;
    DapClient dapClient;
    LspClient lspClient;
    TerminalBackend terminalBackend;
    CommandRegistry commandRegistry;
    DocumentRegistry documentRegistry;
    DocumentChangePoller documentChangePoller;
    WorkbenchState workbenchState;
    WorkspaceSettings workspaceSettings;
    OutputTranscript outputTranscript;
    OutputTranscriptFilter outputFilter;
    GitRepository gitRepository;
    QVector<TerminalProfile> terminalProfiles;
    QString lspDocumentUri;
    int lspDocumentVersion = 0;
    bool lspDocumentOpen = false;
    bool debugSessionActive = false;
    bool debugSessionPaused = false;
    int activeDebugThreadId = 0;
    int activeDebugFrameId = 0;
    QStringList debugWatchExpressions;
    std::unique_ptr<EditorTabsController> editorTabsController;
    std::unique_ptr<ProjectTreeController> projectTreeController;
    std::unique_ptr<BottomPanelController> bottomPanels;
    QString projectRoot;
    QStringList workspaceRoots;
    QString activeWorkspaceTreeRoot;

    void buildUi();
    void registerWorkbenchCommands();
    void setStatus(const QString &text);
    bool loadProject(const QString &path);
    bool openEditorFile(const QString &path);
    bool addWorkspaceRootPath(const QString &path);
    bool switchWorkspaceRootPath(const QString &path);
    bool removeWorkspaceRootPath(const QString &path);
    bool requestCloseEditorTab(int index);
    void clearEditorsForDeletedPath(const QString &path);
    void applyThemePreference();
    void applyShortcutSettings();
    void refreshCurrentEditorUi(bool includeProblems);
    void configureLanguageServer();
    void configureDebugAdapter();
    bool startDebugSession();
    void syncCurrentEditorToLanguageServer(bool reopenDocument);
    void notifyLanguageServerOfSave();
    void closeLanguageServerDocument();
    void requestLanguageServerCompletion(int line, int character);
    void requestLanguageServerHover(int line, int character, const QPoint &viewportPosition);
    void requestLanguageServerDefinition(int line, int character);
    void requestLanguageServerReferences();
    void requestLanguageServerRename();
    void requestLanguageServerSemanticTokens();
    void requestLanguageServerDocumentSymbols();
    void requestLanguageServerWorkspaceSymbols();
    void continueDebugSession();
    void stepOverDebugSession();
    void stepIntoDebugSession();
    void stepOutDebugSession();
    void refreshDebugInspection(int threadId);
    void refreshDebugWatches();
    void renderDebugVariables(const QVector<DapVariable> &variables);
    void renderDebugCallStack(const QVector<DapStackFrame> &frames);
    void addSelectedDebugVariableToWatch();
    void addWatchExpression(const QString &expression);
    void updateBreadcrumbBar();
    void updateStatusIndicators();
    // Test-only snapshot for verifying MainWindow's registry integration.
    QVector<DocumentRecord> documentRecordsForTest() const { return documentRegistry.documents(); }
    void setSearchScanCapForTest(int cap) { searchScanCap = qMax(1, cap); }
    void resolveExternalDocumentChange(const DocumentRecord &record);
    void writeOutput(const QString &title, const QString &text);
    void showOutputPanel();
    void showTerminalPanel();
    void showProblemsPanel();
    void showSearchResultsPanel();
    void showReferencesPanel();
    void showOutlinePanel();
    QVector<SearchResultRow> currentEditorSearchResults(const QString &query) const;
    QVector<ProjectReplacePreviewRow> currentEditorReplacePreviewRows(const QString &query, const QString &replacement) const;
    void renderSearchResults(const QVector<SearchResultRow> &rows);
    void renderReferences(const QVector<LspLocation> &locations);
    void renderReferencesForTest(const QVector<LspLocation> &locations) { renderReferences(locations); }
    void renderOutline(const QVector<LspSymbol> &symbols);
    void renderOutlineForTest(const QVector<LspSymbol> &symbols) { renderOutline(symbols); }
    void renderGitStatusPanel();
    void renderGitDiffForPath(const QString &relativePath);
    void renderGitHistoryPanel();
    void renderGitDiffForCommit(const QString &commitId);
    void updateCurrentEditorBlame();
    QString selectedGitRelativePath() const;
    void refreshGitBranches();
    void renderWorkspaceRootsPanel();
    void openWorkspaceSymbolPicker(const QVector<LspSymbol> &symbols);
    void openWorkspaceSymbolPickerForTest(const QVector<LspSymbol> &symbols) { openWorkspaceSymbolPicker(symbols); }
    void renderRenamePreview(const LspWorkspaceEdit &edit);
    bool applyWorkspaceEdit(const LspWorkspaceEdit &edit, QString *error = nullptr);
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
    bool confirmHiddenBidiSave();
    bool confirmUnsavedDocuments(UnsavedChangesOperation operation);
    void discardUntitledDrafts();
    void runRuntimeAction(RuntimeAction action, const QString &title, bool reloadAfterSuccess = false);
    void appendRuntimeOutput(OutputTranscriptChannel channel, const QString &label, const QString &text);
    void setOutputFilter(const OutputTranscriptFilter &filter);
    void renderOutputTranscript();
    void refreshTerminalProfiles();
    TerminalProfile selectedTerminalProfile() const;
    void appendTerminalOutput(const QString &text);
    void updateTerminalControls();
    void restoreWorkbenchSession(bool promptForDraftRecovery = false);
    void saveWorkbenchSession();
    bool hasDirtyUntitledDraft() const;
    void scheduleUntitledDraftAutosave();
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
};
