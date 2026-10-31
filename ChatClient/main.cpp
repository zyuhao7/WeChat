#include "mainwindow.h"

#include <QApplication>

#include "ElaApplication.h"
#include "ElaTheme.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // rc.qrc lives in the static chatcore lib; the link only keeps it when the
    // initializer is referenced from the executable.
    Q_INIT_RESOURCE(rc);

    // Initialize the Fluent (ElaWidgetTools) framework: registers its resources
    // and applies the base font/attributes used by all Ela widgets.
    eApp->init();

    QString fileName = "config.ini";
    QString app_path = QCoreApplication::applicationDirPath();

    QString config_path = QDir::toNativeSeparators(app_path +
                                                   QDir::separator() + fileName);

    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    gate_url_prefix = "http://" + gate_host + ":" + gate_port;

    MainWindow w;
    w.show();
    return a.exec();
}
