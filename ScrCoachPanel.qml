import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "Coach" tab: what the AI coach was asked, which service answered, how long it took, what it said
Item {
    id: panel

    property bool newEntry: false //red dot on the tab when the coach has something new
    // shown to the user: in the mobile layout the pages of the SwipeView stay visible when they are not the current one
    readonly property bool shown: visible && (SwipeView.view ? SwipeView.isCurrentItem : true)

    Connections {
        target: GamePlay.coachModel
        function onRowsInserted() { if (!panel.shown) panel.newEntry = true }
        function onDataChanged() { if (!panel.shown) panel.newEntry = true }
    }
    onShownChanged: if (shown) newEntry = false

    function kindText(kind) {
        return kind === "auto" ? qsTr("Advice (automatic)")
             : kind === "define" ? qsTr("Definitions")
             : kind === "review" ? qsTr("Game review")
             : kind === "reviewcomment" ? qsTr("Review (coach)")
             : qsTr("Advice")
    }
    function kindColor(kind) {
        return kind === "auto" ? "#7b4fc4" : kind === "define" ? "#1b7f8c"
             : kind === "review" ? "#c26a00" : kind === "reviewcomment" ? "#a8456b" : "#2e7d32"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        // header: service in use and automatic mode
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: header.implicitHeight + 12
            color: config.myPalette.window
            border.color: config.myPalette.mid
            ColumnLayout {
                id: header
                anchors.fill: parent
                anchors.margins: 6
                spacing: 2
                RowLayout {
                    Label {
                        text: qsTr("AI coach")
                        font.bold: true
                        color: config.myPalette.windowText
                    }
                    Label {
                        Layout.fillWidth: true
                        text: GamePlay.coachServiceLabel
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        color: config.myPalette.windowText
                        opacity: 0.7
                    }
                    ToolButton {
                        text: qsTr("Game review")
                        enabled: !GamePlay.isRunning && GamePlay.reviewAvailable
                        onClicked: GamePlay.gameReview()
                        ToolTip.visible: hovered
                        ToolTip.delay: 600
                        ToolTip.text: qsTr("Compares each of your moves with the best move that was possible at that turn. Available when the game is over.")
                    }
                    ToolButton {
                        text: qsTr("Clear")
                        enabled: GamePlay.coachModel.count > 0
                        onClicked: GamePlay.coachClearHistory()
                    }
                }
                CheckBox {
                    text: qsTr("Advice at each of my turns")
                    checked: GamePlay.coachAutoAdvice
                    onToggled: GamePlay.coachAutoAdvice = checked
                    ToolTip.visible: hovered
                    ToolTip.delay: 600
                    ToolTip.text: qsTr("The coach looks at your rack at the start of your turn and explains the best moves. It needs a key and your agreement given once with Game > Coach.")
                }
                CheckBox {
                    text: qsTr("Record my best move at each turn (game review)")
                    checked: GamePlay.reviewRecording
                    onToggled: GamePlay.reviewRecording = checked
                    ToolTip.visible: hovered
                    ToolTip.delay: 600
                    ToolTip.text: qsTr("At the start of each of your turns the game computes the best move possible (about a second) and keeps only its score. At the end of the game the review compares it with what you played. Nothing is shown during the game.")
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

        ListView {
            id: list
            anchors.fill: parent
            clip: true
            spacing: 6
            model: GamePlay.coachModel
            ScrollBar.vertical: ScrollBar { }
            // the newest intervention is on top: the view goes back to it when something is added
            onCountChanged: Qt.callLater(positionViewAtBeginning)

            delegate: Rectangle {
                id: card
                width: list.width - 12
                x: 6
                radius: 6
                color: config.myPalette.base
                border.color: state === 2 ? "#c62828" : kindColor(kind)
                border.width: 1
                implicitHeight: content.implicitHeight + 14

                ColumnLayout {
                    id: content
                    anchors.fill: parent
                    anchors.margins: 7
                    spacing: 3

                    RowLayout {
                        Layout.fillWidth: true
                        Rectangle {
                            radius: 8
                            color: kindColor(kind)
                            implicitWidth: chip.implicitWidth + 12
                            implicitHeight: chip.implicitHeight + 2
                            Text {
                                id: chip
                                anchors.centerIn: parent
                                text: kindText(kind)
                                color: "white"
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }
                        Label {
                            text: when
                            font.pixelSize: 11
                            color: config.myPalette.text
                            opacity: 0.6
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: service + " · " + model
                            elide: Text.ElideRight
                            Layout.maximumWidth: card.width * 0.5
                            font.pixelSize: 11
                            color: config.myPalette.text
                            opacity: 0.6
                        }
                        Label {
                            visible: elapsed >= 0
                            text: (elapsed / 1000).toFixed(1) + " s"
                            font.pixelSize: 11
                            font.bold: true
                            color: config.myPalette.text
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: question
                        wrapMode: Text.WordWrap
                        font.pixelSize: 11
                        font.italic: true
                        color: config.myPalette.text
                        opacity: 0.65
                    }
                    RowLayout {
                        visible: state === 0
                        BusyIndicator {
                            running: state === 0
                            implicitWidth: 22
                            implicitHeight: 22
                        }
                        Label {
                            text: qsTr("The coach is thinking...")
                            color: config.myPalette.text
                        }
                    }
                    Text {
                        visible: state !== 0
                        Layout.fillWidth: true
                        text: answer
                        textFormat: Text.RichText
                        wrapMode: Text.WordWrap
                        color: state === 2 ? "#c62828" : config.myPalette.text
                    }
                }
            }
        }

        Label {
            visible: GamePlay.coachModel.count === 0
            anchors.centerIn: parent
            width: parent.width - 24
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            color: config.myPalette.text
            opacity: 0.65
            text: qsTr("No advice yet. Ask the coach with the Game menu, or tick \"Advice at each of my turns\" to watch it work during the game.")
        }
        } // Item
    }
}
