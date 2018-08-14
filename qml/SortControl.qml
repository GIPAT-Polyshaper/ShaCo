import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

RowLayout {
    id:root

    Text {
        Layout.fillHeight: true
        Layout.fillWidth: false
        text: "Sort by: "
    }

    ComboBox {
        id: comboBox

        Layout.fillHeight: true
        Layout.fillWidth: true
        textRole: "name"

        model: ListModel {
            ListElement { filter: "newest"; name: qsTr("Newest") }
            ListElement { filter: "a-z"; name: qsTr("A...Z") }
            ListElement { filter: "z-a"; name: qsTr("Z...A") }
        }

        onActivated: controller.changeLocalShapesSort(model.get(currentIndex).filter)

        onVisibleChanged:
            if (visible) {
                controller.changeLocalShapesSort(model.get(currentIndex).filter)
            }
    }
}
