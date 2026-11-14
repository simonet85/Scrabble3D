#include "sound.h"

#include <QApplication>
#include <QDebug>
#include <QtConcurrent>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

void playScrabbleSound()
{
#ifdef Q_OS_WIN
    // Beep() blocks until the note is finished: play the notes in a worker thread
    (void) QtConcurrent::run([] {
        const struct { DWORD frequency; DWORD duration; } notes[] = {
            {784, 110},   // G5
            {988, 110},   // B5
            {1175, 110},  // D6
            {1568, 320},  // G6
        };
        for (const auto &note : notes)
            if (!Beep(note.frequency, note.duration)) {
                qWarning() << "Scrabble sound: Beep() failed";
                return;
            }
    });
#else
    QApplication::beep();
#endif
}
