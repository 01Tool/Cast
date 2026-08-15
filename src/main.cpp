#include "engine/castengine.h"
#include "ui/mainwindow.h"

#include <DApplication>
#include <DIconTheme>
#include <DLog>
#include <DWidgetUtil>

#include <QCoreApplication>
#include <QIcon>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE
DGUI_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("deepin"));
    app.setApplicationName(QStringLiteral("deepin-miracast"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setProductIcon(DIconTheme::findQIcon(QStringLiteral("deepin-miracast"),
                                              DIconTheme::findQIcon(QStringLiteral("video-display"))));
    app.loadTranslator();
    app.setProductName(QCoreApplication::translate("Application", "Miracast"));
    app.setApplicationDescription(QCoreApplication::translate(
        "Application", "Mirror this computer to a Miracast wireless display."));

    if (!app.setSingleInstance(QStringLiteral("org.deepin.miracast"))) {
        qWarning() << "Another instance is running";
        return 0;
    }

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    CastEngine engine;
    MainWindow window(&engine);
    window.resize(720, 520);
    Dtk::Widget::moveToCenter(&window);
    window.show();

    return app.exec();
}
