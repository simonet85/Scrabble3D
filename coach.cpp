#include "coach.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

coach::coach(const QString &settingsPath, QObject *parent)
    : QObject(parent), m_sSettingsPath(settingsPath)
{
}

static QSettings openSettings(const QString &path)
{
    return QSettings(path, QSettings::IniFormat);
}

bool coach::hasKey() const
{
    QSettings s = openSettings(m_sSettingsPath);
    return !s.value("apiKey").toString().trimmed().isEmpty();
}

QString coach::model() const
{
    QSettings s = openSettings(m_sSettingsPath);
    const QString m = s.value("model").toString().trimmed();
    return m.isEmpty() ? QString::fromLatin1(DefaultModel) : m;
}

QString coach::baseUrl() const
{
    QSettings s = openSettings(m_sSettingsPath);
    const QString u = s.value("baseUrl").toString().trimmed();
    return u.isEmpty() ? QString::fromLatin1(DefaultBaseUrl) : u;
}

bool coach::hasConsent() const
{
    QSettings s = openSettings(m_sSettingsPath);
    return s.value("consent", false).toBool();
}

QString coach::maskedKey() const
{
    QSettings s = openSettings(m_sSettingsPath);
    const QString k = s.value("apiKey").toString().trimmed();
    if (k.isEmpty())
        return QString();
    if (k.length() <= 12)
        return QString(k.length(), QChar('*'));
    return k.left(7) + QStringLiteral("...") + k.right(4);
}

void coach::setKey(const QString &apiKey)
{
    QSettings s = openSettings(m_sSettingsPath);
    if (apiKey.trimmed().isEmpty())
        s.remove("apiKey");
    else
        s.setValue("apiKey", apiKey.trimmed());
    s.sync();
}

void coach::setModel(const QString &model)
{
    QSettings s = openSettings(m_sSettingsPath);
    s.setValue("model", model.trimmed());
    s.sync();
}

void coach::setBaseUrl(const QString &baseUrl)
{
    QSettings s = openSettings(m_sSettingsPath);
    s.setValue("baseUrl", baseUrl.trimmed());
    s.sync();
}

void coach::setConsent(bool consent)
{
    QSettings s = openSettings(m_sSettingsPath);
    s.setValue("consent", consent);
    s.sync();
}

QJsonObject coach::buildRequest(const QString &system, const QString &user, int maxTokens) const
{
    QJsonObject message;
    message["role"] = "user";
    message["content"] = user;

    QJsonObject body;
    body["model"] = model();
    body["max_tokens"] = maxTokens;
    body["system"] = system;
    body["messages"] = QJsonArray{message};
    return body;
}

QString coach::parseResponse(const QByteArray &body)
{
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject())
        return QString();
    const QJsonArray content = doc.object().value("content").toArray();
    QString text;
    for (const QJsonValue &v : content) {
        const QJsonObject block = v.toObject();
        if (block.value("type").toString() == "text")
            text += block.value("text").toString();
    }
    return text.trimmed();
}

QString coach::parseError(int httpStatus, const QByteArray &body)
{
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (doc.isObject()) {
        const QString message = doc.object().value("error").toObject().value("message").toString();
        if (!message.isEmpty())
            return message;
    }
    switch (httpStatus) {
    case 401:
    case 403:
        return QObject::tr("The API key is not valid or not allowed.");
    case 429:
        return QObject::tr("Too many requests or no credit left; try again later.");
    case 529:
        return QObject::tr("The service is overloaded; try again later.");
    default:
        return QObject::tr("The service answered with the error %1.").arg(httpStatus);
    }
}

void coach::ask(const QString &system, const QString &user, int maxTokens)
{
    if (!hasKey()) {
        emit failed(tr("No API key: enter it in Configuration > AI coach."));
        return;
    }
    QSettings s = openSettings(m_sSettingsPath);
    const QString key = s.value("apiKey").toString().trimmed();

    QNetworkRequest request(QUrl(baseUrl() + QStringLiteral("/v1/messages")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", key.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");
    request.setTransferTimeout(45000);

    const QByteArray payload = QJsonDocument(buildRequest(system, user, maxTokens)).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = m_Network.post(request, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray body = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QNetworkReply::NetworkError error = reply->error();
        const QString errorText = reply->errorString();
        reply->deleteLater();

        if (status >= 200 && status < 300) {
            const QString text = parseResponse(body);
            if (text.isEmpty())
                emit failed(tr("The service answered without any text."));
            else
                emit answered(text);
        } else if (status > 0) {
            emit failed(parseError(status, body));
        } else {
            // no HTTP answer: offline, DNS, timeout ...
            Q_UNUSED(error);
            emit failed(tr("The service cannot be reached: %1").arg(errorText));
        }
    });
}
