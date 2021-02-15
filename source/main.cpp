#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef Q_OS_UNIX
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    a.setOrganizationName("sonnayasomnambula");
    a.setOrganizationDomain("sonnayasomnambula.org");
    a.setApplicationVersion("0.2");

    MainWindow w;
    w.show();

    return a.exec();
}
