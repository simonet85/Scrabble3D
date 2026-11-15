#include "gameplay.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextDocument>

// ---- match sheet (FISF 1.1.5, 3.1.6 and 5.3): moves, jokers, sanctions, result, signatures

GamePlay::SheetData GamePlay::sheetData()
{
    SheetData d;
    const int players = int(m_nPlayerCount);
    QList<int> cumul(players, 0);
    for (int i = 0; i < m_pGameCourseModel->moveCount(); i++) {
        SheetRow r;
        r.number = i + 1;
        r.player = m_pGameCourseModel->getWho(i);
        r.word = replaceAllLetters(m_pGameCourseModel->getWord(i));
        r.value = m_pGameCourseModel->getValue(i);
        if ((r.player >= 0) && (r.player < players)) {
            cumul[r.player] += r.value;
            r.total = cumul[r.player];
        }
        d.rows.append(r);
    }
    for (int p = 0; p < players; p++) {
        d.names.append(m_lPlayerNames.value(p));
        d.scores.append(m_pGameCourseModel->getScore(p));
        d.adjustments.append(m_pGameCourseModel->getScore(p) - cumul[p]); // end of game, time, sanctions, resignation
    }
    for (const JokerEntry &j : std::as_const(m_lJokers))
        d.jokers.append(tr("%1 in %2 (move %3, %4)").arg(j.letter, j.word).arg(j.move).arg(m_lPlayerNames.value(j.player)));
    if (players == 2)
        d.difference = qMin(qAbs(d.scores[0] - d.scores[1]), 100);
    if (!m_bIsRunning) {
        QList<int> winners;
        m_pGameCourseModel->getWinner(winners);
        for (int w : std::as_const(winners))
            d.winners.append(m_lPlayerNames.value(w));
    }
    d.finished = !m_bIsRunning;
    return d;
}

static QString csvField(const QString &s)
{
    QString t = s;
    if (t.contains(QLatin1Char(';')) || t.contains(QLatin1Char('"')) || t.contains(QLatin1Char('\n')))
        t = QLatin1Char('"') + t.replace(QLatin1Char('"'), QStringLiteral("\"\"")) + QLatin1Char('"');
    return t;
}

