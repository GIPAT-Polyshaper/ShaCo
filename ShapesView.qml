import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

GridView {
    id: root
    cellHeight: 160
    cellWidth: width / Math.floor(width / 160)
    clip: true

    property var shapesInfo
    property var selectedShapeItem: null
    property real detailsX: 0
    property real detailsY: 0

    onShapesInfoChanged: {
        var modelString = "import QtQuick 2.4; ListModel{"
        for(var s in shapesInfo) {
            var curShape = shapesInfo[s]
            var tags = ""
            for(var t in curShape.tags) {
                tags += curShape.tags[t] + ", "
            }
            tags = tags.substr(0, tags.length - 2)
            modelString += "ListElement{" +
                    "name:\"" + curShape.name + "\";" +
                    "description:\"" + curShape.description + "\";" +
                    "tags:\"" + tags + "\";" +
                    "image:\"" + curShape.image + "\";" +
                    "workingTime:\"" + curShape.workingTime + "\";" +
                    "originalSize:\"" + curShape.originalSize + "\";" +
                    "}"
        }
        modelString += "}"
        root.model = Qt.createQmlObject(modelString, root, "dynamic grid view model")
    }

    delegate: Item {
        x: 5
        height: 140

        property string shapeName: name
        property string shapeDescription: description
        property var shapeTags: tags
        property url shapeImage: image
        property string shapeWorkingTime: workingTime
        property string shapeOriginalSize: originalSize

        Column {
            spacing: 5
            Image {
                width: 120
                height: 120
                source: image
                fillMode: Image.PreserveAspectFit
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                x: 5
                text: name
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onPositionChanged: {
            // We have to compute the position in the whole view, not in the currently visible area
            var viewYDisplacement = root.visibleArea.yPosition * root.contentHeight
            var item = root.itemAt(mouse.x, mouse.y + viewYDisplacement)
            if (item !== null) {
                if (root.selectedShapeItem !== item) {
                    timer.restart()
                    detailsDialog.visible = false
                    root.selectedShapeItem = item
                    root.detailsX = item.x + item.width / 2
                    root.detailsY = item.y - viewYDisplacement + item.height / 2
                }
            } else {
                root.selectedShapeItem = null
                timer.stop()
            }
        }
    }

    Timer {
        id: timer
        interval: 1000
        running: false
        repeat:false

        onTriggered: detailsDialog.showDialog()
    }

    Dialog {
        id: detailsDialog
        modal: false
        visible: false
        x: Math.min(root.detailsX, root.width - width)
        y: Math.min(root.detailsY, root.height - height)

        function showDialog() {
            if (root.selectedShapeItem !== null) {
                image.source = root.selectedShapeItem.shapeImage
                name.text = "<b>" + root.selectedShapeItem.shapeName + "</b>"
                description.text = root.selectedShapeItem.shapeDescription
                tags.text = "Tags: <i>" + root.selectedShapeItem.shapeTags + "</i>"
                otherInfo.text = "Working time: " + root.selectedShapeItem.shapeWorkingTime + "<br>" +
                        "Panel size: " + root.selectedShapeItem.shapeOriginalSize
                detailsDialog.visible = true
            }
        }

        ColumnLayout {
            anchors.fill: parent

            Image {
                id: image
                Layout.fillHeight: false
                Layout.fillWidth: false
                Layout.preferredHeight: 240
                Layout.preferredWidth: 240
                fillMode: Image.PreserveAspectFit
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                id: name
                textFormat: Text.StyledText
                Layout.fillHeight: false
                Layout.fillWidth: false
                Layout.preferredWidth: 240
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                id: description
                textFormat: Text.StyledText
                wrapMode: Text.WordWrap
                Layout.fillHeight: false
                Layout.fillWidth: false
                Layout.preferredWidth: 240
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                id: tags
                textFormat: Text.StyledText
                wrapMode: Text.WordWrap
                Layout.fillHeight: false
                Layout.fillWidth: false
                Layout.preferredWidth: 240
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                id: otherInfo
                textFormat: Text.StyledText
                Layout.fillHeight: false
                Layout.fillWidth: false
                Layout.preferredWidth: 240
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
