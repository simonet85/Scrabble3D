#include "coach.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>
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

bool coach::isKnownProvider(const QString &provider)
{
    return provider == QLatin1String(Groq) || provider == QLatin1String(Anthropic) || provider == QLatin1String(Gemini);
}

QString coach::providerName(const QString &provider)
{
    if (provider == QLatin1String(Anthropic))
        return QStringLiteral("Anthropic");
    if (provider == QLatin1String(Gemini))
        return QStringLiteral("Gemini");
    return QStringLiteral("Groq");
}

QString coach::defaultModel(const QString &provider)
{
    if (provider == QLatin1String(Anthropic))
        return QStringLiteral("claude-haiku-5-5");
    if (provider == QLatin1String(Gemini))
        return QStringLiteral("gemini-3.1-flash-lite"); // answers in 2-3 s; the "thinking" models need 20-40 s
    return QStringLiteral("openai/gpt-oss-120b");
}

QString coach::defaultBaseUrl(const QString &provider)
{
    if (provider == QLatin1String(Anthropic))
        return QStringLiteral("https://api.anthropic.com");
    if (provider == QLatin1String(Gemini))
        return QStringLiteral("https://generativelanguage.googleapis.com/v1beta");
    return QStringLiteral("https://api.groq.com/openai/v1");
}

QString coach::provider() const
{
    QSettings s = openSettings(m_sSettingsPath);
    const QString p = s.value("provider").toString().trimmed().toLower();
    return isKnownProvider(p) ? p : QString::fromLatin1(Groq);
}

void coach::setProvider(const QString &provider)
{
    if (!isKnownProvider(provider))
        return;
    QSettings s = openSettings(m_sSettingsPath);
    s.setValue("provider", provider);
    s.sync();
}

// Anthropic keeps the names of the first version of the file (apiKey, model, baseUrl)
QString coach::key(const QString &name, const QString &provider) const
{
    return (provider == QLatin1String(Anthropic)) ? name : provider + QLatin1Char('_') + name;
}

bool coach::hasKey(const QString &provider) const
{
    const QString p = provider.isEmpty() ? this->provider() : provider;
    QSettings s = openSettings(m_sSettingsPath);
    return !s.value(key("apiKey", p)).toString().trimmed().isEmpty();
}

QString coach::model(const QString &provider) const
{
    const QString p = provider.isEmpty() ? this->provider() : provider;
    QSettings s = openSettings(m_sSettingsPath);
    const QString m = s.value(key("model", p)).toString().trimmed();
    return m.isEmpty() ? defaultModel(p) : m;
}

QString coach::baseUrl(const QString &provider) const
{
    const QString p = provider.isEmpty() ? this->provider() : provider;
    QSettings s = openSettings(m_sSettingsPath);
    QString u = s.value(key("baseUrl", p)).toString().trimmed();
    if (u.isEmpty())
        u = defaultBaseUrl(p);
    while (u.endsWith('/'))
        u.chop(1);
    return u;
}

bool coach::hasConsent() const
{
    QSettings s = openSettings(m_sSettingsPath);
    const QString p = provider();
    // the first version stored a single "consent" for Anthropic
    return s.value(p == QLatin1String(Anthropic) ? QStringLiteral("consent") : QStringLiteral("consent_") + p, false).toBool();
}

QString coach::maskedKey(const QString &provider) const
{
    const QString p = provider.isEmpty() ? this->provider() : provider;
    QSettings s = openSettings(m_sSettingsPath);
    const QString k = s.value(key("apiKey", p)).toString().trimmed();
    if (k.isEmpty())
        return QString();
    if (k.length() <= 12)
        return QString(k.length(), QChar('*'));
    return k.left(7) + QStringLiteral("...") + k.right(4);
}

void coach::setKey(const QString &apiKey, const QString &provider)
{
    const QString p = provider.isEmpty() ? this->provider() : provider;
    QSettings s = openSettings(m_sSettingsPath);
    if (apiKey.trimmed().isEmpty())
        s.remove(key("apiKey", p));
    else
        s.setValue(key("apiKey", p), apiKey.trimmed());
    s.sync();
}

void coach::setModel(const QString &model, const QString &provider)
{
    const QString p = provider.isEmpty() ? this->provider() : provider;
    QSettings s = openSettings(m_sSettingsPath);
    s.setValue(key("model", p), model.trimmed());
    s.sync();
}

void coach::setBaseUrl(const QString &baseUrl, const QString &provider)
{
    const QString p = provider.isEmpty() ? this->provider() : provider;
    QSettings s = openSettings(m_sSettingsPath);
    s.setValue(key("baseUrl", p), baseUrl.trimmed());
    s.sync();
}

void coach::setConsent(bool consent)
{
    QSettings s = openSettings(m_sSettingsPath);
    const QString p = provider();
    s.setValue(p == QLatin1String(Anthropic) ? QStringLiteral("consent") : QStringLiteral("consent_") + p, consent);
    s.sync();
}

bool coach::autoAdvice() const
{
    QSettings s = openSettings(m_sSettingsPath);
    return s.value("autoAdvice", false).toBool();
}

void coach::setAutoAdvice(bool on)
{
    QSettings s = openSettings(m_sSettingsPath);
    s.setValue("autoAdvice", on);
    s.sync();
}