QString GamePlay::matchSheetCsv()
{
    const SheetData d = sheetData();
    QStringList out;
    auto line = [&](const QStringList &fields) {
        QStringList f;
        for (const QString &s : fields) f.append(csvField(s));
        out.append(f.join(QLatin1Char(';')));
    };
    line({tr("Match sheet"), QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"))});
    line({});
    line({tr("Move"), tr("Player"), tr("Word"), tr("Points"), tr("Total")});
    for (const SheetRow &r : d.rows)
        line({QString::number(r.number), d.names.value(r.player), r.word.isEmpty() ? tr("pass or exchange") : r.word,
              QString::number(r.value), QString::number(r.total)});
    line({});
    line({tr("Player"), tr("Moves"), tr("Adjustments"), tr("Final score")});
    for (int p = 0; p < d.names.count(); p++)
        line({d.names[p], QString::number(d.scores[p] - d.adjustments[p]), QString::number(d.adjustments[p]), QString::number(d.scores[p])});
    if (d.difference >= 0)
        line({tr("Difference (limited to 100 points)"), QString::number(d.difference)});
    line({tr("Winner"), d.finished ? d.winners.join(QStringLiteral(" / ")) : tr("game in progress")});
    line({tr("Jokers"), d.jokers.isEmpty() ? tr("none") : d.jokers.join(QStringLiteral(", "))});
    line({});
    line({tr("Sanctions")});
    if (m_lSanctions.isEmpty())
        line({tr("none")});
    for (const SanctionEntry &s : std::as_const(m_lSanctions))
        line({tr("Move %1").arg(s.move), m_lPlayerNames.value(s.player), sanctionText(s.level), s.reason});
    line({});
    for (const QString &n : d.names)
        line({tr("Signature of %1").arg(n), QString()});
    return out.join(QStringLiteral("\r\n")) + QStringLiteral("\r\n");
}

QString GamePlay::matchSheetHtml()
{
    const SheetData d = sheetData();
    auto esc = [](const QString &s) { return s.toHtmlEscaped(); };
    QString h = QStringLiteral("<html><head><meta charset=\"utf-8\"><style>"
                               "body{font-family:sans-serif;font-size:10pt} table{border-collapse:collapse}"
                               "td,th{border:1px solid #888;padding:2px 6px} th{background:#ddd}</style></head><body>");
    h += QStringLiteral("<h2>") + esc(tr("Match sheet")) + QStringLiteral("</h2><p>")
         + esc(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"))) + QStringLiteral(" &mdash; ")
         + esc(d.names.join(QStringLiteral(" / "))) + QStringLiteral("</p>");

    h += QStringLiteral("<table><tr><th>") + esc(tr("Move")) + QStringLiteral("</th><th>") + esc(tr("Player"))
         + QStringLiteral("</th><th>") + esc(tr("Word")) + QStringLiteral("</th><th>") + esc(tr("Points"))
         + QStringLiteral("</th><th>") + esc(tr("Total")) + QStringLiteral("</th></tr>");
    for (const SheetRow &r : d.rows)
        h += QStringLiteral("<tr><td align=\"right\">%1</td><td>%2</td><td>%3</td><td align=\"right\">%4</td><td align=\"right\">%5</td></tr>")
                 .arg(r.number).arg(esc(d.names.value(r.player)), esc(r.word.isEmpty() ? tr("pass or exchange") : r.word))
                 .arg(r.value).arg(r.total);
    h += QStringLiteral("</table><h3>") + esc(tr("Result")) + QStringLiteral("</h3><table><tr><th>") + esc(tr("Player"))
         + QStringLiteral("</th><th>") + esc(tr("Moves")) + QStringLiteral("</th><th>") + esc(tr("Adjustments"))
         + QStringLiteral("</th><th>") + esc(tr("Final score")) + QStringLiteral("</th></tr>");
    for (int p = 0; p < d.names.count(); p++)
        h += QStringLiteral("<tr><td>%1</td><td align=\"right\">%2</td><td align=\"right\">%3</td><td align=\"right\"><b>%4</b></td></tr>")
                 .arg(esc(d.names[p])).arg(d.scores[p] - d.adjustments[p]).arg(d.adjustments[p]).arg(d.scores[p]);
    h += QStringLiteral("</table><p>");
    if (d.difference >= 0)
        h += esc(tr("Difference (limited to 100 points)")) + QStringLiteral(" : %1<br>").arg(d.difference);
    h += esc(tr("Winner")) + QStringLiteral(" : ") + esc(d.finished ? d.winners.join(QStringLiteral(" / ")) : tr("game in progress"))
         + QStringLiteral("<br>") + esc(tr("Jokers")) + QStringLiteral(" : ")
         + esc(d.jokers.isEmpty() ? tr("none") : d.jokers.join(QStringLiteral(", "))) + QStringLiteral("</p>");

    h += QStringLiteral("<h3>") + esc(tr("Sanctions")) + QStringLiteral("</h3>");
    if (m_lSanctions.isEmpty())
        h += QStringLiteral("<p>") + esc(tr("none")) + QStringLiteral("</p>");
    else {
        h += QStringLiteral("<ul>");
        for (const SanctionEntry &s : std::as_const(m_lSanctions))
            h += QStringLiteral("<li>") + esc(tr("Move %1").arg(s.move)) + QStringLiteral(" &mdash; ") + esc(m_lPlayerNames.value(s.player))
                 + QStringLiteral(" : ") + esc(sanctionText(s.level)) + (s.reason.isEmpty() ? QString() : QStringLiteral(" (") + esc(s.reason) + QStringLiteral(")"))
                 + QStringLiteral("</li>");
        h += QStringLiteral("</ul>");
    }
    h += QStringLiteral("<p><br>");
    for (const QString &n : d.names)
        h += esc(tr("Signature of %1").arg(n)) + QStringLiteral(" : ____________________<br><br>");
    h += QStringLiteral("</p></body></html>");
    return h;
}

bool GamePlay::exportMatchSheet(const QUrl &fileUrl)
{
    const QString fileName = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    if (fileName.isEmpty() || (m_pGameCourseModel->moveCount() == 0)) {
        addMessage(tr("There is no game to export."));
        return false;
    }
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    bool ok = false;
    if (suffix == QLatin1String("pdf")) {
        QPdfWriter writer(fileName);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setResolution(96);
        writer.setTitle(tr("Match sheet"));
        QTextDocument doc;
        doc.setHtml(matchSheetHtml());
        doc.print(&writer);
        ok = true;
    } else {
        QFile f(fileName);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QByteArray data = (suffix == QLatin1String("csv")) ? matchSheetCsv().toUtf8() : matchSheetHtml().toUtf8();
            if (suffix == QLatin1String("csv"))
                data.prepend("\xEF\xBB\xBF"); // UTF-8 mark so that spreadsheets show the accents
            ok = f.write(data) == data.size();
            f.close();
        }
    }
    if (ok && (suffix == QLatin1String("pdf")))
        ok = QFileInfo(fileName).size() > 0;
    addMessage(ok ? tr("Match sheet saved to %1.").arg(QDir::toNativeSeparators(fileName))
                  : tr("The match sheet could not be saved to %1.").arg(QDir::toNativeSeparators(fileName)));
    return ok;
}
