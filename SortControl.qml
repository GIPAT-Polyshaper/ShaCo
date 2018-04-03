import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

ColumnLayout {
    Layout.fillHeight: true
    Layout.margins: 3

    Text {
        text: qsTr("Sort By...")
    }

    ComboBox {
        id: comboBox1
    }
}
