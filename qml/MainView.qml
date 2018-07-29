import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal shapeLibraryRequested
    signal startCuttingRequested
    signal goToCuttingView

    property alias selectedItem: shapesView.selectedItem
    property bool fileImported: false

    ShapesView {
        id: shapesView

        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3
    }

    SortControl {
        Layout.fillWidth: true
        Layout.fillHeight: false
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
            enabled: false

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
            enabled: controller.connected && !controller.streamingGCode
            visible: !controller.streamingGCode

            onClicked: fileDialog.open()
        }

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Start cutting")
            enabled: controller.connected && shapesView.selectedItem !== null
            visible: !controller.streamingGCode

            onClicked: {
                controller.setGCodeFile(shapesView.selectedItem.gcode)
                fileImported = false
                root.startCuttingRequested()
            }
        }

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Current cut")
            enabled: controller.streamingGCode
            visible: controller.streamingGCode

            onClicked: root.goToCuttingView()
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
            fileImported = true
            root.startCuttingRequested()
        }
    }
}
