#include "gameplay.h"
#include "coach.h"
#include "configpath.h"

#include <QMessageBox>
#include <QTimer>

// The coach advises; the dictionary decides. This is said to the model and repeated in the code
// below: nothing the coach answers is ever used to accept or refuse a word.
static const char *coachSystem =
    "Tu es un coach de Scrabble classique francophone. Reponds en francais, ton simple et amical, "
    "6 lignes maximum, en texte brut sans mise en forme (ni gras, ni listes a puces, ni tableaux).\n"
    "Regles strictes :\n"
    "- Les scores des coups sont fournis par le moteur du jeu : reprends-les tels quels, ne refais aucun calcul de points.\n"
    "- Ne parle jamais des cases primes (lettre ou mot compte double ou triple) : tu ne les connais pas ; "
    "parle seulement des points fournis, des lettres gardees et de l'ouverture du plateau.\n"
    "- Tu n'inventes rien : si une information n'est pas dans les donnees, ne l'affirme pas.\n"
    "- Tu n'es pas l'arbitre : ne dis jamais qu'un mot est valide ou invalide au Scrabble, "
    "le dictionnaire du jeu en decide.";

coach *GamePlay::coachClient()
{
    if (!m_pCoach) {
        m_pCoach = new coach(config::file("coach.ini"), this);
        QObject::connect(m_pCoach, &coach::answered, this, [this](const QString &text) {
            m_bCoachBusy = false;
            const QString html = coach::formatAnswer(text);
            m_pCoachModel->setDone(m_nCoachRow, html, int(m_CoachTimer.elapsed()));
            m_pMsgModel->addMessage("<b>" + tr("Coach") + ":</b> " + html); // rich text
        });
        QObject::connect(m_pCoach, &coach::failed, this, [this](const QString &message) {
            m_bCoachBusy = false;
            m_pCoachModel->setFailed(m_nCoachRow, message, int(m_CoachTimer.elapsed()));
            m_pMsgModel->addMessage(tr("Coach: %1").arg(message));
        });
    }
    return m_pCoach;
}

bool GamePlay::coachHasKey() { return coachClient()->hasKey(); }
QString GamePlay::coachMaskedKey() { return coachClient()->maskedKey(); }
QString GamePlay::coachModelName() { return coachClient()->model(); }
QString GamePlay::coachProvider() { return coachClient()->provider(); }

QVariantMap GamePlay::coachInfo(const QString provider)
{
    QVariantMap info;
    if (!coach::isKnownProvider(provider))
        return info;
    info["name"] = coach::providerName(provider);
    info["hasKey"] = coachClient()->hasKey(provider);
    info["masked"] = coachClient()->maskedKey(provider);
    info["model"] = coachClient()->model(provider);
    info["defaultModel"] = coach::defaultModel(provider);
    return info;
}

void GamePlay::coachSave(const QString provider, const QString key, const QString model)
{
    if (!coach::isKnownProvider(provider))
        return;
    coachClient()->setProvider(provider);
    if (!key.trimmed().isEmpty())
        coachClient()->setKey(key, provider);
    coachClient()->setModel(model.trimmed().isEmpty() ? coach::defaultModel(provider) : model, provider);
    emit coachServiceChanged();
}

void GamePlay::coachClearKey(const QString provider)
{
    if (!coach::isKnownProvider(provider))
        return;
    const QString current = coachClient()->provider();
    coachClient()->setProvider(provider);
    coachClient()->setKey(QString(), provider);
    coachClient()->setConsent(false);
    coachClient()->setProvider(current);
    emit coachServiceChanged();
}

QString GamePlay::coachServiceLabel()
{
    return coach::providerName(coachClient()->provider()) + QStringLiteral(" - ") + coachClient()->model();
}

bool GamePlay::coachAutoAdvice() { return coachClient()->autoAdvice(); }

void GamePlay::setCoachAutoAdvice(bool on)
{
    if (coachClient()->autoAdvice() == on)
        return;
    coachClient()->setAutoAdvice(on);
    emit coachAutoAdviceChanged();
    if (on)
        scheduleCoachAdvice(); // advice right away when it is the turn of a human
}

void GamePlay::coachClearHistory() { m_pCoachModel->clear(); }

