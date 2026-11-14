#include "gameplay.h"
#include "coach.h"
#include "computelevel.h"

int GamePlay::hintsPerGame() { return qBound(0, coachClient()->number("hintsPerGame", 3), 20); }

void GamePlay::setHintsPerGame(int count)
{
    if (hintsPerGame() == qBound(0, count, 20))
        return;
    coachClient()->setNumber("hintsPerGame", qBound(0, count, 20));
    emit hintChanged();
}

int GamePlay::hintsLeft() { return qMax(0, hintsPerGame() - m_nHintsUsed); }

// a hint is for a human in a running local 2D game, with the engine free and hints left
bool GamePlay::hintAvailable()
{
    return m_bIsRunning && !m_bIsConnected && !m_bIsHistory && !m_bIsComputing && !m_pBoard->is3D()
           && !m_lPlayerNames.isEmpty() && (m_lPlayerNames[m_nCurrentPlayer] != "Computer") && (hintsLeft() > 0);
}

void GamePlay::hint()
{
    if (!m_bIsRunning || m_bIsConnected || m_bIsHistory || m_pBoard->is3D()) {
        addMessage(tr("Hints are available in a running 2D game that is not played on the network."));
        return;
    }
    if (m_lPlayerNames[m_nCurrentPlayer] == "Computer")
        return;
    if (m_bIsComputing) {
        addMessage(tr("The computer is busy, try again in a moment."));
        return;
    }
    const bool sameTurn = (m_nHintTurn == m_nCurrentMove);
    if (sameTurn && m_nHintLevel >= 2) {
        addMessage(tr("The word of the hint is already shown on the board."));
        return; // nothing more to show: no hint is used
    }
    if (hintsLeft() <= 0) {
        addMessage(tr("No hint left in this game."));
        return;
    }

    HintMove move;
    if (sameTurn && m_nHintLevel == 1) {
        // the second hint of a turn shows the move of the first one: it is not computed again, because several
        // moves can have the same score and a new computation could choose another one
        move = m_hintMove;
    } else {
        // the moves are computed without showing anything and are not kept: only what the hint says is revealed
        resetPieces();
        m_bIsComputing = true; emit isComputingChanged();
        m_pComputeMove->run(m_bIsFirstMove, m_pBoard, m_pRackModel, m_pDictionary);
        m_bIsComputing = false; emit isComputingChanged();
        recordTurnBest(); // the review needs the best move of the turn anyway
        if (m_nComputeResults <= 0) {
            m_pComputeMove->clear();
            setComputeResults(0);
            addMessage(tr("No move was found with this rack: you can exchange tiles or pass. (No hint used.)"));
            return;
        }
        const int index = chooseComputerMove(computelevel::Master);
        const sharedMove aMove = m_pComputeMove->result(index < 0 ? 0 : index);
        const Letter first = aMove->getLetter(0);
        move.word = aMove->PlacedWord
                    + (aMove->ConnectedWords.isEmpty() ? QString() : QStringLiteral(" (+ ") + aMove->ConnectedWords + QStringLiteral(")"));
        move.value = int(aMove->Value());
        move.tiles = aMove->letterCount();
        move.vertical = (aMove->activeDimension() == dmOrdinate);
        move.where = QString(QChar('A' + first.Point.y())) + QString::number(first.Point.x() + 1);
        for (int j = 0; j < aMove->letterCount(); j++)
            move.squares.append(aMove->getLetter(j).Point);
        m_pComputeMove->clear();
        setComputeResults(0);
        m_hintMove = move;
    }

    const int level = sameTurn ? m_nHintLevel + 1 : 1;
    const QString direction = move.vertical ? tr("vertically") : tr("horizontally");
    const int number = m_nHintsUsed + 1;
    QString text;
    if (level == 1)
        text = tr("Hint %1/%2: a move worth %3 points exists. It uses %4 tiles of your rack, its first tile goes on %5, %6.")
                   .arg(number).arg(hintsPerGame()).arg(move.value).arg(move.tiles).arg(move.where, direction);
    else
        text = tr("Hint %1/%2: play %3 for %4 points, first tile on %5, %6.")
                   .arg(number).arg(hintsPerGame()).arg(move.word).arg(move.value).arg(move.where, direction);

    // the first hint frames the first tile only, the second frames every tile of the move
    m_pBoardModel->setHint(level == 1 ? QList<Point3D>{move.squares.first()} : move.squares);
    m_nHintsUsed++;
    m_nHintTurn = m_nCurrentMove;
    m_nHintLevel = level;
    emit hintChanged();
    addMessage(text);
}
