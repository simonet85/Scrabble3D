#include "gameplay.h"
#include "coach.h"
#include "configpath.h"

#include <QMessageBox>

// The coach advises; the dictionary decides. This is said to the model and repeated in the code
// below: nothing the coach answers is ever used to accept or refuse a word.
static const char *coachSystem =
    "Tu es un coach de Scrabble classique francophone. Tu t'adresses a un joueur, en francais, "
    "avec un ton simple et amical, en 8 lignes maximum. Tu conseilles et tu expliques ; tu n'es pas "
    "l'arbitre : tu ne declares jamais qu'un mot est valide ou invalide au Scrabble, le dictionnaire "
    "du jeu en decide. Si tu n'es pas sur d'une information, dis-le.";

coach *GamePlay::coachClient()
{
    if (!m_pCoach) {
        m_pCoach = new coach(config::file("coach.ini"), this);
        QObject::connect(m_pCoach, &coach::answered, this, [this](const QString &text) {
            m_bCoachBusy = false;
            QString html = text.toHtmlEscaped(); // the message list shows rich text
            html.replace("\n", "<br>");
            m_pMsgModel->addMessage("<b>" + tr("Coach") + ":</b> " + html);
        });
        QObject::connect(m_pCoach, &coach::failed, this, [this](const QString &message) {
            m_bCoachBusy = false;
            m_pMsgModel->addMessage(tr("Coach: %1").arg(message));
        });
    }
    return m_pCoach;
}

bool GamePlay::coachHasKey() { return coachClient()->hasKey(); }
QString GamePlay::coachMaskedKey() { return coachClient()->maskedKey(); }
QString GamePlay::coachModel() { return coachClient()->model(); }

void GamePlay::coachSave(const QString key, const QString model)
{
    if (!key.trimmed().isEmpty())
        coachClient()->setKey(key);
    coachClient()->setModel(model.trimmed().isEmpty() ? QString::fromLatin1(coach::DefaultModel) : model);
}

void GamePlay::coachClearKey()
{
    coachClient()->setKey(QString());
    coachClient()->setConsent(false);
}

bool GamePlay::coachReady()
{
    if (m_bCoachBusy) {
        addMessage(tr("The coach is still thinking..."));
        return false;
    }
    if (!coachClient()->hasKey()) {
        addMessage(tr("No API key: enter it in Configuration > AI coach."));
        return false;
    }
    if (!coachClient()->hasConsent()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Scrabble3D");
        msgBox.setText(tr("The coach sends data of this game to an online AI service (Anthropic): the board, "
                          "your rack and the best moves found. Nothing else is sent."));
        msgBox.setInformativeText(tr("The service is paid per use with your API key. Do you want to continue?"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
        coachClient()->setConsent(true);
    }
    return true;
}

QString GamePlay::coachBoardText()
{
    const int size = m_pBoard->getBoardSize();
    QString text;
    for (int y = 0; y < size; y++) {
        text += QChar('A' + y);
        text += QLatin1Char(' ');
        for (int x = 0; x < size; x++) {
            const Letter l = m_pBoard->getLetter(m_pBoard->pointToWhere(Point3D(x, y, 0)));
            if ((l.State == LetterState::lsPlaced) || (l.State == LetterState::lsBoard))
                text += l.IsJoker ? l.What.toLower() : l.What;
            else
                text += QLatin1Char('.');
            text += QLatin1Char(' ');
        }
        text += QLatin1Char('\n');
    }
    return text;
}

void GamePlay::coachExplain()
{
    if (!m_bIsRunning || m_bIsConnected || m_pBoard->is3D()) {
        addMessage(tr("The coach is available in a running 2D game that is not played on the network."));
        return;
    }
    if (!coachReady())
        return;

    computeMove(); // finds the moves of the engine for the current rack
    if (m_nComputeResults == 0)
        return; // computeMove() has already said that no word was found

    QString rack;
    for (int i = 0; i < m_pRackModel->rackSize(); i++) {
        const Letter l = m_pRackModel->getLetter(i);
        rack += l.IsJoker ? QStringLiteral("?") : l.What;
    }
    QString scores;
    for (uint p = 0; p < m_nPlayerCount; p++)
        scores += m_lPlayerNames[p] + " " + QString::number(m_pGameCourseModel->getScore(p)) + "  ";

    QString moves;
    const int top = qMin(5, m_nComputeResults);
    for (int i = 0; i < top; i++) {
        const sharedMove aMove = m_pComputeMove->result(i);
        const Letter first = aMove->getLetter(0);
        moves += QString("%1. %2%3 - %4 points - first tile on %5%6, %7\n")
                     .arg(i + 1)
                     .arg(aMove->PlacedWord,
                          aMove->ConnectedWords.isEmpty() ? QString() : " (+ " + aMove->ConnectedWords + ")")
                     .arg(aMove->Value())
                     .arg(QChar('A' + first.Point.y()))
                     .arg(first.Point.x() + 1)
                     .arg(aMove->activeDimension() == dmOrdinate ? QStringLiteral("vertical") : QStringLiteral("horizontal"));
    }

    const QString user = QString(
        "Plateau classique 15x15 (colonnes 1 a 15 de gauche a droite, lignes A a O de haut en bas ; "
        "'.' = case vide, minuscule = joker) :\n%1\n"
        "Chevalet du joueur ('?' = joker) : %2\n"
        "Lettres restantes dans le sac : %3\n"
        "Scores : %4\n\n"
        "Meilleurs coups trouves par le moteur :\n%5\n"
        "Explique en quelques lignes quel coup tu conseilles et pourquoi (points, lettres gardees, "
        "cases primes offertes a l'adversaire).")
        .arg(coachBoardText(), rack)
        .arg(m_lBag.count())
        .arg(scores, moves);

    m_bCoachBusy = true;
    addMessage(tr("The coach is thinking..."));
    coachClient()->ask(QString::fromUtf8(coachSystem), user, 800);
}

void GamePlay::coachDefine()
{
    if (!m_bIsRunning || m_bIsConnected) {
        addMessage(tr("The coach is available in a running game that is not played on the network."));
        return;
    }
    QStringList words;
    if (m_pMoves.count() > 1) {
        const sharedMove last = m_pMoves[m_pMoves.count() - 2];
        if (!last->PlacedWord.isEmpty())
            words << last->PlacedWord;
        for (const QString &w : last->ConnectedWords.split(",", Qt::SkipEmptyParts))
            if (!w.trimmed().isEmpty())
                words << w.trimmed();
    }
    if (words.isEmpty()) {
        addMessage(tr("No word has been played yet."));
        return;
    }
    if (!coachReady())
        return;

    const QString user = QString(
        "Mots joues au dernier coup : %1.\n"
        "Pour chaque mot, donne une definition courte (une phrase) et sa nature (nom, verbe conjugue ...). "
        "Si tu ne connais pas un mot ou si tu n'es pas sur qu'il existe, dis-le franchement. "
        "Ne dis pas s'il est valide au Scrabble.").arg(words.join(", "));

    m_bCoachBusy = true;
    addMessage(tr("The coach is thinking..."));
    coachClient()->ask(QString::fromUtf8(coachSystem), user, 600);
}
