import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal shapeLibraryRequested
    signal startCuttingRequested

    function selectedItem() {
        return shapesView.selectedItem()
    }

    function addShape(shape) {
        // This is to trigger a shapesInfoChanged signal in shapesView
        var curShapes = shapesView.shapesInfo
        curShapes.push(shape)
        shapesView.shapesInfo = curShapes
    }

    ShapesView {
        id: shapesView

        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3
        showPersonalCategory: true

        shapesInfo: []
    }

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 50
        Layout.fillWidth: true

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Shape Library")

            onClicked: root.shapeLibraryRequested()
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Import && Cut")
            enabled: controller.connected

            onClicked: fileDialog.open()
        }

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Start cutting")
            enabled: false //controller.connected

            onClicked: root.startCuttingRequested()
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a GCode file"
        folder: shortcuts.home
        selectMultiple: false
        selectExisting: true
        nameFilters: ["GCode files (*.gcode)", "All files (*)"]

        onAccepted: {
            controller.setGCodeFile(fileUrl)
            root.startCuttingRequested()
        }
    }
}
