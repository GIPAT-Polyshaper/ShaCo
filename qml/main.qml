import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.1

Window {
    id: root
    visible: true
    minimumWidth: 450
    minimumHeight: 600
    title: qsTr("ShaCo")

    property string statusText: ""
    property bool showSortControl: false
    property string sortType: "local"
    property string machineName: ""
    property string firmwareVersion: ""

    Connections {
        target: controller
        onStartedPortDiscovery: {
            logoAnimationTimer.running = true
            root.machineName = ""
            root.firmwareVersion = ""
        }
        onPortFound: {
            logoAnimationTimer.stopAndResetImage()
            root.machineName = machineName
            root.firmwareVersion = firmwareVersion
        }
        onPortClosedWithError: {
            serialPortErrorDialog.errorReason = reason
            serialPortErrorDialog.open()
        }
    }

    MessageDialog {
         id: serialPortErrorDialog
         title: qsTr("Serial port communication error")
         text: qsTr("The serial port was closed due to an error. Reason: ") + errorReason
         icon: StandardIcon.Critical
         standardButtons: StandardButton.Ok
         visible: false

         property string errorReason: ""
    }

    ColumnLayout {
        anchors.fill: parent

        Item {
            Layout.fillHeight: false
            Layout.preferredHeight: 40
            Layout.fillWidth: true

            Image {
                id: logoImage

                height: 40
                source: "qrc:/images/logo.png"
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignLeft
                verticalAlignment: Image.AlignVCenter
                anchors.left: parent.left

                MouseArea {
                    anchors.fill: parent

                    onClicked: logoImageToolTip.visible = true
                }

                ToolTip {
                    id: logoImageToolTip
                    delay: 0
                    timeout: 3000
                    visible: false
                    text: (root.machineName == "") ? qsTr("Searching machine...") : qsTr(root.machineName + " [" + root.firmwareVersion +"]")
                }
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

            Timer {
                id: logoAnimationTimer
                interval: 100
                running: true
                repeat: true

                property bool fading: true

                function stopAndResetImage() {
                    fading = true
                    logoImage.opacity = 1.0
                    running = false
                }

                onTriggered:
                    if (fading) {
                        if (logoImage.opacity == 0.0) {
                            fading = false
                            logoImage.opacity = 0.1
                        } else {
                            logoImage.opacity = Math.max(0.0, logoImage.opacity - 0.1001)
                        }
                    } else {
                        if (logoImage.opacity == 1.0) {
                            fading = true
                            logoImage.opacity = 0.9
                        } else {
                            logoImage.opacity = Math.min(1.0, logoImage.opacity + 0.1001)
                        }
                    }
            }
        }

        StackView {
            id: stack
            Layout.fillHeight: true
            Layout.fillWidth: true
            initialItem: mainView
            focus: true

            Keys.onPressed:
                if (stack.currentItem !== terminalView && event.key === Qt.Key_T && event.modifiers === Qt.ControlModifier) {
                    stack.push(terminalView)
                    event.accepted = true
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

        onDownload: {
            mainView.addShape(shape)
            stack.pop()
        }

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
        onStartCutRequested: {
            controller.startStreamingGCode()
            stack.push(cutView)
        }

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
                cutView.startTimer()
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
