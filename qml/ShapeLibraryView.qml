import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back
    signal download(var shape)

    ShapesView {
        id: shapesView

        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3
    }

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 50
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
            text: qsTr("Download")

            // onClicked: controller.download???
        }
    }
}
