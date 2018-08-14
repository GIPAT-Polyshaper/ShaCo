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
    property string machineInfo: ""

    onClosing:
        if (controller.streamingGCode) {
            close.accepted = false
            confirmExit.open()
        }

    Connections {
        target: controller
        onStartedPortDiscovery: {
            logoAnimationTimer.running = true
            root.machineInfo = ""
        }
        onPortFound: {
            logoAnimationTimer.stopAndResetImage()
            root.machineInfo = machineName + " [" + partNumber + " " + serialNumber + " " + firmwareVersion + "]"
        }
        onPortClosedWithError:
            if (!errorDialog.visible) {
                errorDialog.title = qsTr("Serial port communication error")
                errorDialog.text = qsTr("The serial port was closed due to an error. Reason: ") + reason
                errorDialog.open()
            }
        onStreamingEndedWithError:
            if (!errorDialog.visible) {
                errorDialog.title = qsTr("GCode streaming error")
                errorDialog.text = qsTr("GCode streaming failed with error. Reason: ") + reason
                errorDialog.open()
            }
    }

    MessageDialog {
         id: confirmExit
         title: qsTr("Confirm exit")
         text: qsTr("There is a works in progress. Interrupt and exit?")
         icon: StandardIcon.Question
         standardButtons: StandardButton.Yes | StandardButton.No
         visible: false

         onYes: {
             controller.stopStreaminGCode()
             Qt.quit()
         }
    }

    MessageDialog {
         id: errorDialog
         icon: StandardIcon.Critical
         standardButtons: StandardButton.Ok
         visible: false
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
                    text: (root.machineInfo == "") ? qsTr("Searching machine...") : qsTr(root.machineInfo)
                }
            }

            AnimatedImage {
                id: streamingImage

                height: 20
                visible: controller.streamingGCode
                playing: controller.streamingGCode
                source: "qrc:/images/gears.gif"
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignLeft
                verticalAlignment: Image.AlignVCenter
                anchors.left: logoImage.right
                anchors.top: parent.top
            }

            Text {
                font.pixelSize: 30
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent

                text: "<b><i>" + root.statusText + "</b></i>"
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

            Button {
                height: 40
                width: 40
                anchors.right: parent.right
                icon.source: "qrc:/images/reload.png"
                icon.width: 22
                icon.height: 22
                flat: true
                enabled: true
                visible: stack.currentItem === mainView

                onClicked: controller.reloadShapes()
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
        onGoToCuttingView: stack.push(cutView)

        onVisibleChanged:
            if (visible) {
                root.statusText = qsTr("Your shapes")
            }
    }

    ShapeLibraryView {
        id: shapeLibraryView
        visible: false
        onBack: stack.pop()

        onVisibleChanged:
            if (visible) {
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
                root.statusText = qsTr("Preparing to cut")
                if (mainView.fileImported) {
                    cutPreparationView.itemToCut.imported = true
                } else {
                    cutPreparationView.itemToCut.imported = false
                    cutPreparationView.itemToCut.name = mainView.selectedItem.name
                    cutPreparationView.itemToCut.image = mainView.selectedItem.image
                    cutPreparationView.itemToCut.duration = mainView.selectedItem.duration
                    cutPreparationView.itemToCut.description = mainView.selectedItem.description
                    cutPreparationView.itemToCut.panelX = mainView.selectedItem.panelX
                    cutPreparationView.itemToCut.panelY = mainView.selectedItem.panelY
                }
            }
    }

    CutView {
        id: cutView
        visible: false
        onBack: stack.pop(mainView)

        onVisibleChanged:
            if (visible) {
                root.statusText = Qt.binding(function() {
                    return controller.streamingGCode ? qsTr("Cutting...") : qsTr("Cut Completed")
                })

                cutView.itemToCut.imported = cutPreparationView.itemToCut.imported
                cutView.itemToCut.image = cutPreparationView.itemToCut.image
                cutView.itemToCut.duration = cutPreparationView.itemToCut.duration
            }
    }

    TerminalView {
        id: terminalView
        visible: false
        onBack: stack.pop()

        onVisibleChanged:
            if (visible) {
                root.statusText = qsTr("Terminal")
            }
    }
}
