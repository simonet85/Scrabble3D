#pragma once

#include <QList>
#include <QPoint>
#include <QRandomGenerator>
#include <QString>
#include <QVector>

// Strength of the computer player. Pure functions (no dependency on the game state) so that
// they can be tested on their own.
namespace computelevel {

enum Level {
    Beginner = 0,     // plays ordinary, rather weak moves
    Intermediate = 1, // plays one of the better moves, without caring about the pieces kept
    Advanced = 2,     // looks at the pieces it keeps on the rack among the five best moves
    Master = 3        // best equity (points + value of the kept pieces) among the strongest moves
};

struct KeptPiece {
    QString what;   // letter, upper case
    bool isJoker = false;
    int value = 0;  // points of the piece
};

// Heuristic value of the pieces kept on the rack after a move ("leave"), in points:
// a blank and an S are worth keeping, duplicates and a vowel/consonant imbalance are not.
double leaveValue(const QList<KeptPiece> &kept);

// Equity of a move: points plus the value of what is kept. When the bag is empty the kept
// pieces will cost their points at the end of the game and there is a bonus for going out.
double equity(int score, const QList<KeptPiece> &kept, bool bagIsEmpty);

// Defense: a 2D board seen by the computer (row-major, index = y * size + x)
struct BoardView {
    int size = 15;
    QVector<int> fieldType;   // board.h FieldType of each square (5 = double word, 6 = triple word, 7 = quad word)
    QVector<bool> occupied;   // a tile lies on the square
};

constexpr int OpeningReach = 4;            // a premium square is reachable from a tile at most this far on its row or column
constexpr double TripleWordOpening = 10.0; // points given away by opening a "triple word" square to the opponent
constexpr double DoubleWordOpening = 3.0;
constexpr double QuadWordOpening = 14.0;

// Points the move gives away: premium word squares that are still free and become reachable for the
// opponent because of the tiles placed (and were not reachable before)
double openingPenalty(const BoardView &before, const QList<QPoint> &placed);

// Chooses a move among candidates sorted by descending score; returns its index or -1.
// equities[i] belongs to scores[i].
int pickMove(Level level, const QVector<int> &scores, const QVector<double> &equities, QRandomGenerator &rng);

} // namespace computelevel
