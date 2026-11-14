#pragma once

#include <QByteArray>
#include <QString>

// Short audible notification when a player places all pieces of the rack (Scrabble/Bingo).
// The jingle is synthesized in memory (four rising notes, a 16 bit mono WAV), so that its volume can be
// chosen; it needs no audio module and no file. Windows plays it with PlaySound, other platforms use the
// system beep. The settings (on/off, volume) are kept in a small ini file.
namespace sound {

constexpr int DefaultVolume = 70;

// WAV file (16 bit, mono, 22050 Hz) of the jingle; volume 0..100 (0 gives silence)
QByteArray scrabbleJingle(int volumePercent);

// Where the settings are kept; call once at start up
void configure(const QString &iniPath);
bool enabled();
void setEnabled(bool on);
int volume();            // 0..100
void setVolume(int percent);

} // namespace sound

// Plays the jingle when the sound is enabled; force = true plays it anyway (test button of the settings)
void playScrabbleSound(bool force = false);
