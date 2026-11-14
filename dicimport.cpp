#include "dicimport.h"

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QList>
#include <QStringList>
#include <algorithm>

namespace dicimport {

QString normalizeWord(const QString &line, int maxLength)
{
    QString s = line.trimmed();
    if (s.isEmpty() || s.startsWith('#'))
        return QString();
    // first word only: "mot=définition;3", "mot<TAB>..." or "mot, mot2"
    int end = 0;
    while (end < s.length() && !s[end].isSpace() && s[end] != '=' && s[end] != ';' && s[end] != ',')
        end++;
    s = s.left(end);

    s.replace(QStringLiteral("œ"), QStringLiteral("OE")); // oe
    s.replace(QStringLiteral("Œ"), QStringLiteral("OE")); // OE
    s.replace(QStringLiteral("æ"), QStringLiteral("AE")); // ae
    s.replace(QStringLiteral("Æ"), QStringLiteral("AE")); // AE
    s.replace(QStringLiteral("ß"), QStringLiteral("SS")); // sharp s

    QString result;
    const QString decomposed = s.normalized(QString::NormalizationForm_D);
    for (const QChar &c : decomposed) {
        if (c.category() == QChar::Mark_NonSpacing)
            continue; // accents
        const QChar u = c.toUpper();
        if (u >= 'A' && u <= 'Z')
            result.append(u);
        else
            return QString(); // digit, apostrophe, hyphen ...: not a playable word
    }
    if (result.length() < 2 || result.length() > maxLength)
        return QString();
    return result;
}

namespace {
struct Line {
    QString word;
    QByteArray raw; // the line as in the file, without end of line
};
}

Result importWordList(const QString &dicPath, const QString &wordListPath)
{
    Result r;

    QFile dic(dicPath);
    if (!dic.open(QIODevice::ReadOnly)) {
        r.error = QStringLiteral("Cannot read the dictionary: ") + dicPath;
        return r;
    }
    const QByteArray dicData = dic.readAll();
    dic.close();

    // header and categories are kept as they are, the words are parsed
    const QByteArray marker = "[Words]";
    const int posWords = dicData.indexOf(marker);
    if (posWords < 0) {
        r.error = QStringLiteral("The file is not a dictionary (no [Words] section).");
        return r;
    }
    int posAfter = dicData.indexOf('\n', posWords);
    if (posAfter < 0)
        posAfter = dicData.size() - 1;
    const QByteArray head = dicData.left(posAfter + 1);
    if (head.contains("\nKey=") || head.startsWith("Key=")) {
        r.error = QStringLiteral("Encrypted dictionaries cannot be extended.");
        return r;
    }
    const bool crlf = dicData.contains("\r\n");

    QList<Line> lines;
    QHash<QString, int> known;
    const QList<QByteArray> rawLines = dicData.mid(posAfter + 1).split('\n');
    for (QByteArray raw : rawLines) {
        if (raw.endsWith('\r'))
            raw.chop(1);
        if (raw.trimmed().isEmpty())
            continue;
        const QString text = QString::fromUtf8(raw);
        int end = 0;
        while (end < text.length() && text[end] != '=' && text[end] != ';')
            end++;
        const QString word = text.left(end);
        if (known.contains(word))
            continue; // duplicate in the original dictionary
        known.insert(word, int(lines.size()));
        lines.append({word, raw});
    }

    QFile list(wordListPath);
    if (!list.open(QIODevice::ReadOnly)) {
        r.error = QStringLiteral("Cannot read the word list: ") + wordListPath;
        return r;
    }
    const QList<QByteArray> listLines = list.readAll().split('\n');
    list.close();

    int nonEmpty = 0;
    for (const QByteArray &raw : listLines) {
        const QString text = QString::fromUtf8(raw).trimmed();
        if (text.isEmpty() || text.startsWith('#'))
            continue;
        nonEmpty++;
        const QString word = normalizeWord(text);
        if (word.isEmpty()) {
            r.rejected++;
            continue;
        }
        if (known.contains(word)) {
            r.alreadyKnown++;
            continue;
        }
        known.insert(word, int(lines.size()));
        lines.append({word, word.toUtf8()});
        r.added++;
    }
    if (nonEmpty == 0) {
        r.error = QStringLiteral("The word list is empty.");
        return r;
    }

    if (r.added > 0) {
        std::sort(lines.begin(), lines.end(), [](const Line &a, const Line &b) {
            return QString::compare(a.word, b.word) < 0; // same order as the lookup in dictionary
        });

        r.backupPath = dicPath + QStringLiteral(".bak-") + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
        if (!QFile::copy(dicPath, r.backupPath)) {
            r.error = QStringLiteral("Cannot save the dictionary before the import: ") + r.backupPath;
            return r;
        }

        // written beside the dictionary, then renamed: a failure never leaves a half written file
        const QString tmpPath = dicPath + QStringLiteral(".new");
        QFile out(tmpPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            r.error = QStringLiteral("Cannot write: ") + tmpPath;
            return r;
        }
        const QByteArray eol = crlf ? "\r\n" : "\n";
        out.write(head);
        for (const Line &l : std::as_const(lines)) {
            out.write(l.raw);
            out.write(eol);
        }
        out.close();
        if (!QFile::remove(dicPath) || !QFile::rename(tmpPath, dicPath)) {
            r.error = QStringLiteral("Cannot replace the dictionary; the previous version is saved as ") + r.backupPath;
            return r;
        }
    }
    r.total = int(lines.size());
    r.ok = true;
    return r;
}

} // namespace dicimport
