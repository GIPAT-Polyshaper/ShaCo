import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

RowLayout {
    property alias temperature: slider.value

    Text {
        Layout.fillHeight: true
        Layout.fillWidth: false
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Temperature: ")
    }

    Slider {
        id: slider
        Layout.fillHeight: true
        Layout.fillWidth: true
        from: 0
        to: 100
        value: 50

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
    }
}
