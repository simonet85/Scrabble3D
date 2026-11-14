#include "gamereview.h"

#include <QObject>
#include <algorithm>

namespace gamereview {

QList<PlayerSummary> summarize(const QList<MoveRecord> &moves, int worstCount)
{
    QList<PlayerSummary> result;
    for (const MoveRecord &m : moves) {
        int index = -1;
        for (int i = 0; i < result.size(); i++)
            if (result[i].player == m.player)
                index = i;
        if (index < 0) {
            PlayerSummary s;
            s.player = m.player;
            result.append(s);
            index = int(result.size()) - 1;
        }
        PlayerSummary &s = result[index];
        s.moves++;
        if (m.bestValue <= 0)
            continue; // the best score of this turn is not known
        s.analysed++;
        s.points += m.playedValue;
        s.bestPoints += qMax(m.bestValue, m.playedValue);
        if (m.playedValue >= m.bestValue)
            s.perfect++;
        else
            s.worst.append(m);
    }
    for (PlayerSummary &s : result) {
        std::stable_sort(s.worst.begin(), s.worst.end(), [](const MoveRecord &a, const MoveRecord &b) {
            return (a.bestValue - a.playedValue) > (b.bestValue - b.playedValue);
        });
        while (s.worst.size() > worstCount)
            s.worst.removeLast();
    }
    return result;
}

QString toHtml(const PlayerSummary &s)
{
    QString html = QStringLiteral("<b>") + QObject::tr("Game review - %1").arg(s.player.toHtmlEscaped()) + QStringLiteral("</b><br>");
    html += QObject::tr("%1 points scored; the best possible at each turn would have given %2 points: %3 % of the best.")
                .arg(s.points).arg(s.bestPoints).arg(qRound(s.efficiency()));
    html += QStringLiteral("<br>")
            + QObject::tr("%1 of %2 moves were as good as the best move.").arg(s.perfect).arg(s.analysed);
    if (s.worst.isEmpty()) {
        html += QStringLiteral("<br>") + QObject::tr("No points lost: every move was as good as the best one.");
    } else {
        html += QStringLiteral("<br>") + QObject::tr("Biggest losses:");
        for (const MoveRecord &m : s.worst) {
            const QString played = m.playedWord.isEmpty() ? QObject::tr("no word") : m.playedWord.toHtmlEscaped();
            html += QStringLiteral("<br>") + QObject::tr("Turn %1: %2 (%3 points) instead of %4 (%5 points), %6 points lost.")
                        .arg(m.number).arg(played).arg(m.playedValue)
                        .arg(m.bestWord.isEmpty() ? QStringLiteral("?") : m.bestWord.toHtmlEscaped())
                        .arg(m.bestValue).arg(m.bestValue - m.playedValue);
        }
    }
    return html;
}

QString toCoachFacts(const PlayerSummary &s)
{
    QString text = QStringLiteral("Joueur %1 : %2 coups analyses, %3 points marques sur %4 possibles (%5 %), %6 coups aussi bons que le meilleur.")
                       .arg(s.player).arg(s.analysed).arg(s.points).arg(s.bestPoints).arg(qRound(s.efficiency())).arg(s.perfect);
    if (s.worst.isEmpty()) {
        text += QStringLiteral(" Aucun point perdu.");
    } else {
        text += QStringLiteral(" Plus grosses pertes :");
        for (const MoveRecord &m : s.worst)
            text += QStringLiteral(" tour %1 : joue %2 (%3 points) au lieu de %4 (%5 points), %6 points perdus ;")
                        .arg(m.number)
                        .arg(m.playedWord.isEmpty() ? QStringLiteral("aucun mot") : m.playedWord)
                        .arg(m.playedValue)
                        .arg(m.bestWord.isEmpty() ? QStringLiteral("?") : m.bestWord)
                        .arg(m.bestValue)
                        .arg(m.bestValue - m.playedValue);
    }
    return text;
}

} // namespace gamereview
