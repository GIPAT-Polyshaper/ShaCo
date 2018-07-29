import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "shacoutils.js" as ShaCoUtils

ColumnLayout {
    id: root

    signal back

    property var itemToCut: null
    property string remainingTimeStr: itemToCut === null ? qsTr("Unknown") : computeRemainingTime(controller.cutProgress)

    function computeRemainingTime(progress)
    {
        if (progress > itemToCut.duration) {
            return qsTr("Unknown")
        } else {
            return ShaCoUtils.secondsToMMSS(itemToCut.duration - progress)
        }
    }

    Image {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 3
        source: (itemToCut !== null) ? itemToCut.image : ""
        visible: itemToCut !== null
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignVCenter
    }

    Text {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 3
        visible: itemToCut === null
        text: "<b>" + qsTr("Preview not available") + "</b>"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ProgressBar {
        Layout.fillHeight: false
        Layout.preferredHeight: 40
        Layout.fillWidth: true
        Layout.margins: 10
        from: 0
        to: (itemToCut === null) ? 1 : itemToCut.duration
        value: controller.streamingGCode ? controller.cutProgress : to
        indeterminate: controller.streamingGCode &&
                       (itemToCut === null || controller.cutProgress > itemToCut.duration)

        Text {
            id: remainingTime

            anchors.fill: parent
            z: 10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: controller.streamingGCode ? qsTr("Remaining time: ") + remainingTimeStr : qsTr("Cut Completed")
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
