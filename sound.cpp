#include "sound.h"

#include <QApplication>
#include <QDataStream>
#include <QIODevice>
#include <QSettings>
#include <QtMath>
#include <cmath>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace sound {

static QString g_sIniPath;

static QSettings settings()
{
    return g_sIniPath.isEmpty() ? QSettings() : QSettings(g_sIniPath, QSettings::IniFormat);
}

void configure(const QString &iniPath) { g_sIniPath = iniPath; }

bool enabled()
{
    QSettings s = settings();
    return s.value("enabled", true).toBool();
}

void setEnabled(bool on)
{
    QSettings s = settings();
    s.setValue("enabled", on);
    s.sync();
}

int volume()
{
    QSettings s = settings();
    return qBound(0, s.value("volume", DefaultVolume).toInt(), 100);
}

void setVolume(int percent)
{
    QSettings s = settings();
    s.setValue("volume", qBound(0, percent, 100));
    s.sync();
}

QByteArray scrabbleJingle(int volumePercent)
{
    constexpr int sampleRate = 22050;
    struct Note { double frequency; int milliseconds; };
    static const Note notes[] = {
        {784.0, 110},   // G5
        {988.0, 110},   // B5
        {1175.0, 110},  // D6
        {1568.0, 320},  // G6
    };
    const double v = qBound(0, volumePercent, 100) / 100.0;
    // perceived loudness grows faster than the amplitude: the amplitude follows the square of the setting
    const double peak = 0.6 * 32767.0 * v * v;

    QList<qint16> samples;
    for (const Note &note : notes) {
        const int count = sampleRate * note.milliseconds / 1000;
        const int attack = sampleRate * 4 / 1000;    // 4 ms: no click at the start of a note
        const int release = sampleRate * 40 / 1000;  // 40 ms: the note fades out
        for (int i = 0; i < count; i++) {
            const double t = double(i) / sampleRate;
            double s = std::sin(2.0 * M_PI * note.frequency * t) + 0.25 * std::sin(4.0 * M_PI * note.frequency * t);
            s /= 1.25;
            double envelope = 1.0;
            if (i < attack)
                envelope = double(i) / attack;
            else if (i > count - release)
                envelope = double(count - i) / release;
            samples.append(qint16(std::lround(s * envelope * peak)));
        }
    }

    const quint32 dataSize = quint32(samples.size()) * 2;
    QByteArray wav;
    QDataStream out(&wav, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << quint32(36 + dataSize);
    out.writeRawData("WAVE", 4);
    out.writeRawData("fmt ", 4);
    out << quint32(16) << quint16(1) << quint16(1) << quint32(sampleRate) << quint32(sampleRate * 2) << quint16(2) << quint16(16);
    out.writeRawData("data", 4);
    out << dataSize;
    for (qint16 sample : samples)
        out << sample;
    return wav;
}

} // namespace sound

void playScrabbleSound(bool force)
{
    if (!force && !sound::enabled())
        return;
    const int volume = sound::volume();
    if (volume <= 0)
        return;
#ifdef Q_OS_WIN
    // SND_MEMORY needs the buffer to stay valid while the sound plays: it is kept until the next call
    static QByteArray buffer;
    buffer = sound::scrabbleJingle(volume);
    PlaySoundA(reinterpret_cast<LPCSTR>(buffer.constData()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
    QApplication::beep();
#endif
}
