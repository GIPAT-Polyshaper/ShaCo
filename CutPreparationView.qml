import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back
    signal startCutRequested

    property var itemToCut: null

    Item {
        id: info
        Layout.fillHeight: false
        Layout.fillWidth: true
        Layout.preferredHeight: 200
        Layout.margins: 3

        Image {
            id: image
            x: 0
            y: 0
            width: 200
            height: parent.height
            source: root.itemToCut !== null ? root.itemToCut.image : ""
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignHCenter
            verticalAlignment: Image.AlignVCenter
        }

        ScrollView {
            x: image.width + 5
            y: 0
            width: parent.width - image.width - 5
            height: parent.height
            clip: true

            Text {
                height: Math.max(contentHeight, info.height)
                width: info.width
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text: (root.itemToCut === null) ?
                          "" :
                          "<b>" + root.itemToCut.name + "</b><br>" +
                          root.itemToCut.description + "<br>" +
                          "Category: <i>" + root.itemToCut.category + "</i><br>" +
                          "Working time: " + root.itemToCut.workingTime + "<br>" +
                          "Panel size: " + root.itemToCut.originalSize
            }
        }
    }

    TemperatureControl {
        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true
        Layout.margins: 3
    }

    AnimatedImage {
        id: moveToStart

        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3

        onVisibleChanged:
            if (visible) {
                currentFrame = 0
                playing = true
            } else {
                playing = false
            }

        fillMode: Image.PreserveAspectFit
        source: "qrc:/images/move_to_start.gif"
    }

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 80
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
            text: qsTr("Start!")

            onClicked: root.startCutRequested()
        }
    }
}
