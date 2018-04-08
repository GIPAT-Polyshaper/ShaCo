import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back

    RowLayout {
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3

        Image {
            Layout.fillWidth: false
            Layout.fillHeight: true
            source: "qrc:/images/log_160.png"
        }

        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: qsTr("Text")
            font.pixelSize: 12
        }
    }

    ProgressBar {
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.margins: 3
        value: 0.5
    }

    TemperatureControl {
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.margins: 3
    }

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        Layout.margins: 3

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
            text: qsTr("Stop")
        }

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Pause")
        }
    }
}
