#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

// Optional AI coach: sends a question about the game to the Anthropic Messages API and returns
// the answer. It only advises (explains moves, defines words); it never validates a word, the
// dictionary does. The API key is stored in a settings file of the user and nowhere else.
class coach : public QObject
{
    Q_OBJECT
public:
    static constexpr const char *DefaultModel = "claude-haiku-5-5";
    static constexpr const char *DefaultBaseUrl = "https://api.anthropic.com";

    // settingsPath: ini file holding apiKey, model, baseUrl and consent
    explicit coach(const QString &settingsPath, QObject *parent = nullptr);

    bool hasKey() const;
    QString model() const;
    QString baseUrl() const;
    bool hasConsent() const;
    QString maskedKey() const; // "sk-ant-...abcd" for display, "" without a key

    void setKey(const QString &apiKey);
    void setModel(const QString &model);
    void setBaseUrl(const QString &baseUrl); // tests, proxies
    void setConsent(bool consent);

    // Builds the JSON body of a request (public for tests)
    QJsonObject buildRequest(const QString &system, const QString &user, int maxTokens = 700) const;
    // Extracts the text of a successful response; empty when there is none
    static QString parseResponse(const QByteArray &body);
    // Message of an error response ({"error":{"message":...}}), or a generic text for the HTTP status
    static QString parseError(int httpStatus, const QByteArray &body);

    // Asynchronous request; answers through answered() or failed()
    void ask(const QString &system, const QString &user, int maxTokens = 700);

signals:
    void answered(const QString &text);
    void failed(const QString &message);

private:
    QString m_sSettingsPath;
    QNetworkAccessManager m_Network;
};
