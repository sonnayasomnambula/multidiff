#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName("sonnayasomnambula");
    a.setOrganizationDomain("sonnayasomnambula.org");
    a.setApplicationVersion("0.2");

    MainWindow w;
    w.show();

    return a.exec();
}
