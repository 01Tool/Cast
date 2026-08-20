#include "dbus/castdbus.h"
#include "dbus/castdbusservice.h"
#include "engine/castengine.h"
#include "ui/mainwindow.h"

#include <DApplication>
#include <DIconTheme>
#include <DLog>
#include <DWidgetUtil>

#include <QCoreApplication>
#include <QDebug>
#include <QIcon>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE
DGUI_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("01tool"));
    app.setApplicationName(QStringLiteral("ot-cast"));
    app.setApplicationVersion(QStringLiteral("0.2.0"));
    app.setApplicationHomePage(QStringLiteral("https://01tool.com"));
    app.setProductIcon(DIconTheme::findQIcon(QStringLiteral("ot-cast"),
                                              DIconTheme::findQIcon(QStringLiteral("video-display"))));
    app.loadTranslator();
    app.setProductName(QCoreApplication::translate("Application", "Cast"));
    app.setApplicationDescription(QCoreApplication::translate(
        "Application", "Cast this screen to a TV or wireless display."));

    if (!app.setSingleInstance(QStringLiteral("com.01tool.cast"))) {
        qWarning() << "Another instance is running";
        return 0;
    }

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    const bool background = app.arguments().contains(QStringLiteral("--background"));

    CastEngine engine;
    MainWindow window(&engine);
    window.resize(720, 520);
    Dtk::Widget::moveToCenter(&window);

    CastDBusService dbus(&engine);
    if (!dbus.registerService())
        qWarning() << "Could not register" << CastDBus::service << "on the session bus";
    QObject::connect(&dbus, &CastDBusService::raiseRequested, &window, &MainWindow::showAndRaise);
    QObject::connect(&app, &DApplication::newInstanceStarted, &window, &MainWindow::showAndRaise);

    if (!background)
        window.show();

    return app.exec();
}
