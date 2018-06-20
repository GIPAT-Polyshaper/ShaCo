import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back

    property var itemToCut: null

//    Image {
//        Layout.fillWidth: true
//        Layout.fillHeight: true
//        Layout.margins: 3
//        source: (itemToCut != null) ? itemToCut.image : ""
//        fillMode: Image.PreserveAspectFit
//        horizontalAlignment: Image.AlignHCenter
//        verticalAlignment: Image.AlignVCenter
//    }

    Text {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 3
        text: "<b>" + qsTr("Preview not available") + "</b>"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ProgressBar {
        Layout.fillHeight: false
        Layout.preferredHeight: 40
        Layout.fillWidth: true
        Layout.margins: 10
        from: 0.0
        to: 1.0
        value: 1.0
        indeterminate: controller.streamingGCode

        Text {
            id: remainingTime

            anchors.fill: parent
            z: 10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: controller.streamingGCode ? qsTr("Remaining time: Unknown") : qsTr("Cut Completed")
        }
    }

    WireControl {
        id: wireControl

        Layout.fillHeight: false
        Layout.preferredHeight: 80
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
            enabled: !controller.stoppingStreaming && controller.streamingGCode
            text: controller.stoppingStreaming ? qsTr("Stopping...") : qsTr("Stop")

            onClicked: controller.stopStreaminGCode()
        }

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            enabled: !controller.stoppingStreaming && controller.streamingGCode
            checkable: true
            checked: controller.paused
            text: controller.paused ? qsTr("Paused") : qsTr("Pause")

            onCheckedChanged: checked ? controller.feedHold() : controller.resumeFeedHold()
        }
    }
}
