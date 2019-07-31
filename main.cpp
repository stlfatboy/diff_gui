#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

#include <QDebug>
#include <updatecontroller.h>
#include <updater.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    a.setWindowIcon(QIcon(":/icon/options.ico"));
    w.startupjobs(argv[1]);
    w.show();

    return a.exec();
}
