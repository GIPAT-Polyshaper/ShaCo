import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back

    property var itemToCut: null
    property alias temperature: temperatureControl.temperature
    property int workingTime: 1000
    property int remainingTime: 0

    function startTimer() {
        if (itemToCut != null) {
            var splitted = itemToCut.workingTime.split(":")
            root.workingTime = Number(splitted[0]) * 60 + Number(splitted[1])
            root.remainingTime = root.workingTime
            timer.start()
        }
    }

    Image {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 3
        source: (itemToCut !== null) ? itemToCut.image : ""
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignVCenter
    }

    ProgressBar {
        Layout.fillHeight: false
        Layout.preferredHeight: 40
        Layout.fillWidth: true
        Layout.margins: 10
        from: 0.0
        to: 1.0
        value: 1.0 - (root.remainingTime / root.workingTime)

        Text {
            id: remainingTime

            anchors.fill: parent
            z: 10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("Remaining time: ") + Math.floor(root.remainingTime / 60) + ":" + (root.remainingTime % 60 < 10 ? "0" + (root.remainingTime % 60) : root.remainingTime % 60)
        }
    }

    TemperatureControl {
        id: temperatureControl

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
            text: qsTr("Stop")
        }

        Button {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.margins: 3
            text: qsTr("Pause")
        }
    }

    Timer {
        id: timer
        interval: 1000
        running: false
        repeat: true

        onTriggered:
            if (root.remainingTime > 0) {
                root.remainingTime = root.remainingTime - 1
            } else {
                timer.stop()
            }
    }
}
