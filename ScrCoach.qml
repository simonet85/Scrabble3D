import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: coachDialog
    //: dialog title
    title: qsTr("AI coach")
    standardButtons: Dialog.Cancel | Dialog.Save
    modal: true
    x: (scrabble3D.width - coachDialog.width) / 2
    y: (scrabble3D.height - coachDialog.height) / 2
    padding: 12
    width: Math.min(scrabble3D.width - 24, 460)

    property bool hasKey: false

    onAboutToShow: {
        hasKey = GamePlay.coachHasKey()
        apiKey.text = ""
        apiKey.placeholderText = hasKey ? GamePlay.coachMaskedKey() : qsTr("Paste your API key here")
        model.text = GamePlay.coachModel()
    }

    contentItem: ColumnLayout {
        spacing: 8
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("The coach explains the best moves and defines the words played. It only advises: the dictionary alone decides whether a word is valid, the answers of the coach are never used for that.")
        }
        Label { text: qsTr("API key (Anthropic):") }
        TextField {
            id: apiKey
            Layout.fillWidth: true
            echoMode: TextInput.Password
            color: palette.buttonText
            selectByMouse: true
        }
        Label { text: qsTr("Model:") }
        TextField {
            id: model
            Layout.fillWidth: true
            selectByMouse: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            color: "#b00020"
            text: qsTr("The key is stored in a file of your user profile (coach.ini), not encrypted. The coach sends the board, your rack and the best moves to the online service, which is paid per use with your key. Nothing is sent as long as you do not ask the coach.")
        }
        Button {
            text: qsTr("Remove the saved key")
            enabled: coachDialog.hasKey
            onClicked: {
                GamePlay.coachClearKey()
                coachDialog.hasKey = false
                apiKey.placeholderText = qsTr("Paste your API key here")
            }
        }
    }

    onAccepted: GamePlay.coachSave(apiKey.text, model.text)
}
