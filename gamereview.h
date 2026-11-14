#pragma once

#include <QList>
#include <QString>

// Review of a finished game: for each move of a human player, the points scored are compared with the best
// move that was possible at that turn (computed by the engine at the start of the turn).
namespace gamereview {

struct MoveRecord {
    int number = 0;        // turn number, starting at 1
    QString player;
    QString playedWord;    // empty for a pass, an exchange or a move refused by a challenge
    int playedValue = 0;
    int bestValue = 0;     // best score possible at that turn; 0 = unknown
    QString bestWord;
};

struct PlayerSummary {
    QString player;
    int moves = 0;          // moves of the player
    int analysed = 0;       // moves whose best score is known
    int points = 0;         // points of the analysed moves
    int bestPoints = 0;     // best points possible on the same moves (never below the points scored)
    int perfect = 0;        // analysed moves at least as good as the best one
    QList<MoveRecord> worst; // biggest losses, the worst first (only moves that lost points)

    int lost() const { return bestPoints - points; }
    // percentage of the best possible points; 100 when nothing could be analysed
    double efficiency() const { return bestPoints > 0 ? 100.0 * points / bestPoints : 100.0; }
};

// One summary per player, in order of first appearance
QList<PlayerSummary> summarize(const QList<MoveRecord> &moves, int worstCount = 3);

// Text for the Coach tab (HTML, translated)
QString toHtml(const PlayerSummary &summary);

// Facts given to the coach for its comment (French, plain text)
QString toCoachFacts(const PlayerSummary &summary);

} // namespace gamereview
