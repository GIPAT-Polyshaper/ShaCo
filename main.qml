import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Window 2.2

Window {
    visible: true
    minimumWidth: 600
    minimumHeight: 800
    title: qsTr("Hello World")

    StackView {
        id: stack
        initialItem: mainView
        anchors.fill: parent
    }

    MainView {
        id: mainView
        visible: false
        onShapeLibraryRequested: stack.push(shapeLibraryView)
        onStartCuttingRequested: stack.push(cutPreparationView)
    }


    ShapeLibraryView {
        id: shapeLibraryView
        visible: false
        onBack: stack.pop()
    }

    CutPreparationView {
        id: cutPreparationView
        visible: false
        onBack: stack.pop()
        onStartCutRequested: stack.push(cutView)
    }

    CutView {
        id: cutView
        visible: false

        onBack: stack.pop(mainView)
    }
}
