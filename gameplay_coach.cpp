#include "gameplay.h"
#include "coach.h"
#include "configpath.h"
#include "computelevel.h"

#include <QMessageBox>
#include <QTimer>

// The coach advises; the dictionary decides. This is said to the model and repeated in the code
// below: nothing the coach answers is ever used to accept or refuse a word.
static const char *coachSystem =
    "Tu es un coach de Scrabble classique francophone. Reponds en francais, en 2 ou 3 phrases courtes "
    "(50 mots au maximum), une seule idee principale, texte brut sans mise en forme (ni gras, ni listes, ni tableaux). "
    "Commence directement par le coup conseille ; pas de salutation, pas d'encouragement, pas de conclusion.\n"
    "Regles strictes :\n"
    "- Les scores des coups sont fournis par le moteur du jeu : reprends-les tels quels, ne refais aucun calcul de points.\n"
    "- Les cases primes : parle seulement de ce que disent les donnees (\"ouvre a l'adversaire des cases mot compte "
    "double ou triple\" ou \"n'ouvre aucune case\") ; n'invente jamais une case ni son emplacement.\n"
    "- Tu n'inventes rien : si une information n'est pas dans les donnees, ne l'affirme pas.\n"
    "- Les tuiles jouees et les lettres gardees de chaque coup sont fournies : reprends-les telles quelles, "
    "ne deduis jamais toi-meme les lettres qui restent et ne cite aucune lettre ou aucun mot absents des donnees.\n"
    "- Un mot peut utiliser des tuiles deja posees sur le plateau : seules les tuiles jouees viennent du chevalet.\n"
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

// waits (at most about 3 seconds) until the engine is free, then gives the automatic advice of the turn
void GamePlay::coachAdviceWhenFree(const int turn, const int attempt)
{
    if (!m_bIsRunning || m_bIsConnected || m_bIsHistory || m_nCurrentMove != turn ||
        (m_lPlayerNames[m_nCurrentPlayer] == "Computer"))
        return;
    if (m_bIsComputing) {
        if (attempt < 10)
            QTimer::singleShot(300, this, [this, turn, attempt] { coachAdviceWhenFree(turn, attempt + 1); });
        return;
    }
    coachExplainImpl(true);
}

