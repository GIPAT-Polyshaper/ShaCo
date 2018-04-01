import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Window 2.2

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    StackView {
        id: stack
        initialItem: mainView
        anchors.fill: parent
    }

    MainView {
        id: mainView
    }
}
