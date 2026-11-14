import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    columns: 2
    columnSpacing: 8
    rowSpacing: 8

    property int defaultLetterSet: 0
    onDefaultLetterSetChanged: {
        sbPasses.value = defaults.languages[defaultLetterSet].numberOfPasses
        sbBingo.value = defaults.languages[defaultLetterSet].scrabbleBonus
        sbBingoExtra.value = defaults.languages[defaultLetterSet].scrabbleExtraBonus ?? 0
        sbGameEnd.value = defaults.languages[defaultLetterSet].endBonus
        cbAddLetters.checked = defaults.languages[defaultLetterSet].addLetters
        cbSubtractLetters.checked = defaults.languages[defaultLetterSet].subtractLetters
        sbJokerPenalty.value = defaults.languages[defaultLetterSet].jokerPenalty
        cbChangeIsPass.checked = defaults.languages[defaultLetterSet].changeIsPass
        cbCambioSecco.checked = defaults.languages[defaultLetterSet].cambioSecco
    }

    property alias sbPasses: sbPasses
    property alias sbBingo: sbBingo
    property alias sbBingoExtra: sbBingoExtra
    property alias sbGameEnd: sbGameEnd
    property alias cbAddLetters: cbAddLetters
    property alias cbSubtractLetters: cbSubtractLetters
    property alias sbJokerPenalty: sbJokerPenalty
    property alias cbChangeIsPass: cbChangeIsPass
    property alias cbCambioSecco: cbCambioSecco
    property alias cbLevel: cbLevel

    ColorLabel {
        id: lbPasses
        Layout.leftMargin: 8
        Layout.topMargin: 8
        Layout.alignment: Qt.AlignRight
        text: qsTr("Passes until game end:")
    }
    RowLayout {
        Layout.topMargin: 8
        ColorSpinBox {
            id: sbPasses
            from: 1
            to: 99
            value: 3
            onValueChanged: config.numberOfPasses = value
        }
        InfoTip { tiptext: qsTr("The number of moves each player can pass consecutively until the game ends") }
    }

    ColorLabel {
        id: lbBingo
        Layout.leftMargin: 8
        Layout.alignment: Qt.AlignRight
        text: qsTr("Bonus for Scrabble/Bingos:")
    }
    RowLayout {
        ColorSpinBox {
            id: sbBingo
            from: 0
            to: 999
            value: 50
            onValueChanged: config.bingoBonus = value
        }
        InfoTip { tiptext: qsTr("The 'bingo' bonus when all tiles from the rack are placed at once") }
    }

    ColorLabel {
        id: lbBingoExtra
        Layout.leftMargin: 8
        Layout.alignment: Qt.AlignRight
        text: qsTr("Extra bonus per additional tile:")
    }
    RowLayout {
        ColorSpinBox {
            id: sbBingoExtra
            from: 0
            to: 999
            value: 0
            onValueChanged: config.bingoExtraBonus = value
        }
        InfoTip { tiptext: qsTr("Original formulas with more than 7 tiles on the rack: each tile beyond the seventh adds this bonus to the 'bingo' bonus") }
    }

    ColorLabel {
        id: lbGameEnd
        Layout.leftMargin: 8
        Layout.alignment: Qt.AlignRight
        text: qsTr("Bonus on game end:")
    }
    RowLayout {
        ColorSpinBox {
            id: sbGameEnd
            from: 0
            to: 999
            value: 0
            onValueChanged: config.gameEndBonus = value
        }
        InfoTip { tiptext: qsTr("Extra bonus awarded to the player who ends the game") }
    }

    ColorLabel {
        id: lbJokerPenalty
        Layout.leftMargin: 8
        Layout.alignment: Qt.AlignRight
        text: qsTr("Penalty for left blanks:")
    }
    RowLayout {
        ColorSpinBox {
            id: sbJokerPenalty
            from: 0
            to: 999
            value: 0
            onValueChanged: config.jokerPenalty = value
        }
        InfoTip { tiptext: qsTr("Extra penalty for blank tile on the rack when the game ends") }
    }

    RowLayout {
        Layout.columnSpan: 2
        Layout.leftMargin: 8
        ColorCheckBox {
            id: cbAddLetters
            text: qsTr("Add the value of other players letters")
            onCheckStateChanged: config.addLetters = checked
        }
        InfoTip { tiptext: qsTr("The sum of remaining tiles from the opponents is added to the result of the player who ended the game") }
    }
    RowLayout {
        Layout.columnSpan: 2
        Layout.leftMargin: 8
        ColorCheckBox {
            id: cbSubtractLetters
            text: qsTr("Subtract the value of left letters")
            onCheckStateChanged: config.subtractLetters = checked
        }
        InfoTip { tiptext: qsTr("The value of the remaining tiles is deducted from each player's result") }
    }
    RowLayout {
        Layout.columnSpan: 2
        Layout.leftMargin: 8
        ColorCheckBox {
            id: cbChangeIsPass
            text: qsTr("Treat exchange as pass")
            onCheckStateChanged: config.changeIsPass = checked
        }
        InfoTip { tiptext: qsTr("Check this option to count an exchange of letters as a pass") }
    }
    RowLayout {
        Layout.columnSpan: 2
        Layout.leftMargin: 8
        ColorCheckBox {
            id: cbCambioSecco
            text: qsTr("Cambio Secco")
            onCheckStateChanged: config.cambiosecco = checked
        }
        InfoTip { tiptext: qsTr("Once per game you can exchange all pieces on the rack without losing the right to place") }
    }

    ColorLabel {
        Layout.leftMargin: 8
        Layout.topMargin: 16
        text: qsTr("Hints per game:")
    }
    RowLayout {
        Layout.columnSpan: 2
        Layout.leftMargin: 16
        Layout.topMargin: 16
        ColorSpinBox {
            from: 0
            to: 20
            value: GamePlay.hintsPerGame
            onValueModified: GamePlay.hintsPerGame = value
        }
        InfoTip { tiptext: qsTr("How many times the Hint button can be used in a game. The first hint of a turn shows where a good move can be played, a second one shows the word. 0 turns the button off.") }
    }

    ColorLabel {
        Layout.leftMargin: 8
        Layout.topMargin: 16
        text: qsTr("Computer level:")
    }
    RowLayout {
        Layout.columnSpan: 2
        Layout.leftMargin: 16
        ColorComboBox {
            id: cbLevel
            Layout.minimumWidth: 160
            model: [qsTr("Beginner"), qsTr("Intermediate"), qsTr("Advanced"), qsTr("Master")]
            currentIndex: 3
            onCurrentIndexChanged: config.computerLevel = currentIndex
        }
        InfoTip {
            tiptext: qsTr("Beginner: plays ordinary moves\nIntermediate: plays one of the best moves\nAdvanced: also thinks about the pieces it keeps\nMaster: best balance between points and the pieces kept")
        }
    }
}
