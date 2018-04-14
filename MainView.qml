import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "qrc:/shapes/local.js" as LocalShapes

ColumnLayout {
    id: root

    signal shapeLibraryRequested
    signal startCuttingRequested

    function selectedItem() {
        return shapesView.selectedItem()
    }

    ShapesView {
        id: shapesView

        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3
        sortType: "local"

        shapesInfo: LocalShapes.shapes()
    }

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Shape Library")

            onClicked: root.shapeLibraryRequested()
        }

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Import")
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Start cutting")

            onClicked: root.startCuttingRequested()
        }
    }
}
