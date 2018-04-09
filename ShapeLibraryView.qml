import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "qrc:/shapes/library.js" as LibraryShapes

ColumnLayout {
    id: root

    signal back

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        Button {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            text: qsTr("Reload")
        }

        Text {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("Hourglass")
        }

        Item {
            Layout.fillWidth: true
        }

        FilterControl {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            filtersForLocal: false
        }

        SortControl {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
        }
    }

    ShapesView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3

        shapesInfo: LibraryShapes.shapes()
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
            text: qsTr("Download")
        }
    }
}
