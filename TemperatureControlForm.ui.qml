import QtQuick 2.4
import QtQuick.Controls 2.3

Item {
    width: 400
    height: 400

    Row {
        id: row
        anchors.fill: parent

        Text {
            id: text1
            text: qsTr("Text")
            font.pixelSize: 12
        }

        Slider {
            id: slider
            value: 0.5
        }
    }
}
