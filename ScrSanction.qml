import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// FISF 6.4: the arbiter gives a graded sanction to a player
Dialog {
    id: sanctionDialog
    //: dialog title
    title: qsTr("Sanction")
    standardButtons: Dialog.Cancel | Dialog.Ok
    modal: true
    x: (scrabble3D.width - sanctionDialog.width) / 2
    y: (scrabble3D.height - sanctionDialog.height) / 2
    padding: 12
    width: Math.min(scrabble3D.width - 24, 420)

    function prepare() {
        var names = []
        for (var i = 0; i < GamePlay.numberOfPlayers; i++)
            names.push(GamePlay.getPlayerName(i))
        playerBox.model = names
        playerBox.currentIndex = GamePlay.currentPlayer
        reasonField.text = ""
        updateNext()
    }
    function updateNext() {
        nextLabel.text = qsTr("Sanction applied: %1").arg(GamePlay.nextSanction(playerBox.currentIndex))
    }

    onAboutToShow: prepare()
    onAccepted: GamePlay.sanction(playerBox.currentIndex, reasonField.text)

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Graded scale: reminder of the rules, then 5 points, 50 points, game lost and exclusion.")
        }
        Label { text: qsTr("Player:") }
        ComboBox {
            id: playerBox
            Layout.fillWidth: true
            onCurrentIndexChanged: sanctionDialog.updateNext()
        }
        Label { text: qsTr("Infraction:") }
        TextField {
            id: reasonField
            Layout.fillWidth: true
            placeholderText: qsTr("for example: tiles drawn before the clock was started")
        }
        Label {
            id: nextLabel
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.bold: true
        }
    }
}
