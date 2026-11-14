#pragma once

#include <QList>
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

// Chooses a move among candidates sorted by descending score; returns its index or -1.
// equities[i] belongs to scores[i].
int pickMove(Level level, const QVector<int> &scores, const QVector<double> &equities, QRandomGenerator &rng);

} // namespace computelevel
