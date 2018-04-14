import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "qrc:/shapes/library.js" as LibraryShapes

ColumnLayout {
    id: root

    signal back

    ShapesView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3
        sortType: "shapesLibrary"

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
