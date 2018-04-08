import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

RowLayout {
    id: root

    property int direction: Qt.AscendingOrder

    ColumnLayout {
        Layout.fillHeight: true
        Layout.fillWidth: false

        Text {
            text: qsTr("Sort By...")
        }

        ComboBox {
            textRole: "name"
            model: ListModel {
                ListElement { filter: "date"; name: qsTr("Date") }
                ListElement { filter: "name"; name: qsTr("Name") }
                ListElement { filter: "category"; name: qsTr("Category") }
            }
        }
    }

    Button {
        Layout.fillHeight: true
        Layout.fillWidth: false
        display: AbstractButton.IconOnly
        flat: true
        icon.source: root.direction === Qt.AscendingOrder ? "qrc:/images/uparrow.png" : "qrc:/images/downarrow.png"
        icon.height: 68

        onClicked: root.direction = root.direction == Qt.AscendingOrder ? Qt.DescendingOrder : Qt.AscendingOrder
    }
}
