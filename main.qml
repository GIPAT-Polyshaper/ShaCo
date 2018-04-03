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

    Component {
        id: mainView
        MainView {
            onShapeLibraryRequested: stack.push(shapeLibraryView)
            onStartCuttingRequested: stack.push(cutPreparationView)
        }
    }

    Component {
        id: shapeLibraryView
        ShapeLibraryView {
            onBack: stack.pop()
        }
    }

    Component {
        id: cutPreparationView
        CutPreparationView {
            onBack: stack.pop()
            onStartCutRequested: stack.push(cutView)
        }
    }

    Component {
        id: cutView
        CutView {
        }
    }
}