bool GamePlay::coachReady(const bool silent)
{
    if (m_bCoachBusy) {
        if (!silent)
            addMessage(tr("The coach is still thinking..."));
        return false;
    }
    if (!coachClient()->hasKey()) {
        if (!silent)
            addMessage(tr("No API key: enter it in Configuration > AI coach."));
        return false;
    }
    if (!coachClient()->hasConsent()) {
        if (silent)
            return false; // the automatic advice never asks: the first request is made by hand
        QMessageBox msgBox;
        msgBox.setWindowTitle("Scrabble3D");
        msgBox.setText(tr("The coach sends data of this game to an online AI service (%1): the board, "
                          "your rack and the best moves found. Nothing else is sent.")
                           .arg(coach::providerName(coachClient()->provider())));
        msgBox.setInformativeText(tr("The service is used with your API key and may be charged. Do you want to continue?"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes)
            return false;
        coachClient()->setConsent(true);
    }
    return true;
}

void GamePlay::coachBegin(const QString &kind, const QString &question)
{
    m_nCoachRow = m_pCoachModel->addPending(coach::providerName(coachClient()->provider()),
                                            coachClient()->model(), kind, question);
    m_CoachTimer.start();
    m_bCoachBusy = true;
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

void GamePlay::coachExplain() { coachExplainImpl(false); }

// automatic: asked by the game itself at the start of the turn of a human; it stays silent when
// something is missing (key, agreement) instead of bothering the player
void GamePlay::coachExplainImpl(const bool automatic)
{
    if (!m_bIsRunning || m_bIsConnected || m_pBoard->is3D()) {
        if (!automatic)
            addMessage(tr("The coach is available in a running 2D game that is not played on the network."));
        return;
    }
    if (!coachReady(automatic))
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
        + QString(m_bIsFirstMove ? "(coup d'ouverture : le plateau est vide)\n" : "") +
        "Chevalet du joueur ('?' = joker) : %2\n"
        "Lettres restantes dans le sac : %3\n"
        "Scores : %4\n\n"
        "Meilleurs coups trouves par le moteur :\n%5\n"
        "Explique en quelques lignes quel coup tu conseilles et pourquoi (points du moteur, lettres gardees).")
        .arg(coachBoardText(), rack)
        .arg(m_lBag.count())
        .arg(scores, moves);

    coachBegin(automatic ? "auto" : "advice",
               tr("Rack %1 - %2 possible moves, the best one: %3 (%4 points)")
                   .arg(rack)
                   .arg(m_nComputeResults)
                   .arg(m_pComputeMove->result(0)->PlacedWord)
                   .arg(m_pComputeMove->result(0)->Value()));
    if (!automatic)
        addMessage(tr("The coach is thinking..."));
    coachClient()->ask(QString::fromUtf8(coachSystem), user, 800, 0.3);
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
    if (!coachReady(false))
        return;

    const QString user = QString(
        "Mots joues au dernier coup : %1.\n"
        "Pour chaque mot, donne en une phrase son sens dans la langue francaise courante et sa nature "
        "(nom, verbe, interjection...). Pour un nom propre, une marque, un sigle ou un mot que tu ne connais "
        "pas avec certitude, reponds seulement 'je ne connais pas ce mot' : n'invente jamais un sens ni une "
        "origine. Reste neutre et sobre. Ne dis pas s'il est valide au Scrabble.").arg(words.join(", "));

    coachBegin("define", tr("Definitions of: %1").arg(words.join(", ")));
    addMessage(tr("The coach is thinking..."));
    coachClient()->ask(QString::fromUtf8(coachSystem), user, 600, 0.2);
}

// called when a turn starts: advice for a human player when the automatic mode is on
void GamePlay::scheduleCoachAdvice()
{
    if (!m_bIsRunning || m_bIsConnected || m_lPlayerNames.isEmpty())
        return;
    if (!coachClient()->autoAdvice())
        return;
    if (m_lPlayerNames[m_nCurrentPlayer] == "Computer")
        return;
    // after the end of the current event, so that the new turn is completely set up
    QTimer::singleShot(150, this, [this] {
        if (m_bIsRunning && !m_bIsConnected && !m_bIsHistory && !m_bIsComputing &&
            (m_lPlayerNames[m_nCurrentPlayer] != "Computer"))
            coachExplainImpl(true);
    });
}
