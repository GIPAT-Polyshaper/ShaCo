import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3

Window {
    id: root
    visible: true
    minimumWidth: 600
    minimumHeight: 800
    title: qsTr("ShaCo")

    property string statusText: ""
    property bool showSortControl: false
    property string sortType: "local"

    ColumnLayout {
        anchors.fill: parent

        Item {
            Layout.fillHeight: false
            Layout.preferredHeight: 40
            Layout.fillWidth: true

            Image {
                height: 40
                source: "qrc:/images/log_160.png"
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignLeft
                verticalAlignment: Image.AlignVCenter
                anchors.left: parent.left
            }

            Text {
                font.pixelSize: 30
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent

                text: "<b><i>" + root.statusText + "</b></i>"
            }

            SortControl {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: root.showSortControl
                sortType: root.sortType
            }
        }

        StackView {
            id: stack
            Layout.fillHeight: true
            Layout.fillWidth: true
            initialItem: mainView
            focus: true

            Keys.onPressed: {
                if (stack.currentItem !== terminalView && event.key === Qt.Key_T && event.modifiers === Qt.ControlModifier) {
                    stack.push(terminalView)
                    event.accepted = true
                }
            }
        }
    }

    MainView {
        id: mainView
        visible: false
        onShapeLibraryRequested: stack.push(shapeLibraryView)
        onStartCuttingRequested: stack.push(cutPreparationView)

        onVisibleChanged:
            if (visible) {
                root.showSortControl = true
                root.sortType = "local"
                root.statusText = qsTr("Your shapes")
            }
    }

    ShapeLibraryView {
        id: shapeLibraryView
        visible: false
        onBack: stack.pop()

        onVisibleChanged:
            if (visible) {
                root.showSortControl = true
                root.sortType = "shapeLibrary"
                root.statusText = qsTr("Shape library")
            }
    }

    CutPreparationView {
        id: cutPreparationView
        visible: false
        onBack: stack.pop()
        onStartCutRequested: stack.push(cutView)

        onVisibleChanged:
            if (visible) {
                root.showSortControl = false
                root.statusText = qsTr("Preparing to cut")
                cutPreparationView.itemToCut = mainView.selectedItem()
            }
    }

    CutView {
        id: cutView
        visible: false
        onBack: stack.pop(mainView)

        onVisibleChanged:
            if (visible) {
                root.showSortControl = false
                root.statusText = qsTr("Cutting...")
                cutView.itemToCut = mainView.selectedItem()
            }
    }

    TerminalView {
        id: terminalView
        visible: false
        onBack: stack.pop()

        onVisibleChanged:
            if (visible) {
                root.showSortControl = false
                root.statusText = qsTr("Terminal")
            }
    }
}
