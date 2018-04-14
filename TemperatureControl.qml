import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

RowLayout {
    Text {
        Layout.fillHeight: true
        Layout.fillWidth: false
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Temperature: 0%")
    }

    Slider {
        id: slider
        Layout.fillHeight: true
        Layout.fillWidth: true
        from: 0
        to: 100
        value: 50

        background: Rectangle {
            x: slider.leftPadding - width
            y: slider.topPadding + (slider.availableHeight - width) / 2
            width: slider.availableHeight / 4
            height: slider.availableWidth
            transformOrigin: Item.TopRight
            rotation: -90
            radius: 2
            gradient: Gradient {
                GradientStop { position: 0.0; color: "blue" }
                GradientStop { position: 1.0; color: "red" }
            }

            Rectangle {
                x: 0
                y: slider.visualPosition * parent.height
                width: parent.width
                height: parent.height - y
                color: "#bdbebf"
                radius: 2
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 15
            height: slider.availableHeight / 2
            color: slider.pressed ? "#f0f0f0" : "#f6f6f6"
            border.color: "#bdbebf"
        }
    }

    Text {
        Layout.fillHeight: true
        Layout.fillWidth: false
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("100%")
    }
}
