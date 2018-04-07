#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    // TODO: For the moment we use the dafault QT style "Fusion", perhaps create a custom style as needed
    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
