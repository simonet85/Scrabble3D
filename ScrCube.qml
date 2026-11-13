import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
//import QtQuick3D // unavailable in this deployment

Item {

    TextMetrics {
        id: textMetrics
        text: "10" //avoid jumping UI when label goes from one to two digits
    }

    function roll() { //animate on keypress
    }

    Item {
        id: view3dPlaceholder // QtQuick3D unavailable in this deployment
    }
    RowLayout {
        id: topPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 50
        Label {
            Layout.leftMargin: 8
            //: please keep it short; pos/dim are shown in the cube header
            text: qsTr("Pos: ")
        }
        Slider {
            id: pos3D
            implicitWidth: 100
            from: 0
            to: GamePlay.is3D ? GamePlay.boardSize-1 : 0
            value: GamePlay.activePosition
            onValueChanged: GamePlay.activePosition = value
            snapMode: Slider.SnapAlways
            stepSize: 1
        }
        Label {
            text: pos3D.value
            Layout.preferredWidth: textMetrics.width
        }
        Label {
            Layout.leftMargin: 16
            //: please keep it short; pos/dim are shown in the cube header
            text: qsTr("Dim: ")
        }
        Rectangle {
            height: 38
            width: 38
            color: "lightgrey"
            border.width: GamePlay.activeDimension === 0 ? 3 : 0
            Image {
                anchors.fill: parent
                anchors.margins: 6
                source: "qrc:///resources/abscissa.png"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: GamePlay.activeDimension = 0
            }
        }
        Rectangle {
            height: 38
            width: 38
            color: "lightgrey"
            border.width: GamePlay.activeDimension === 1 ? 3 : 0
            Image {
                anchors.fill: parent
                anchors.margins: 6
                source: "qrc:///resources/ordinate.png"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: GamePlay.activeDimension = 1
            }
        }
        Rectangle {
            height: 38
            width: 38
            color: "lightgrey"
            border.width: GamePlay.activeDimension === 2 ? 3 : 0
            Image {
                anchors.fill: parent
                anchors.margins: 6
                source: "qrc:///resources/applicate.png"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: GamePlay.activeDimension = 2
            }
        }
        Item {
            Layout.fillWidth: true
        }
    }
    Image {
        id: showTopPanel
        width: 24
        height: 24
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 16
        anchors.topMargin: 16
        source: "qrc:///resources/up.png"
        MouseArea {
            anchors.fill: parent
            onClicked: {
                topPanel.visible = !topPanel.visible;
                if (topPanel.visible)
                    parent.source = "qrc:///resources/up.png"
                else
                    parent.source = "qrc:///resources/down.png"
            }
        }
    }
}
