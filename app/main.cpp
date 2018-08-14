#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <iostream>
#include "controller.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName("PolyShaper");
    QCoreApplication::setOrganizationDomain("polyshaper.eu");
    QCoreApplication::setApplicationName("ShaCo");
    QCoreApplication::setApplicationVersion("1.0.0");

    QGuiApplication app(argc, argv);
#ifdef Q_OS_MACOS
    app.setWindowIcon(QIcon(":/images/ShaCo.icns"));
#else
    app.setWindowIcon(QIcon(":/images/ShaCo.ico"));
#endif

    // TODO: For the moment we use the default QT style "Fusion", perhaps create a custom style as needed
    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;

    Controller controller;
    engine.rootContext()->setContextProperty("controller", &controller);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