// automatic: asked by the game itself at the start of the turn of a human; it stays silent when
// something is missing (key, agreement) instead of bothering the player
void GamePlay::coachExplainImpl(const bool automatic)
{
    if (!m_bIsRunning || m_bIsConnected || m_pBoard->is3D()) {
        if (!automatic)
            addMessage(tr("The coach is available in a running 2D game that is not played on the network."));
        return;
    }
    if (m_bIsComputing) {
        if (!automatic)
            addMessage(tr("The computer is busy, try again in a moment."));
        return;
    }
    if (!coachReady(automatic))
        return;

    computeMove(); // finds the moves of the engine for the current rack
    if (m_nComputeResults == 0)
        return; // computeMove() has already said that no word was found

    // the pieces really on the rack: with an empty bag some slots are empty
    const int nRack = m_pRackModel->rackSize();
    QList<computelevel::KeptPiece> rackPieces;
    QList<bool> onRack;
    QString rack;
    for (int i = 0; i < nRack; i++) {
        const Letter l = m_pRackModel->getLetter(i);
        const bool piece = (l.State == LetterState::lsRack) && !l.What.isEmpty() && (l.What != QString(QChar(0)));
        onRack.append(piece);
        computelevel::KeptPiece kp;
        kp.what = l.What;
        kp.isJoker = l.IsJoker;
        kp.value = l.Value;
        rackPieces.append(kp);
        if (piece)
            rack += l.IsJoker ? QStringLiteral("?") : l.What;
    }
    QString scores;
    for (uint p = 0; p < m_nPlayerCount; p++)
        scores += m_lPlayerNames[p] + " " + QString::number(m_pGameCourseModel->getScore(p)) + "  ";

    // what each candidate move uses and keeps: computed here, so that the model never has to guess it
    struct Candidate {
        int index = 0;
        int score = 0;
        QList<computelevel::KeptPiece> kept;
        double equity = 0;
        double opening = 0; // points the move gives away by opening premium word squares
    };
    const bool bagEmpty = m_lBag.isEmpty();
    const int nCandidates = qMin(m_nComputeResults, 40);
    const computelevel::BoardView view = m_bMasterDefense ? boardView() : computelevel::BoardView();
    QList<Candidate> candidates;
    QVector<int> scoreList;
    QVector<double> equityList;
    for (int i = 0; i < nCandidates; i++) {
        const sharedMove aMove = m_pComputeMove->result(i);
        QList<bool> used(nRack, false);
        for (int j = 0; j < aMove->letterCount(); j++) {
            const int pos = aMove->getLetter(j).RackPos;
            if (pos >= 0 && pos < nRack)
                used[pos] = true;
        }
        Candidate c;
        c.index = i;
        c.score = int(aMove->Value());
        for (int k = 0; k < nRack; k++)
            if (onRack[k] && !used[k])
                c.kept.append(rackPieces[k]);
        c.opening = defensePenalty(aMove, view);
        c.equity = computelevel::equity(c.score, c.kept, bagEmpty) - c.opening;
        candidates.append(c);
        scoreList.append(c.score);
        equityList.append(c.equity);
    }
    // the recommended move is chosen by the engine, not by the model: best balance between the points
    // and the pieces kept among the moves at most 30 points below the best score
    QRandomGenerator rng(0);
    int recommended = computelevel::pickMove(computelevel::Master, scoreList, equityList, rng);
    if (recommended < 0)
        recommended = 0;
    QList<int> order{recommended};
    for (int i = 0; i < nCandidates && order.size() < 5; i++)
        if (i != recommended)
            order.append(i);

    QString moves;
    for (int k = 0; k < order.size(); k++) {
        const Candidate &c = candidates[order[k]];
        const sharedMove aMove = m_pComputeMove->result(c.index);
        const Letter first = aMove->getLetter(0);
        QString keptLetters;
        int keptValue = 0;
        for (const computelevel::KeptPiece &piece : c.kept) {
            keptLetters += piece.isJoker ? QStringLiteral("?") : piece.what;
            keptValue += piece.value;
        }
        QString keptText;
        if (c.kept.isEmpty())
            keptText = bagEmpty ? QStringLiteral("aucune, le chevalet est vide : ce coup termine la partie")
                                : QStringLiteral("aucune, tout le chevalet est joue");
        else
            keptText = keptLetters + QStringLiteral(" (") + QString::number(keptValue) + QStringLiteral(" points de lettres)");
        moves += (k == 0 ? QStringLiteral("COUP RECOMMANDE : ") : QStringLiteral("autre coup : "))
                 + aMove->PlacedWord
                 + (aMove->ConnectedWords.isEmpty() ? QString() : QStringLiteral(" (+ ") + aMove->ConnectedWords + QStringLiteral(")"))
                 + QStringLiteral(" - ") + QString::number(c.score) + QStringLiteral(" points - ")
                 + QString::number(aMove->letterCount()) + QStringLiteral(" tuiles du chevalet jouees, premiere tuile posee en ")
                 + QChar('A' + first.Point.y()) + QString::number(first.Point.x() + 1)
                 + (aMove->activeDimension() == dmOrdinate ? QStringLiteral(" (vertical)") : QStringLiteral(" (horizontal)"))
                 + QStringLiteral(" - lettres gardees : ") + keptText
                 + (c.opening > 0 ? QStringLiteral(" - ouvre a l'adversaire des cases mot compte double ou triple (risque estime : ")
                                        + QString::number(qRound(c.opening)) + QStringLiteral(" points)")
                                  : QStringLiteral(" - n'ouvre aucune case mot compte double ou triple"))
                 + QStringLiteral("\n");
    }

    QString user = QStringLiteral(
        "Plateau classique 15x15 (colonnes 1 a 15 de gauche a droite, lignes A a O de haut en bas ; "
        "'.' = case vide, minuscule = joker) :\n") + coachBoardText() + QStringLiteral("\n");
    if (m_bIsFirstMove)
        user += QStringLiteral("(coup d'ouverture : le plateau est vide)\n");
    user += QStringLiteral("Chevalet du joueur ('?' = joker) : ") + rack + QStringLiteral("\n");
    user += QStringLiteral("Lettres restantes dans le sac : ") + QString::number(m_lBag.count()) + QStringLiteral("\n");
    if (bagEmpty)
        user += QStringLiteral("Le sac est vide : les lettres gardees seront retranchees du score en fin de partie, "
                               "et le joueur qui vide son chevalet termine la partie.\n");
    user += QStringLiteral("Scores : ") + scores + QStringLiteral("\n\nCoups calcules par le moteur (tuiles jouees et lettres gardees "
                           "sont calculees par le jeu, ne les deduis pas toi-meme) :\n") + moves
         + QStringLiteral("En 2 phrases courtes (50 mots au maximum), dis pourquoi le COUP RECOMMANDE est un bon choix, en citant "
                          "ses points et ses lettres gardees exactement comme indiques ci-dessus. Ne cite aucune autre lettre.");

    coachBegin(automatic ? "auto" : "advice",
               tr("Rack %1 - %2 possible moves, recommended: %3 (%4 points)")
                   .arg(rack)
                   .arg(m_nComputeResults)
                   .arg(m_pComputeMove->result(candidates[recommended].index)->PlacedWord)
                   .arg(candidates[recommended].score));
    if (!automatic)
        addMessage(tr("The coach is thinking..."));
    coachClient()->ask(QString::fromUtf8(coachSystem), user, 300, 0.3);
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
        "Pour chaque mot, donne en une phrase de 12 mots au maximum son sens dans la langue francaise courante et sa nature "
        "(nom, verbe, interjection...). Pour un nom propre, une marque, un sigle ou un mot que tu ne connais "
        "pas avec certitude, reponds seulement 'je ne connais pas ce mot' : n'invente jamais un sens ni une "
        "origine. Reste neutre et sobre. Ne dis pas s'il est valide au Scrabble.").arg(words.join(", "));

    coachBegin("define", tr("Definitions of: %1").arg(words.join(", ")));
    addMessage(tr("The coach is thinking..."));
    coachClient()->ask(QString::fromUtf8(coachSystem), user, 300, 0.2);
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
    const int turn = m_nCurrentMove;
    QTimer::singleShot(150, this, [this, turn] { coachAdviceWhenFree(turn, 0); });
}

