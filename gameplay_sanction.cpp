#include "gameplay.h"


// FISF 6.4: graded sanctions. 1st infraction: reminder of the rules; 1st repeat: 5 points; 2nd repeat: 50 points;
// 3rd repeat: lost game; 4th repeat: exclusion from the tournament (here the game is lost as well)
static int sanctionPoints(int level) { return level == 2 ? 5 : (level == 3 ? 50 : 0); }

QString GamePlay::sanctionText(int level) const
{
    switch (level) {
    case 1: return tr("reminder of the rules");
    case 2: return tr("penalty of 5 points");
    case 3: return tr("penalty of 50 points");
    case 4: return tr("game lost");
    default: return tr("exclusion from the tournament");
    }
}

QString GamePlay::nextSanction(int player)
{
    if ((player < 0) || (player >= m_lPlayerNames.count()))
        return QString();
    return sanctionText(m_nInfractions.value(player, 0) + 1);
}

bool GamePlay::canSanction()
{
    return m_bIsRunning && !m_bIsConnected && !m_bIsHistory && (m_nPlayerCount >= 2);
}

void GamePlay::sanction(int player, const QString &reason)
{
    if (!canSanction() || (player < 0) || (player >= int(m_nPlayerCount)))
        return;
    while (m_nInfractions.count() < int(m_nPlayerCount))
        m_nInfractions.append(0);
    const int level = ++m_nInfractions[player];
    const int points = sanctionPoints(level);

    SanctionEntry entry;
    entry.move = m_nCurrentMove + 1;
    entry.player = player;
    entry.reason = reason.trimmed();
    entry.level = level;
    entry.points = points;
    m_lSanctions.append(entry);

    const QString why = entry.reason.isEmpty() ? QString() : QStringLiteral(" (") + entry.reason + QStringLiteral(")");
    m_pMsgModel->addMessage(tr("Sanction for %1%2: %3.").arg(m_lPlayerNames[player], why, sanctionText(level)));

    if (points > 0) {
        m_pGameCourseModel->addBonus(player, -points);
        emit statInfoChanged();
        emit statTotalChanged();
    }
    if (level >= 4) {
        // the game is lost: the score of the player is set to 0 and the game ends
        m_pGameCourseModel->zerototal(uint(player));
        QStringList others;
        for (int p = 0; p < int(m_nPlayerCount); p++)
            if (p != player) others.append(m_lPlayerNames[p]);
        m_pMsgModel->addMessage(tr("%1 loses the game, %2 wins.").arg(m_lPlayerNames[player], others.join(", ")));
        if (m_nTimerID != -1) {
            killTimer(m_nTimerID);
            m_nTimerID = -1;
        }
        if (!matchSheet().isEmpty())
            m_pMsgModel->addMessage(matchSheet());
        if (!jokerSheet().isEmpty())
            m_pMsgModel->addMessage(jokerSheet());
        m_bIsRunning = false;
        emit isRunningChanged();
        emit statInfoChanged();
        emit statTotalChanged();
        emit reviewChanged();
        m_pBoardModel->clearHint();
        emit hintChanged();
    }
    emit sanctionsChanged();
}
