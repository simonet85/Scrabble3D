#pragma once

#include <QHash>
#include <QTranslator>

// French translation of the texts that were added to the game after the downloaded translation files were
// written. It is installed BEFORE the downloaded French file, which therefore keeps the priority: this
// translator only answers for the texts that the downloaded file does not know.
class frenchsupplement : public QTranslator
{
    Q_OBJECT
public:
    explicit frenchsupplement(QObject *parent = nullptr) : QTranslator(parent) {}
    QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr, int n = -1) const override;
    bool isEmpty() const override { return false; }
    // English text -> French text (the context is ignored)
    static const QHash<QString, QString> &table();
};
