#include "engine/castengine.h"
#include "ui/mainwindow.h"

#include <DApplication>
#include <DLog>
#include <DWidgetUtil>

#include <QIcon>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("deepin"));
    app.setApplicationName(QStringLiteral("deepin-miracast"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setProductName(QStringLiteral("Miracast"));
    app.setApplicationDescription(
        QStringLiteral("Mirror this computer to a Miracast wireless display."));
    app.setProductIcon(QIcon::fromTheme(QStringLiteral("video-display")));
    app.loadTranslator();

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
