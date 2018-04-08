import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back

    TextArea {
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Here list of commands exchanged with microcontroller")
        readOnly: true
        font.pixelSize: 12
    }


    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        Text {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("Send command:")
            font.pixelSize: 12
        }

        TextEdit {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 3
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text: qsTr("command...")
            font.pixelSize: 12
        }

        Button {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            text: qsTr("Send")
        }
    }

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        Button {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            text: qsTr("Back")

            onClicked: root.back()
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
