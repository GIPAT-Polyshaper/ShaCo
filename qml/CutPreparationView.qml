import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "shacoutils.js" as ShaCoUtils

ColumnLayout {
    id: root

    signal back
    signal startCutRequested

    property var itemToCut: theShape

    QtObject {
        id: theShape

        property bool imported: true
        property string name: ""
        property string image: ""
        property int duration: 0
        property string description: ""
        property double panelX: 0.0
        property double panelY: 0.0
    }

    Item {
        id: info
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3

        Image {
            id: image
            x: 0
            y: 0
            width: 200
            height: parent.height
            source: theShape.imported ? "" : theShape.image
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
                text: theShape.imported ?
                          "<b>" + qsTr("Preview not available") + "</b>" :
                          "<b>" + theShape.name + "</b><br>" +
                          theShape.description + "<br>" +
                          qsTr("Working time") + ": " + ShaCoUtils.secondsToMMSS(theShape.duration) + "<br>" +
                          qsTr("Panel size") + ": " + ShaCoUtils.panelSize(theShape.panelX, theShape.panelY)
            }
        }
    }

    WireControl {
        id: wireControl

        Layout.fillHeight: false
        Layout.preferredHeight: 80
        Layout.fillWidth: true
        Layout.margins: 3
    }

    AnimatedImage {
        id: moveToStart

        Layout.fillHeight: false
        Layout.preferredHeight: 300
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
            enabled: controller.senderCreated && controller.connected
            text: qsTr("Start!")

            onClicked: root.startCutRequested()
        }
    }
}
