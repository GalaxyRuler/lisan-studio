#pragma once

#include "EditorSurface.h"
#include "ProjectModel.h"
#include "RuntimeRunner.h"
#include "SearchService.h"
#include "SettingsStore.h"

#include <QFileSystemModel>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeView>

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool openPath(const QString &path);
    QString currentProjectRoot() const;
    QString currentEditorPath() const;

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void openFolder();
    void runCurrentFile();
    void lintCurrentFile();
    void formatCurrentFile();
    void findInProject();
    void openSelectedProjectFile(const QModelIndex &index);
    void openSettings();

private:
    EditorSurface *editor = nullptr;
    QTreeView *projectTree = nullptr;
    QFileSystemModel *fileSystemModel = nullptr;
    QLineEdit *commandBox = nullptr;
    QLineEdit *searchBox = nullptr;
    QPlainTextEdit *outputPanel = nullptr;
    QLabel *statusLabel = nullptr;
    SettingsStore settings;
    RuntimeRunner runtime;
    QString projectRoot;

    void buildUi();
    void setStatus(const QString &text);
    bool loadProject(const QString &path);
    bool openEditorFile(const QString &path);
    void writeOutput(const QString &title, const QString &text);
    bool confirmSaveIfDirty();
    bool ensureCurrentFileSaved();
    void runRuntimeAction(RuntimeAction action, const QString &title, bool reloadAfterSuccess = false);
};
