#include "MainWindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QLocale::setDefault(QLocale(QLocale::Arabic, QLocale::SaudiArabia));
    QApplication::setApplicationName(QStringLiteral("Arabic Code Studio"));
    QApplication::setOrganizationName(QStringLiteral("Arabic Code Studio"));

    MainWindow window;
    window.show();

    return app.exec();
}