// ---------------------------------------------------------------------------------------------
// Game review: the best move of each turn of the human players is recorded (silently, at the start of
// the turn) and compared with the move that was played when the game is over.

bool GamePlay::reviewRecording() { return coachClient()->flag("reviewRecording", true); }

void GamePlay::setReviewRecording(bool on)
{
    if (reviewRecording() == on)
        return;
    coachClient()->setFlag("reviewRecording", on);
    emit reviewChanged();
    if (on)
        scheduleTurnBest();
}

QList<gamereview::MoveRecord> GamePlay::reviewRecords()
{
    QList<gamereview::MoveRecord> records;
    for (int i = 0; i < m_pGameCourseModel->moveCount(); i++) {
        const int who = m_pGameCourseModel->getWho(i);
        if (who < 0 || who >= m_lPlayerNames.count() || m_lPlayerNames[who] == "Computer")
            continue;
        gamereview::MoveRecord r;
        r.number = i / int(qMax<uint>(1, m_nPlayerCount)) + 1;
        r.player = m_lPlayerNames[who];
        r.playedWord = m_pGameCourseModel->getWord(i);
        r.playedValue = m_pGameCourseModel->getValue(i);
        r.bestValue = m_pGameCourseModel->getBestValue(i);
        r.bestWord = m_turnBest.value(i).second;
        records.append(r);
    }
    return records;
}

bool GamePlay::reviewAvailable()
{
    if (m_bIsRunning)
        return false;
    for (const gamereview::MoveRecord &r : reviewRecords())
        if (r.bestValue > 0)
            return true;
    return false;
}

// records the result of a computation of the moves made at the start of a turn
void GamePlay::recordTurnBest()
{
    if (m_bIsHistory || m_bIsConnected)
        return;
    const bool found = m_nComputeResults > 0;
    m_turnBest[m_nCurrentMove] = qMakePair(found ? int(m_pComputeMove->result(0)->Value()) : 0,
                                           found ? m_pComputeMove->result(0)->PlacedWord : QString());
}

