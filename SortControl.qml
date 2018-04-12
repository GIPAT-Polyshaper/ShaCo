import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ComboBox {
    id: root

    textRole: "name"
    model: ListModel {
        ListElement { filter: "newest"; name: qsTr("Newest") }
        ListElement { filter: "most popular"; name: qsTr("Most Popular") }
        ListElement { filter: "a-z"; name: qsTr("A...Z") }
        ListElement { filter: "z-a"; name: qsTr("Z...A") }
    }
}
