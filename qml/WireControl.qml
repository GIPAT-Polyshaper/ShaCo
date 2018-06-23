import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

RowLayout {
    id: root

    ColumnLayout {
        Layout.fillHeight: true
        Layout.fillWidth: false

        Text {
            Layout.fillHeight: false
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("Temperature: ")
        }

        Button {
            Layout.fillHeight: false
            Layout.fillWidth: true
            checkable: true
            checked: controller.wireOn
            text: currentText()
            enabled: !controller.streamingGCode

            function currentText()
            {
                if (checked) {
                    return qsTr("Wire On")
                } else {
                    return qsTr("Wire Off")
                }
            }

            onCheckedChanged: {
                text = currentText()
                controller.wireOn = checked;
            }
        }
    }

    Slider {
        id: slider
        Layout.fillHeight: true
        Layout.fillWidth: true
        from: 0
        to: 100
        value: controller.wireTemperature

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + 3 * slider.availableHeight / 8
            width: slider.availableWidth
            height: slider.availableHeight / 4
            radius: 2
            color: "#d06804"

            Rectangle {
                x: slider.visualPosition * parent.width
                y: 0
                width:  parent.width - x
                height: parent.height
                color: "#bdbebf"
                radius: 2
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 60
            height: slider.availableHeight / 2
            radius: 5
            color: slider.pressed ? "#f0f0f0" : "#f6f6f6"
            border.color: "#bdbebf"

            Text {
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                text: slider.value.toFixed(0) + "%"
            }
        }

        onValueChanged: controller.wireTemperature = value
    }
}