// computes the best move of the current turn of a human without showing anything: the moves found are not
// kept, so that this can never be used as a hint
void GamePlay::recordTurnBestSilently()
{
    if (!m_bIsRunning || m_bIsConnected || m_bIsHistory || m_pBoard->is3D() || !reviewRecording())
        return;
    if (m_lPlayerNames.isEmpty() || m_lPlayerNames[m_nCurrentPlayer] == "Computer")
        return;
    if (m_turnBest.contains(m_nCurrentMove))
        return; // already known (the coach or the player computed the moves of this turn)
    if (m_pMoves.last()->letterCount() > 0)
        return; // the player has started to place pieces: the rack is no longer complete
    if (m_bIsComputing) {
        // the engine is busy (the coach or the player): try again shortly, for the same turn
        const int turn = m_nCurrentMove;
        QTimer::singleShot(300, this, [this, turn] { if (m_nCurrentMove == turn) recordTurnBestSilently(); });
        return;
    }
    m_bIsComputing = true; emit isComputingChanged();
    m_pComputeMove->run(m_bIsFirstMove, m_pBoard, m_pRackModel, m_pDictionary);
    m_bIsComputing = false; emit isComputingChanged();
    recordTurnBest();
    m_pComputeMove->clear();
    setComputeResults(0);
}

void GamePlay::scheduleTurnBest()
{
    if (!m_bIsRunning || m_bIsConnected || m_lPlayerNames.isEmpty() || !reviewRecording())
        return;
    if (m_lPlayerNames[m_nCurrentPlayer] == "Computer")
        return;
    // after the automatic advice of the coach (which computes the moves too) and the end of the current event
    QTimer::singleShot(400, this, [this] { recordTurnBestSilently(); });
}

void GamePlay::scheduleGameReview()
{
    if (m_bReviewDone || m_bIsConnected || !reviewRecording())
        return;
    QTimer::singleShot(800, this, [this] {
        if (!m_bIsRunning && !m_bReviewDone)
            runGameReview(false);
    });
}

void GamePlay::gameReview() { runGameReview(true); }

void GamePlay::runGameReview(const bool manual)
{
    const QList<gamereview::PlayerSummary> all = gamereview::summarize(reviewRecords(), 3);
    QList<gamereview::PlayerSummary> usable;
    int analysed = 0;
    for (const gamereview::PlayerSummary &s : all)
        if (s.analysed > 0) {
            usable.append(s);
            analysed += s.analysed;
        }
    if (usable.isEmpty()) {
        if (manual)
            addMessage(tr("Game review: no data. The best move is recorded at the start of each of your turns (option of the Coach tab)."));
        return;
    }
    m_bReviewDone = true;

    QString html, facts;
    for (const gamereview::PlayerSummary &s : usable) {
        if (!html.isEmpty())
            html += QStringLiteral("<br><br>");
        html += gamereview::toHtml(s);
        facts += gamereview::toCoachFacts(s) + QStringLiteral("\n");
    }
    if (m_nHintsUsed > 0) {
        html += QStringLiteral("<br><br>") + tr("Hints used: %1 of %2.").arg(m_nHintsUsed).arg(hintsPerGame());
        facts += QStringLiteral("Indices utilises par le joueur : %1 sur %2.\n").arg(m_nHintsUsed).arg(hintsPerGame());
    }
    // the figures are computed by the game: they are shown at once, with or without a coach
    const int id = m_pCoachModel->addPending(QStringLiteral("Scrabble3D"), tr("computed by the game"), "review",
                                             tr("Review of the game: %1 moves analysed").arg(analysed));
    m_pCoachModel->setDone(id, html, -1);
    m_pMsgModel->addMessage(html);

    // the comment of the coach: asked by hand, or automatically when the automatic advice is on
    const bool askCoach = manual ? coachReady(false) : (coachClient()->autoAdvice() && coachReady(true));
    if (!askCoach)
        return;
    const QString user = QStringLiteral("Bilan de la partie, calcule par le jeu (compare chaque coup au meilleur coup possible a ce tour) :\n")
                         + facts
                         + QStringLiteral("Il n'y a pas de coup a conseiller ici : en 2 phrases courtes (50 mots au maximum), donne le principal "
                                          "axe de progres du joueur, en t'appuyant seulement sur ces chiffres. N'invente aucun mot ni aucun score.");
    coachBegin("reviewcomment", tr("Comment of the coach on the review"));
    coachClient()->ask(QString::fromUtf8(coachSystem), user, 300, 0.3);
}
