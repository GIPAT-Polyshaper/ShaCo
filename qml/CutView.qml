import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "shacoutils.js" as ShaCoUtils

ColumnLayout {
    id: root

    signal back

    property var itemToCut: theShape
    property string remainingTimeStr: theShape.imported ? qsTr("Unknown") : computeRemainingTime(controller.cutProgress)

    QtObject {
        id: theShape

        property bool imported: true
        property string image: ""
        property int duration: 0
    }

    function computeRemainingTime(progress)
    {
        if (progress > theShape.duration) {
            return qsTr("Unknown")
        } else {
            return ShaCoUtils.secondsToMMSS(theShape.duration - progress)
        }
    }

    Image {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 3
        source: theShape.imported ? "" : theShape.image
        visible: !theShape.imported
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignVCenter
    }

    Text {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 3
        visible: theShape.imported
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
        to: theShape.imported ? 1 : theShape.duration
        value: controller.streamingGCode ? controller.cutProgress : to
        indeterminate: controller.streamingGCode &&
                       (theShape.imported || controller.cutProgress > theShape.duration)

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
