import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back
    signal startCutRequested

    RowLayout {
        id: row
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.preferredHeight: 200
        Layout.margins: 3

        Image {
            id: image
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            source: "qrc:/log_160.png"
        }

        Text {
            id: text1
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 3
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("Shape description")
            font.pixelSize: 12
        }
    }

    Image {
        id: image1
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3
        source: "qrc:/log_160.png"
    }

    TemperatureControl {
        id: temperatureControl
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.margins: 3
    }

    RowLayout {
        id: row2
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        Button {
            id: button
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Back")

            onClicked: root.back()
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            id: button1
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Start!")

            onClicked: root.startCutRequested()
        }
    }
}
