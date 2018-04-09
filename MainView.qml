import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import "qrc:/shapes/local.js" as LocalShapes

ColumnLayout {
    id: root

    signal shapeLibraryRequested
    signal startCuttingRequested

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        Button {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            text: qsTr("Info")
        }

        Item {
            Layout.fillWidth: true
        }

        FilterControl {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            filtersForLocal: true
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
