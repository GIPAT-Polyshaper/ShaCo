import QtQuick 2.4
import QtQuick.Controls 2.3

Item {
    width: 400
    height: 600

    Column {
        id: column

        Row {
            id: row
            width: 200
            height: 400

            Image {
                id: image
                width: 100
                height: 100
                source: "qrc:/qtquickplugin/images/template_image.png"
            }

            Text {
                id: text1
                text: qsTr("Text")
                font.pixelSize: 12
            }
        }

        ProgressBar {
            id: progressBar
            value: 0.5
        }

        TemperatureControl {
            id: temperatureControl
        }

        Row {
            id: row1
            width: 200
            height: 400

            Button {
                id: button
                text: qsTr("Button")
            }

            Button {
                id: button1
                text: qsTr("Button")
            }

            Button {
                id: button2
                text: qsTr("Button")
            }
        }
    }
}
