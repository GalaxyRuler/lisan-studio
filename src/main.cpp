#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QLocale>
#include <QTimer>
#include <QTranslator>

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

int main(int argc, char *argv[])
{
    QString pathToOpen;
    int smokeExitMs = -1;
    for (int index = 1; index < argc; ++index) {
        const std::string argument(argv[index]);
        if (argument == "--smoke-exit-ms" && index + 1 < argc) {
            const int value = std::atoi(argv[++index]);
            if (value >= 0) {
                smokeExitMs = value;
            }
            continue;
        }
        if (!argument.empty() && argument.front() != '-' && pathToOpen.isEmpty()) {
            pathToOpen = QString::fromLocal8Bit(argv[index]);
        }
    }

    if (smokeExitMs < 0) {
        bool ok = false;
        const int value = qEnvironmentVariableIntValue("LISAN_STUDIO_SMOKE_EXIT_MS", &ok);
        if (ok && value >= 0) {
            smokeExitMs = value;
        }
    }

    if (smokeExitMs >= 0) {
        const int hardExitMs = smokeExitMs + 5000;
        std::thread([hardExitMs]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(hardExitMs));
            std::exit(0);
        }).detach();
    }

    QApplication app(argc, argv);
    QLocale::setDefault(QLocale(QLocale::Arabic, QLocale::SaudiArabia));
    QApplication::setApplicationName(QStringLiteral("Lisan Studio"));
    QApplication::setOrganizationName(QStringLiteral("Lisan Studio"));

    MainWindow window;
    window.show();

    if (smokeExitMs >= 0) {
        QTimer::singleShot(smokeExitMs, &app, [&app, &window]() {
            window.close();
            app.quit();
            QCoreApplication::exit(0);
        });
    }
    if (!pathToOpen.isEmpty()) {
        if (smokeExitMs >= 0) {
            const auto openRequestedPath = [&window, pathToOpen]() {
                window.openPath(pathToOpen);
            };
            QTimer::singleShot(0, &window, openRequestedPath);
        } else {
            window.openPath(pathToOpen);
        }
    }

    return app.exec();
}
