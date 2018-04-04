import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back

    RowLayout {
        id: row
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3

        Image {
            id: image
            Layout.fillWidth: false
            Layout.fillHeight: true
            source: "qrc:/log_160.png"
        }

        Text {
            id: text1
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: qsTr("Text")
            font.pixelSize: 12
        }
    }

    ProgressBar {
        id: progressBar
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.margins: 3
        value: 0.5
    }

    TemperatureControl {
        id: temperatureControl
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.margins: 3
    }

    RowLayout {
        id: row1
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        Layout.margins: 3

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
            text: qsTr("Stop")
        }

        Button {
            id: button2
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Pause")
        }
    }
}
