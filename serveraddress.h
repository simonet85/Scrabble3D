#pragma once

#include <QRegularExpression>
#include <QString>

// "host", "host:port" or "tcp://host:port" typed in the network dialog.
namespace serveraddress {

constexpr uint DefaultPort = 5001;
inline const char *DefaultServer = "scrabble3d.ignorelist.com:5001";

struct Address {
    QString host;
    uint port = DefaultPort;
    bool valid = false;
};

inline Address parse(const QString &text)
{
    Address a;
    QString s = text.trimmed();
    if (s.startsWith(QStringLiteral("tcp://"), Qt::CaseInsensitive))
        s = s.mid(6);
    if (s.endsWith('/'))
        s.chop(1);
    if (s.isEmpty())
        return a;

    const int colon = s.lastIndexOf(':');
    QString host = s;
    if (colon >= 0) {
        host = s.left(colon);
        bool ok = false;
        const uint port = s.mid(colon + 1).toUInt(&ok);
        if (!ok || port < 1 || port > 65535)
            return a;
        a.port = port;
    }
    static const QRegularExpression hostName(QStringLiteral("^[A-Za-z0-9]([A-Za-z0-9.-]{0,251}[A-Za-z0-9])?$"));
    if (!hostName.match(host).hasMatch())
        return a;
    a.host = host;
    a.valid = true;
    return a;
}

} // namespace serveraddress
