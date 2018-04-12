import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ColumnLayout{
    id: root

    property var shapesInfo
    property var selectedShapeItem: null
    property string currentCategory: "3D Puzzle"
    property real detailsX: 0
    property real detailsY: 0

    function showDialog() {
        if (root.selectedShapeItem !== null) {
            image.source = root.selectedShapeItem.shapeImage
            name.text = "<b>" + root.selectedShapeItem.shapeName + "</b>"
            description.text = root.selectedShapeItem.shapeDescription
            category.text = "Category: <i>" + root.selectedShapeItem.shapeCategory + "</i>"
            otherInfo.text = "Working time: " + root.selectedShapeItem.shapeWorkingTime + "<br>" +
                    "Panel size: " + root.selectedShapeItem.shapeOriginalSize
            detailsDialog.visible = true
        }
    }

    onVisibleChanged: {
        if (visible === true) {
            root.selectedShapeItem = null
            timer.stop()
        }
    }

    onShapesInfoChanged: generateModel()
    onCurrentCategoryChanged: generateModel()

    function generateModel() {
        var modelString = "import QtQuick 2.4; ListModel{"
        for(var s in shapesInfo) {
            var curShape = shapesInfo[s]

            if (curShape.category === root.currentCategory) {
                modelString += "ListElement{" +
                        "name:\"" + curShape.name + "\";" +
                        "description:\"" + curShape.description + "\";" +
                        "image:\"" + curShape.image + "\";" +
                        "category:\"" + curShape.category + "\";" +
                        "workingTime:\"" + curShape.workingTime + "\";" +
                        "originalSize:\"" + curShape.originalSize + "\";" +
                        "}"
            }
        }
        modelString += "}"
        grid.model = Qt.createQmlObject(modelString, grid, "dynamic grid view model")
    }

    ButtonGroup {
        id: categoryGroup
    }

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 40
        Layout.fillWidth: true

        Image {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            Layout.preferredWidth: sortControl.width
            source: "qrc:/images/log_160.png"
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignLeft
            verticalAlignment: Image.AlignVCenter
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Button {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            text: qsTr("3D Puzzle")
            flat: true
            checkable: true
            checked: true
            ButtonGroup.group: categoryGroup

            onCheckedChanged: if (checked) root.currentCategory = "3D Puzzle"
        }

        Button {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            text: qsTr("Stencil")
            flat: true
            checkable: true
            ButtonGroup.group: categoryGroup

            onCheckedChanged: if (checked) root.currentCategory = "Stencil"
        }

        Button {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            text: qsTr("Decor")
            flat: true
            checkable: true
            ButtonGroup.group: categoryGroup

            onCheckedChanged: if (checked) root.currentCategory = "Decor"
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        SortControl {
            id: sortControl
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
        }
    }

    GridView {
        id: grid
        Layout.fillHeight: true
        Layout.fillWidth: true
        cellHeight: 160
        cellWidth: width / Math.floor(width / 160)
        clip: true

        delegate: Item {
            x: 5
            height: 140

            property string shapeName: name
            property string shapeDescription: description
            property string shapeCategory: category
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
                var viewYDisplacement = grid.visibleArea.yPosition * grid.contentHeight
                var item = grid.itemAt(mouse.x, mouse.y + viewYDisplacement)
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
    }

    Timer {
        id: timer
        interval: 2000
        running: false
        repeat:false

        onTriggered: root.showDialog()
    }

    Dialog {
        id: detailsDialog
        modal: false
        visible: false
        x: Math.min(root.detailsX, grid.width - width)
        y: Math.min(root.detailsY, grid.height - height)

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
                id: category
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
