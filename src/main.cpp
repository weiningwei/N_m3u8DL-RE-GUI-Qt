#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("N_m3u8DL-RE GUI");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("N_m3u8DL-RE-GUI-Qt");

    MainWindow window;
    window.show();

    return app.exec();
}
