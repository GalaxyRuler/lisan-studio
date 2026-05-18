#pragma once

#include "BottomPanelController.h"
#include "CommandRegistry.h"
#include "DocumentChangePoller.h"
#include "DocumentRegistry.h"
#include "EditorTabsController.h"
#include "EditorSurface.h"
#include "OutputTranscript.h"
#include "ProjectModel.h"
#include "ProjectTreeController.h"
#include "ProjectReplaceService.h"
#include "RuntimeOrchestrator.h"
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
    void openSettings();
    void pollOpenDocumentChanges();

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
    QTimer *documentChangePollTimer = nullptr;
    QFutureWatcher<QVector<SearchResultRow>> *activeSearchWatcher = nullptr;
    QVector<ProjectReplacePreviewRow> currentProjectReplacePreviewRows;
    SettingsStore settings;
    std::function<RuntimeDiagnostics(int)> runtimeDiagnosticsProvider;
    RuntimeOrchestrator runtimeOrchestrator;
    CommandRegistry commandRegistry;
    DocumentRegistry documentRegistry;
    DocumentChangePoller documentChangePoller;
    WorkbenchState workbenchState;
    WorkspaceSettings workspaceSettings;
    OutputTranscript outputTranscript;
    OutputTranscriptFilter outputFilter;
    std::unique_ptr<EditorTabsController> editorTabsController;
    std::unique_ptr<ProjectTreeController> projectTreeController;
    std::unique_ptr<BottomPanelController> bottomPanels;
    QString projectRoot;

    void buildUi();
    void registerWorkbenchCommands();
    void setStatus(const QString &text);
    bool loadProject(const QString &path);
    bool openEditorFile(const QString &path);
    bool requestCloseEditorTab(int index);
    void clearEditorsForDeletedPath(const QString &path);
    void applyThemePreference();
    void applyShortcutSettings();
    void refreshCurrentEditorUi(bool includeProblems);
    void updateBreadcrumbBar();
    void updateStatusIndicators();
    // Test-only snapshot for verifying MainWindow's registry integration.
    QVector<DocumentRecord> documentRecordsForTest() const { return documentRegistry.documents(); }
    void resolveExternalDocumentChange(const DocumentRecord &record);
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
    bool confirmHiddenBidiSave();
    bool confirmUnsavedDocuments(UnsavedChangesOperation operation);
    void discardUntitledDrafts();
    void runRuntimeAction(RuntimeAction action, const QString &title, bool reloadAfterSuccess = false);
    void appendRuntimeOutput(OutputTranscriptChannel channel, const QString &label, const QString &text);
    void setOutputFilter(const OutputTranscriptFilter &filter);
    void renderOutputTranscript();
    void restoreWorkbenchSession();
    void saveWorkbenchSession();
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
};
