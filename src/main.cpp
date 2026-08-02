#include "mainwindow.h"

#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("N_m3u8DL-RE GUI");
    QApplication::setOrganizationName("N_m3u8DL-RE-GUI-Qt");
    QApplication::setApplicationVersion("1.0.0");

    // 使用 ini 配置文件而非注册表
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // 清理旧版注册表配置（曾使用 NativeFormat 存储）
    {
        QSettings reg(QSettings::NativeFormat, QSettings::UserScope,
                      QApplication::organizationName(),
                      QApplication::applicationName());
        reg.clear();
        reg.remove("");
        reg.sync();
    }

    MainWindow window;
    window.show();

    return app.exec();
}
