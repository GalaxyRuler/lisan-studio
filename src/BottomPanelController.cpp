#include "BottomPanelController.h"

#include <QTabWidget>
#include <QWidget>

BottomPanelController::BottomPanelController(
    QTabWidget *tabs,
    QWidget *outputPanel,
    QWidget *terminalPanel,
    QWidget *problemsPanel,
    QWidget *searchResultsPanel,
    QWidget *debugPanel)
    : tabs(tabs),
      output(outputPanel),
      terminal(terminalPanel),
      problems(problemsPanel),
      search(searchResultsPanel),
      debug(debugPanel)
{
}

void BottomPanelController::showOutputPanel()
{
    setCurrent(output);
}

void BottomPanelController::showTerminalPanel()
{
    setCurrent(terminal);
}

void BottomPanelController::showProblemsPanel()
{
    setCurrent(problems);
}

void BottomPanelController::showSearchResultsPanel()
{
    setCurrent(search);
}

QString BottomPanelController::panelId(QWidget *panel) const
{
    if (panel == output) {
        return QStringLiteral("output");
    }
    if (panel == problems) {
        return QStringLiteral("problems");
    }
    if (panel == search) {
        return QStringLiteral("search");
    }
    if (panel == debug) {
        return QStringLiteral("debug");
    }
    return QStringLiteral("terminal");
}

QWidget *BottomPanelController::panelForId(const QString &id) const
{
    if (id == QStringLiteral("output")) {
        return output;
    }
    if (id == QStringLiteral("problems")) {
        return problems;
    }
    if (id == QStringLiteral("search")) {
        return search;
    }
    if (id == QStringLiteral("debug")) {
        return debug;
    }
    return terminal;
}

void BottomPanelController::setCurrent(QWidget *panel)
{
    if (tabs && panel) {
        tabs->setCurrentWidget(panel);
    }
}
