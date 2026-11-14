#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

// Optional AI coach: sends a question about the game to an online AI service and returns the
// answer. Four services are supported: Groq (OpenAI compatible API, default), Gemini (Google), Anthropic and
// OpenCode (OpenAI compatible gateway to many models; Zen or Go address).
// The coach only advises (explains moves, defines words); it never validates a word, the
// dictionary does. The API key is stored in a settings file of the user and nowhere else.
class coach : public QObject
{
    Q_OBJECT
public:
    static constexpr const char *Groq = "groq";
    static constexpr const char *Anthropic = "anthropic";
    static constexpr const char *Gemini = "gemini";
    static constexpr const char *OpenCode = "opencode";

    // settingsPath: ini file holding provider, keys, models, base urls and consent
    explicit coach(const QString &settingsPath, QObject *parent = nullptr);

    // provider in use ("groq" by default)
    QString provider() const;
    void setProvider(const QString &provider);
    static bool isKnownProvider(const QString &provider);
    static QString providerName(const QString &provider); // "Groq", "Gemini", "Anthropic", "OpenCode"
    static QString defaultModel(const QString &provider);
    static QString defaultBaseUrl(const QString &provider);

    // Key, model and address of a provider (the one in use by default)
    bool hasKey(const QString &provider = QString()) const;
    QString model(const QString &provider = QString()) const;
    QString baseUrl(const QString &provider = QString()) const;
    QString maskedKey(const QString &provider = QString()) const; // "gsk_abc...wxyz", "" without a key
    bool hasConsent() const; // agreement to send game data to the service in use

    void setKey(const QString &apiKey, const QString &provider = QString());
    void setModel(const QString &model, const QString &provider = QString());
    void setBaseUrl(const QString &baseUrl, const QString &provider = QString()); // tests, proxies
    void setConsent(bool consent); // for the provider in use
    bool autoAdvice() const;       // the game asks for advice itself at the start of each human turn
    void setAutoAdvice(bool on);

    // Body of a request for the provider in use (public for tests)
    QJsonObject buildRequest(const QString &system, const QString &user, int maxTokens = 700, double temperature = 0.3) const;
    // Address and headers are public for tests as well
    QString requestUrl() const;
    // Text of a successful response, whatever the provider; empty when there is none
    static QString parseResponse(const QByteArray &body);
    // Text of an answer for the message list: HTML escaped, **bold** kept, line breaks as <br>
    static QString formatAnswer(const QString &text);
    // Message of an error response ({"error":{"message":...}}), or a generic text for the HTTP status
    static QString parseError(int httpStatus, const QByteArray &body);

    // Temporary errors of the service (overload, rate limit) are tried a second time after this delay
    void setRetryDelay(int milliseconds) { m_nRetryDelay = milliseconds; }
    static bool isRetryable(int httpStatus);
    void setTimeout(int milliseconds) { m_nTimeout = milliseconds; } // how long to wait for an answer

    // Asynchronous request; answers through answered() or failed()
    void ask(const QString &system, const QString &user, int maxTokens = 700, double temperature = 0.3);

signals:
    void answered(const QString &text);
    void failed(const QString &message);

private:
    QString key(const QString &name, const QString &provider) const; // settings key of a provider
    void post(const QNetworkRequest &request, const QByteArray &payload, int attempt);
    int m_nRetryDelay = 2000;
    QByteArray m_sSession; // identifies this run of the application for OpenCode (header x-opencode-session)
    int m_nTimeout = 45000;
    QString m_sSettingsPath;
    QNetworkAccessManager m_Network;
};
