#include "MainWindow.h"

#include <QApplication>
#include <QLocale>
#include <QTimer>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QLocale::setDefault(QLocale(QLocale::Arabic, QLocale::SaudiArabia));
    QApplication::setApplicationName(QStringLiteral("Arabic Code Studio"));
    QApplication::setOrganizationName(QStringLiteral("Arabic Code Studio"));

    MainWindow window;
    window.show();

    QString pathToOpen;
    int smokeExitMs = -1;
    const QStringList arguments = app.arguments();
    for (int index = 1; index < arguments.size(); ++index) {
        const QString argument = arguments.at(index);
        if (argument == QStringLiteral("--smoke-exit-ms") && index + 1 < arguments.size()) {
            bool ok = false;
            const int value = arguments.at(++index).toInt(&ok);
            if (ok && value >= 0) {
                smokeExitMs = value;
            }
            continue;
        }
        if (!argument.startsWith('-') && pathToOpen.isEmpty()) {
            pathToOpen = argument;
        }
    }

    if (!pathToOpen.isEmpty()) {
        window.openPath(pathToOpen);
    }
    if (smokeExitMs >= 0) {
        QTimer::singleShot(smokeExitMs, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
