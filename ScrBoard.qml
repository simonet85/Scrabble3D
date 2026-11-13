import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    anchors.fill: parent

    property int fieldSize
    property int labelSize: Math.round(fieldSize * 0.6) //coordinates frame around the board
    property alias jokerPicker: jokerPicker //alias needed for newgame > updatemodel

    // board frame: 2 label rows/columns, then a gap and the rack below the board
    function updateFieldSize() {
        fieldSize = Math.min(height / (GamePlay.boardSize + 2.6),
                             width / (GamePlay.boardSize + 1.3))
    }

    // short caption written on empty premium squares, indexed by field type
    readonly property var bonusCaptions: ["", "", qsTr("DOUBLE\nLETTER"), qsTr("TRIPLE\nLETTER"),
        qsTr("QUAD\nLETTER"), qsTr("DOUBLE\nWORD"), qsTr("TRIPLE\nWORD"), qsTr("QUAD\nWORD")]
    onWidthChanged: updateFieldSize();
    onHeightChanged: updateFieldSize();

    ScrJokerPicker { id: jokerPicker; visible: false }

    ListModel {
        id: horizLabelModel
        dynamicRoles: true
    }
    ListModel {
        id: vertLabelModel
        dynamicRoles: true
    }
    Component.onCompleted: updateLabelsModel();

    function updateLabelsModel() {
        horizLabelModel.clear()
        vertLabelModel.clear()
        for (var i = 0; i < GamePlay.boardSize; i++)
        {
            horizLabelModel.append({"label":i+1})
            if (i < 26)
                vertLabelModel.append({"label":String.fromCharCode(65 + i)});
            else
                vertLabelModel.append({"label":String.fromCharCode(65 + i % 26) + Math.floor(i / 26)}); //A..Z,A1..Z1,A2..Z2...
        }
    }

    Component {
        id: labelDelegate
        Item {
            implicitWidth: ListView.view.orientation === ListView.Vertical ? ListView.view.width : fieldSize
            implicitHeight: ListView.view.orientation === ListView.Vertical ? fieldSize : ListView.view.height
            Text {
                text: label
                color: "white"
                font.bold: true
                font.pixelSize: labelSize * 0.6
                anchors.centerIn: parent
            }
        }
    }

    Component {
        id: boardDelegate
        ScrPiece {
            id: rcPiece
            property string aWhat: what
            property int aCurrentMove: GamePlay.currentMove

            implicitWidth: fieldSize
            implicitHeight: fieldSize

            pieceColor: aWhat == String.fromCharCode(0) //empty?
                        ? config.colors.get(fieldtype).itemColor //use field color when empty
                        : isPlaced && config.bColoredPlayers && (aCurrentMove - when) <= GamePlay.numberOfPlayers
                          ? Qt.lighter(config.playercolors.get(who).itemColor, 1.75) //use (lighter) player color for the last move, if set in config
                          : when === aCurrentMove
                            ? config.colors.get(8).itemColor //use (yellow) field color for non-empty squares
                            : Qt.lighter(config.colors.get(8).itemColor) // ... but a bit lighter when removing is not possible

            pieceLabel: aWhat
            pieceValue: value
            pieceIsJoker: isJoker
            pieceIsRandom: isRandom
            bonusCaption: bonusCaptions[fieldtype] ?? ""
            pieceIsStart: fieldtype === 0
            // white grid lines between the squares, like a printed board
            border.color: isEmpty ? "#eef5ee" : Qt.darker(color, 1.15)
            border.width: Math.max(1, Math.round(fieldSize * 0.035))
            pieceShadow: (isPlaced && config.bColoredPlayers)
                         ? config.playercolors.get(who).itemColor //colored shadow to show who placed a piece
                         : "transparent"

            bonusTop:    config.bMarkers && (bonustop > 0)    ? config.colors.get(bonustop).itemColor   : "transparent"
            bonusBottom: config.bMarkers && (bonusbottom > 0) ? config.colors.get(bonusbottom).itemColor : "transparent"
            bonusLeft: config.ltr
                ? config.bMarkers && (bonusleft > 0) ? config.colors.get(bonusleft).itemColor  : "transparent"
                : config.bMarkers && (bonusright > 0) ? config.colors.get(bonusright).itemColor : "transparent"
            bonusRight: config.ltr
                ? config.bMarkers && (bonusright > 0) ? config.colors.get(bonusright).itemColor : "transparent"
                : config.bMarkers && (bonusleft > 0) ? config.colors.get(bonusleft).itemColor  : "transparent"

            ToolTip {
                id: tipMeaning
                visible: maBoard.containsMouse && (text !== "<html>\n</html>") && (text !== "")
                delay: 1000
                timeout: 5000

                contentItem: Label {
                    color: config.myPalette.ToolTipText ? config.myPalette.toolTipText : "#000000"
                    text: tipMeaning.text
                }
                background: Rectangle { // required on macOS
                    color: config.myPalette.ToolTipBase ? config.myPalette.ToolTipBase : "#ffffca"
                }
            }

            DropArea {
                anchors.fill: parent
                keys: "scrPiece"
                onEntered: (drag)=> {
                    rcPiece.border.color = Qt.darker(rcPiece.color)
                    drag.source.dragAccept = GamePlay.canDrop(index)
                }
                onExited: {
                    rcPiece.border.color = Qt.lighter(rcPiece.color)
                    drag.source.dragAccept = false
                }
                onDropped: (drag)=> {
                    rcPiece.border.color = "transparent"
                    GamePlay.dropLetter(drag.source.rackIndex, index)
                    if (drag.source.pieceIsJoker) {
                        var p = mapToItem(board, drag.x,drag.y)
                        if (scrabble3D.width > p.x + jokerPicker.width)
                            jokerPicker.x = p.x; else
                            jokerPicker.x = p.x - jokerPicker.width
                        if (scrabble3D.height > p.y + jokerPicker.height)
                            jokerPicker.y = p.y; else
                            jokerPicker.y = p.y - jokerPicker.height
                        jokerPicker.boardIndex = index
                        jokerPicker.open()
                    }
                }
            }
            MouseArea {
                id: maBoard
                property var rackItem
                property int rackIndex
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onEntered: tipMeaning.text = GamePlay.getMeaningAt(index)
                onPressed: (mouse)=> {
                    if (!isPlaced && (aWhat !== String.fromCharCode(0))) {
                        if (mouse.button === Qt.RightButton)
                            GamePlay.removeLetter(index);
                        if (mouse.button === Qt.LeftButton) {
                            rackIndex = GamePlay.removeLetter(index)
                            var p = mapToItem(rackList, mouseX, mouseY)
                            rackItem = rackList.itemAtIndex(rackIndex)
                            rackItem.dragStartedAt = Qt.point(rackItem.x, rackItem.y)
                            rackItem.x = p.x - fieldSize / 2
                            rackItem.y = p.y - fieldSize / 2
                            rackItem.z = Infinity
                            drag.target = rackItem
                            rackItem.Drag.active = true
                         }
                     }
                 }
                onReleased: (mouse)=> {
                    if ((mouse.button === Qt.LeftButton) &&
                        (Drag.source.rackItem !== undefined))
                        rackItem.maRack.released(mouse)
                }
            }
            TapHandler {
                onTapped: tipMeaning.visible = true
            }
        }
    }

    Component {
        id: rackDelegate
        ScrPiece {
            id: rcPiece
            property int rackIndex: index
            property alias maRack: maRack //required to call released() for backAnim

            width: fieldSize
            height: fieldSize
            border.color: Qt.darker(color)
            pieceColor: (!GamePlay.isLocalPlayer) || isExchange
                            ? config.colors.get(0).itemColor //start
                            : config.colors.get(8).itemColor //piece
            visible: isVisible //LetterState::lsRack
            pieceLabel: what
            pieceValue: value
            pieceIsJoker: isJoker
            pieceIsRandom: isRandom
            property bool dragAccept: false
            property point dragStartedAt: Qt.point(0,0)
            Drag.active: maRack.drag.active
            Drag.keys: "scrPiece"
            DropArea {
                keys: "scrPiece"
                anchors.fill: parent
                onEntered: (drag)=> { drag.source.dragAccept = true }
                onExited: drag.source.dragAccept = false
                onDropped: (drag)=> GamePlay.dropLetterRack(drag.source.rackIndex, index)
            }

            MouseArea {
                id: maRack
                anchors.fill: parent
                cursorShape: drag.active ? (dragAccept ? Qt.DragLinkCursor : Qt.ClosedHandCursor) : Qt.ArrowCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onPressed: (mouse) => {
                    if (mouse.button === Qt.RightButton) {
                       rcPiece.dragStartedAt = Qt.point(0,0) //block back animation on release
                       GamePlay.exchangeLetter(index)
                    } else
                    if ((mouse.button === Qt.LeftButton) && GamePlay.canDrag()) {
                       drag.target = parent
                       rcPiece.dragStartedAt = Qt.point(rcPiece.x, rcPiece.y)
                       rcPiece.z = Infinity //topmost z-order
                    }
                }
                onPressAndHold: GamePlay.exchangeLetter(index)
                onReleased: (drag) => {
                    rcPiece.z = rackIndex //original z-order
                    if (rcPiece.dragAccept) {
                        rcPiece.Drag.drop()
                    } else
                    if (dragStartedAt !== Qt.point(0,0)) {
                        backAnimX.from = rcPiece.x;
                        backAnimX.to = dragStartedAt.x;
                        backAnimY.from = rcPiece.y;
                        backAnimY.to = dragStartedAt.y;
                        backAnim.start()
                    }
                }
            }
            ParallelAnimation {
                id: backAnim
                SpringAnimation { id: backAnimX; target: rcPiece; property: "x";
                                  duration: 500; spring: 2; damping: 0.2 }
                SpringAnimation { id: backAnimY; target: rcPiece; property: "y";
                                  duration: 500; spring: 2; damping: 0.2 }
            }
        }
    }

    // dark green frame carrying the coordinates on all four sides
    Rectangle {
        id: boardFrame
        x: 0
        y: 0
        width: gdBoard.width + 2 * labelSize
        height: gdBoard.height + 2 * labelSize
        radius: labelSize / 2
        color: "#0b5d2a"
        border.color: Qt.darker(color, 1.3)
    }
    ListView {
        id: vertLabels
        anchors.top: gdBoard.top
        anchors.left: parent.left
        height: fieldSize * GamePlay.boardSize
        width: labelSize
        model: vertLabelModel
        delegate: labelDelegate
        orientation: ListView.Vertical
        interactive: false
    }
    ListView {
        id: vertLabelsRight
        anchors.top: gdBoard.top
        anchors.left: gdBoard.right
        height: fieldSize * GamePlay.boardSize
        width: labelSize
        model: vertLabelModel
        delegate: labelDelegate
        orientation: ListView.Vertical
        interactive: false
    }
    ListView {
        id: horzLabels
        anchors.top: parent.top
        anchors.left: gdBoard.left
        width: fieldSize * GamePlay.boardSize
        height: labelSize
        model: horizLabelModel
        delegate: labelDelegate
        orientation: ListView.Horizontal
        interactive: false
        layoutDirection: config.ltr ? Qt.LeftToRight : Qt.RightToLeft
    }
    ListView {
        id: horzLabelsBottom
        anchors.top: gdBoard.bottom
        anchors.left: gdBoard.left
        width: fieldSize * GamePlay.boardSize
        height: labelSize
        model: horizLabelModel
        delegate: labelDelegate
        orientation: ListView.Horizontal
        interactive: false
        layoutDirection: config.ltr ? Qt.LeftToRight : Qt.RightToLeft
    }
    GridView {
        id: gdBoard
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: labelSize
        anchors.leftMargin: labelSize
        width: fieldSize*GamePlay.boardSize
        height: fieldSize*GamePlay.boardSize
        cellHeight: fieldSize
        cellWidth: fieldSize
        model: GamePlay.boardModel
        delegate: boardDelegate
        interactive: false
        layoutDirection: config.ltr ? Qt.LeftToRight : Qt.RightToLeft
    }
    ListView {
        id: rackList
        anchors.left: parent.left
        anchors.leftMargin: labelSize
        anchors.top: boardFrame.bottom
        anchors.topMargin: fieldSize * 0.25
        width: gdBoard.width
        height: fieldSize //any height works; none makes pieces invisible
        orientation: Qt.Horizontal
        spacing: 1
        model: GamePlay.rackModel
        delegate: rackDelegate
    }

    // player actions next to the rack: shuffle, recall, pass + live score of the move
    RowLayout {
        id: rackActions
        anchors.left: rackList.left
        anchors.leftMargin: rackList.contentWidth + fieldSize * 0.5
        anchors.right: gdBoard.right
        anchors.verticalCenter: rackList.verticalCenter
        spacing: 4
        visible: GamePlay.isRunning

        component RackButton: ToolButton {
            id: rackButton
            required property Action act
            action: act
            display: AbstractButton.IconOnly
            icon.width: fieldSize * 0.6
            icon.height: fieldSize * 0.6
            implicitWidth: fieldSize
            implicitHeight: fieldSize
            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: act.text + (act.shortcut ? "  (" + act.shortcut + ")" : "")
        }
        RackButton { act: acShuffleRack }
        RackButton { act: acRecallPieces }
        Button {
            action: acPassMove
            Layout.preferredHeight: fieldSize
            highlighted: GamePlay.exchangeCount > 0
            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: GamePlay.exchangeCount > 0
                          ? qsTr("End the turn by exchanging the marked tiles")
                          : GamePlay.canExchange
                            ? qsTr("Pass the move. To exchange tiles instead, right-click them on the rack.")
                            : qsTr("Pass the move. Exchanging needs at least 7 tiles in the bag.")
        }
        Label {
            id: lbPlacedScore
            Layout.fillWidth: true
            elide: Text.ElideRight
            leftPadding: 8
            visible: GamePlay.isLocalPlayer && (GamePlay.placedWords !== "" || !GamePlay.isAccepted)
            font.pixelSize: fieldSize * 0.4
            font.bold: true
            color: !GamePlay.isAccepted ? "#c00000"
                                        : GamePlay.placedValid ? "#007000" : "#c06000"
            text: !GamePlay.isAccepted
                  ? "✗"
                  : (GamePlay.placedValid ? "✓ " : "? ") + GamePlay.placedWords
                    + "  " + qsTr("%1 pts").arg(GamePlay.placedValue)
            ToolTip.visible: maPlacedScore.containsMouse
            ToolTip.text: !GamePlay.isAccepted
                          ? GamePlay.lastError
                          : GamePlay.placedValid ? qsTr("All words found in the dictionary")
                                                 : qsTr("Word(s) not found in the dictionary")
            MouseArea { id: maPlacedScore; anchors.fill: parent; hoverEnabled: true }
        }
    }
}
