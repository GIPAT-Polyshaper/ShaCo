import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back
    signal startCutRequested

    RowLayout {
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.preferredHeight: 200
        Layout.margins: 3

        Image {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            source: "qrc:/images/log_160.png"
        }

        Text {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 3
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("Shape description")
            font.pixelSize: 12
        }
    }

    TemperatureControl {
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.margins: 3
    }

    Image {
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3
        source: "qrc:/images/log_160.png"
    }

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        Button {
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
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Start!")

            onClicked: root.startCutRequested()
        }
    }
}
