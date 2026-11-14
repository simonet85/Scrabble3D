#include "computelevel.h"

#include <QHash>
#include <algorithm>
#include <cmath>

namespace computelevel {

double leaveValue(const QList<KeptPiece> &kept)
{
    if (kept.isEmpty())
        return 0;

    static const QString vowels = QStringLiteral("AEIOU");
    QHash<QString, int> count;
    double value = 0;
    int nVowels = 0;

    for (const KeptPiece &piece : kept) {
        if (piece.isJoker) {
            value += 25;
            continue;
        }
        const QString letter = piece.what.toUpper();
        count[letter]++;
        if (vowels.contains(letter))
            nVowels++;
        // flexible letters are good to keep, awkward ones are not (French distribution)
        if (letter == "S")
            value += (count[letter] == 1) ? 7 : 1;
        else if (letter == "E" || letter == "A" || letter == "R" || letter == "T" || letter == "N" || letter == "I")
            value += 1;
        else if (letter == "Q")
            value -= 6;
        else if (letter == "W" || letter == "K" || letter == "Y")
            value -= 3;
        else if (letter == "V" || letter == "J")
            value -= 2;
    }
    // the Q is much less harmful with a U
    if (count.value("Q") > 0 && count.value("U") > 0)
        value += 4;

    // duplicates
    for (auto it = count.cbegin(); it != count.cend(); ++it)
        if (it.value() > 1)
            value -= 4.0 * (it.value() - 1);

    // vowel / consonant balance: about 40 % vowels is comfortable
    const int nPieces = int(kept.size());
    const int nJokers = int(std::count_if(kept.cbegin(), kept.cend(), [](const KeptPiece &p) { return p.isJoker; }));
    const int nLetters = nPieces - nJokers;
    if (nLetters >= 2) {
        const double ideal = nLetters * 0.4;
        value -= 2.5 * std::fabs(nVowels - ideal);
    }
    return value;
}

double equity(int score, const QList<KeptPiece> &kept, bool bagIsEmpty)
{
    if (!bagIsEmpty)
        return score + leaveValue(kept);

    // end game: no more draws, what stays on the rack will be subtracted
    int remaining = 0;
    for (const KeptPiece &piece : kept)
        remaining += piece.value;
    return score - remaining + (kept.isEmpty() ? 10 : 0);
}

static bool reachable(const BoardView &v, int x, int y)
{
    static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto &d : dirs)
        for (int step = 1; step <= OpeningReach; step++) {
            const int nx = x + d[0] * step, ny = y + d[1] * step;
            if (nx < 0 || ny < 0 || nx >= v.size || ny >= v.size)
                break;
            if (v.occupied[ny * v.size + nx])
                return true;
        }
    return false;
}

double openingPenalty(const BoardView &before, const QList<QPoint> &placed)
{
    const int n = before.size * before.size;
    if (placed.isEmpty() || before.fieldType.size() < n || before.occupied.size() < n)
        return 0;
    BoardView after = before;
    for (const QPoint &p : placed)
        if (p.x() >= 0 && p.y() >= 0 && p.x() < before.size && p.y() < before.size)
            after.occupied[p.y() * before.size + p.x()] = true;

    double penalty = 0;
    for (int y = 0; y < before.size; y++)
        for (int x = 0; x < before.size; x++) {
            const int i = y * before.size + x;
            const int type = before.fieldType[i];
            double worth = 0;
            if (type == 6) worth = TripleWordOpening;
            else if (type == 5) worth = DoubleWordOpening;
            else if (type == 7) worth = QuadWordOpening;
            if (worth == 0 || after.occupied[i])
                continue; // not a premium word square, or covered by the move itself
            if (reachable(after, x, y) && !reachable(before, x, y))
                penalty += worth;
        }
    return penalty;
}

int pickMove(Level level, const QVector<int> &scores, const QVector<double> &equities, QRandomGenerator &rng)
{
    const int n = int(scores.size());
    if (n == 0)
        return -1;
    if (n == 1)
        return 0;

    switch (level) {
    case Beginner: {
        // a move from the middle of the list: neither the best ones nor the very worst
        int lo = n * 40 / 100;
        int hi = n * 80 / 100;
        if (hi < lo)
            hi = lo;
        if (hi >= n)
            hi = n - 1;
        return rng.bounded(lo, hi + 1);
    }
    case Intermediate: {
        // one of the best 15 percent (at most 40 moves) without any further thought
        const int top = std::clamp(n * 15 / 100, 1, 40);
        return rng.bounded(0, top);
    }
    case Advanced: {
        const int top = std::min(n, 5);
        int best = 0;
        for (int i = 1; i < top; i++)
            if (equities[i] > equities[best])
                best = i;
        return best;
    }
    case Master:
    default: {
        // highest equity among the moves that are at most 30 points below the best score
        int best = 0;
        for (int i = 1; i < n; i++) {
            if (scores[i] < scores[0] - 30)
                break; // sorted: the next ones are worse
            if (equities[i] > equities[best])
                best = i;
        }
        return best;
    }
    }
}

} // namespace computelevel
