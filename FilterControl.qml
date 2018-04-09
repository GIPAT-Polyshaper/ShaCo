import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    property bool filtersForLocal: true

    Button {
        Layout.fillHeight: true
        text: qsTr("Filters...")

        onClicked: dialog.visible = true
    }

    Dialog {
        id: dialog
        modal: true
        visible: false
        title: qsTr("Choose Filter")
        x: (root.parent.width - width) / 2  - root.x

        ColumnLayout {
            anchors.fill: parent

            CheckBox {
                text: qsTr("Local")
                visible: root.filtersForLocal
            }

            CheckBox {
                text: qsTr("Shape Library")
                visible: root.filtersForLocal
            }

            CheckBox {
                text: qsTr("Name (use editbox)")
            }

            CheckBox {
                text: qsTr("Category (once checkbox per category?)")
            }
        }
    }
}
