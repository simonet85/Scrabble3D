#pragma once

#include <QString>

// Merges a plain word list (for example a newer edition of the ODS) into a .dic dictionary.
namespace dicimport {

struct Result {
    bool ok = false;
    QString error;        // reason when !ok
    int added = 0;        // new words written to the dictionary
    int alreadyKnown = 0; // words of the list that were already in the dictionary
    int rejected = 0;     // lines that are not usable words (too short, too long, digits ...)
    int total = 0;        // words in the dictionary after the import
    QString backupPath;   // copy of the dictionary made before it was changed
};

// Turns a line of a word list into a dictionary word: upper case, without accents or ligatures,
// letters A-Z only. Takes the first word of the line, ignores "=meaning" and ";category" parts.
// Returns an empty string when the line holds no usable word (2 to maxLength letters).
QString normalizeWord(const QString &line, int maxLength = 15);

// Merges the words of wordListPath into the dictionary dicPath. The dictionary is saved first
// (dicPath + ".bak-yyyyMMdd-hhmmss"), the words are kept sorted (the lookup relies on it) and
// existing words keep their meaning and category. Encrypted dictionaries are refused.
Result importWordList(const QString &dicPath, const QString &wordListPath);

} // namespace dicimport
