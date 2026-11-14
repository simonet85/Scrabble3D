#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>

// History of the interventions of the AI coach, shown in the "Coach" tab: what was asked, which
// service answered, how long it took and what it said.
class coachmodel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum State { Pending = 0, Done = 1, Failed = 2 };
    enum Roles {
        WhenRole = Qt::UserRole + 1, // "hh:mm:ss"
        ServiceRole,                 // "Groq", "Gemini", "Anthropic"
        ModelRole,
        KindRole,                    // "advice", "auto", "define"
        QuestionRole,                // short description of what was asked
        AnswerRole,                  // html (or the error message when Failed)
        ElapsedRole,                 // milliseconds, -1 while pending
        StateRole
    };

    explicit coachmodel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return int(m_entries.size()); }

    int addPending(const QString &service, const QString &model, const QString &kind, const QString &question);
    void setDone(int row, const QString &answerHtml, int elapsedMs);
    void setFailed(int row, const QString &message, int elapsedMs);
    Q_INVOKABLE void clear();

signals:
    void countChanged();

private:
    struct Entry {
        QDateTime when;
        QString service, model, kind, question, answer;
        int elapsed = -1;
        State state = Pending;
    };
    void update(int row, const QList<int> &roles);
    QList<Entry> m_entries;
};
