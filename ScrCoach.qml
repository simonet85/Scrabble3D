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

    readonly property var providers: ["groq", "gemini", "anthropic", "opencode"]
    property bool hasKey: false

    function currentProvider() { return providers[provider.currentIndex] }

    // fills the fields with what is saved for the selected service
    function showProvider() {
        const info = GamePlay.coachInfo(currentProvider())
        hasKey = info.hasKey
        apiKey.text = ""
        apiKey.placeholderText = hasKey ? info.masked : qsTr("Paste your API key here")
        model.text = info.model
    }

    onAboutToShow: {
        provider.currentIndex = Math.max(0, providers.indexOf(GamePlay.coachProvider()))
        showProvider()
    }

    contentItem: ColumnLayout {
        spacing: 8
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("The coach explains the best moves and defines the words played. It only advises: the dictionary alone decides whether a word is valid, the answers of the coach are never used for that.")
        }
        Label { text: qsTr("Service:") }
        ComboBox {
            id: provider
            Layout.fillWidth: true
            model: ["Groq", "Gemini (Google)", "Anthropic", "OpenCode"]
            onActivated: coachDialog.showProvider()
        }
        Label { text: qsTr("API key:") }
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
            text: qsTr("The key is stored encrypted in the Windows Credential Manager (not in coach.ini). The coach sends the board, your rack and the best moves to the online service, which may charge your key. Nothing is sent as long as you do not ask the coach.")
        }
        Button {
            text: qsTr("Remove the saved key")
            enabled: coachDialog.hasKey
            onClicked: {
                GamePlay.coachClearKey(coachDialog.currentProvider())
                coachDialog.showProvider()
            }
        }
    }

    onAccepted: GamePlay.coachSave(currentProvider(), apiKey.text, model.text)
}
