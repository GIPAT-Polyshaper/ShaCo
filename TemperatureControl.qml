import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

RowLayout {
    id: row

    Text {
        Layout.fillHeight: true
        Layout.preferredHeight: 80
        Layout.fillWidth: false
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Temperature")

        font.pixelSize: 12
    }

    Slider {
        Layout.fillHeight: true
        Layout.preferredHeight: 80
        Layout.fillWidth: true

        value: 0.5
    }
}
