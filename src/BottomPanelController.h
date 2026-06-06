#pragma once

#include <QString>

class QTabWidget;
class QWidget;

class BottomPanelController final
{
public:
    BottomPanelController(
        QTabWidget *tabs,
        QWidget *outputPanel,
        QWidget *terminalPanel,
        QWidget *problemsPanel,
        QWidget *searchResultsPanel,
        QWidget *referencesPanel,
        QWidget *outlinePanel,
        QWidget *gitPanel,
        QWidget *debugPanel);

    void showOutputPanel();
    void showTerminalPanel();
    void showProblemsPanel();
    void showSearchResultsPanel();
    void showReferencesPanel();
    void showOutlinePanel();

    QString panelId(QWidget *panel) const;
    QWidget *panelForId(const QString &id) const;

private:
    QTabWidget *tabs = nullptr;
    QWidget *output = nullptr;
    QWidget *terminal = nullptr;
    QWidget *problems = nullptr;
    QWidget *search = nullptr;
    QWidget *references = nullptr;
    QWidget *outline = nullptr;
    QWidget *git = nullptr;
    QWidget *debug = nullptr;

    void setCurrent(QWidget *panel);
};