QString coach::requestUrl() const
{
    const QString p = provider();
    if (p == QLatin1String(Anthropic))
        return baseUrl() + QStringLiteral("/v1/messages");
    if (p == QLatin1String(Gemini))
        return baseUrl() + QStringLiteral("/models/") + model() + QStringLiteral(":generateContent");
    return baseUrl() + QStringLiteral("/chat/completions");
}

QJsonObject coach::buildRequest(const QString &system, const QString &user, int maxTokens, double temperature) const
{
    QJsonObject body;
    body["model"] = model();
    if (provider() == QLatin1String(Anthropic)) {
        QJsonObject message;
        message["role"] = "user";
        message["content"] = user;
        body["max_tokens"] = maxTokens;
        body["temperature"] = temperature;
        body["system"] = system;
        body["messages"] = QJsonArray{message};
    } else if (provider() == QLatin1String(Gemini)) {
        // the model is in the address, not in the body
        body.remove("model");
        QJsonObject systemPart;
        systemPart["text"] = system;
        QJsonObject systemInstruction;
        systemInstruction["parts"] = QJsonArray{systemPart};
        QJsonObject userPart;
        userPart["text"] = user;
        QJsonObject content;
        content["role"] = "user";
        content["parts"] = QJsonArray{userPart};
        QJsonObject generation;
        generation["temperature"] = temperature;
        generation["maxOutputTokens"] = maxTokens + 800; // the "thinking" of some models counts in the tokens
        body["systemInstruction"] = systemInstruction;
        body["contents"] = QJsonArray{content};
        body["generationConfig"] = generation;
    } else {
        // OpenAI compatible: the instruction is the first message
        QJsonObject sys;
        sys["role"] = "system";
        sys["content"] = system;
        QJsonObject usr;
        usr["role"] = "user";
        usr["content"] = user;
        body["messages"] = QJsonArray{sys, usr};
        body["max_tokens"] = maxTokens + 800; // the reasoning of some models counts in the tokens
        body["temperature"] = temperature;
        if (model().startsWith(QLatin1String("openai/gpt-oss")))
            body["reasoning_effort"] = "low"; // short thinking: faster and cheaper
    }
    return body;
}

QString coach::parseResponse(const QByteArray &body)
{
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject())
        return QString();
    const QJsonObject o = doc.object();

    // Gemini: {"candidates":[{"content":{"parts":[{"text":"..."}]}}]}
    const QJsonArray candidates = o.value("candidates").toArray();
    if (!candidates.isEmpty()) {
        QString text;
        for (const QJsonValue &v : candidates.first().toObject().value("content").toObject().value("parts").toArray()) {
            const QJsonObject part = v.toObject();
            if (!part.value("thought").toBool())
                text += part.value("text").toString();
        }
        return text.trimmed();
    }

    // OpenAI compatible: {"choices":[{"message":{"content":"..."}}]}
    const QJsonArray choices = o.value("choices").toArray();
    if (!choices.isEmpty())
        return choices.first().toObject().value("message").toObject().value("content").toString().trimmed();

    // Anthropic: {"content":[{"type":"text","text":"..."}]}
    QString text;
    for (const QJsonValue &v : o.value("content").toArray()) {
        const QJsonObject block = v.toObject();
        if (block.value("type").toString() == "text")
            text += block.value("text").toString();
    }
    return text.trimmed();
}

QString coach::formatAnswer(const QString &text)
{
    QString html = text.trimmed().toHtmlEscaped();
    // the models like markdown: **bold** becomes bold, lone asterisks and back quotes disappear
    static const QRegularExpression bold(QStringLiteral("\\*\\*(.+?)\\*\\*"));
    html.replace(bold, QStringLiteral("<b>\\1</b>"));
    html.remove(QLatin1Char('*'));
    html.remove(QLatin1Char('`'));
    html.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    html.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    return html;
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

void coach::ask(const QString &system, const QString &user, int maxTokens, double temperature)
{
    if (!hasKey()) {
        emit failed(tr("No API key: enter it in Configuration > AI coach."));
        return;
    }
    QSettings s = openSettings(m_sSettingsPath);
    const QString apiKey = s.value(key("apiKey", provider())).toString().trimmed();

    QNetworkRequest request{QUrl(requestUrl())};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "Scrabble3D");
    if (provider() == QLatin1String(Anthropic)) {
        request.setRawHeader("x-api-key", apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
    } else if (provider() == QLatin1String(Gemini)) {
        request.setRawHeader("x-goog-api-key", apiKey.toUtf8());
    } else {
        request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());
    }
    request.setTransferTimeout(45000);

    const QByteArray payload = QJsonDocument(buildRequest(system, user, maxTokens, temperature)).toJson(QJsonDocument::Compact);
    post(request, payload, 0);
}

bool coach::isRetryable(int httpStatus)
{
    return httpStatus == 429 || httpStatus == 500 || httpStatus == 502 || httpStatus == 503
           || httpStatus == 504 || httpStatus == 529;
}

void coach::post(const QNetworkRequest &request, const QByteArray &payload, int attempt)
{
    QNetworkReply *reply = m_Network.post(request, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply, request, payload, attempt] {
        const QByteArray body = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString errorText = reply->errorString();
        reply->deleteLater();

        if (isRetryable(status) && attempt < 1) {
            // one more try: these errors are usually a short peak of load
            QTimer::singleShot(m_nRetryDelay, this, [this, request, payload, attempt] { post(request, payload, attempt + 1); });
            return;
        }
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
            emit failed(tr("The service cannot be reached: %1").arg(errorText));
        }
    });
}
