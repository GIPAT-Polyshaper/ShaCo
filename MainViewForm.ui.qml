import QtQuick 2.4
import QtQuick.Controls 2.3

Item {
    width: 400
    height: 600

    Column {
        id: column
        anchors.fill: parent

        Row {
            id: row
            width: 200
            height: 400

            Button {
                id: button
                text: qsTr("Button")
            }

            ComboBox {
                id: comboBox
            }

            ComboBox {
                id: comboBox1
            }
        }

        GridView {
            id: gridView
            x: 0
            y: 0
            width: 140
            height: 140
            cellHeight: 70
            model: ListModel {
                ListElement {
                    name: "Grey"
                    colorCode: "grey"
                }

                ListElement {
                    name: "Red"
                    colorCode: "red"
                }

                ListElement {
                    name: "Blue"
                    colorCode: "blue"
                }

                ListElement {
                    name: "Green"
                    colorCode: "green"
                }
            }
            delegate: Item {
                x: 5
                height: 50
                Column {
                    spacing: 5
                    Rectangle {
                        width: 40
                        height: 40
                        color: colorCode
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
            cellWidth: 70
        }

        Row {
            id: row1
            width: 200
            height: 400

            Button {
                id: button3
                text: qsTr("Button")
            }

            Button {
                id: button2
                text: qsTr("Button")
            }

            Button {
                id: button1
                text: qsTr("Button")
            }


        }

    }
}
