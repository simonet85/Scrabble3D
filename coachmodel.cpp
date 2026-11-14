#include "coachmodel.h"

coachmodel::coachmodel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int coachmodel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant coachmodel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();
    const Entry &e = m_entries[index.row()];
    switch (role) {
    case WhenRole: return e.when.toString("hh:mm:ss");
    case ServiceRole: return e.service;
    case ModelRole: return e.model;
    case KindRole: return e.kind;
    case QuestionRole: return e.question;
    case AnswerRole: return e.answer;
    case ElapsedRole: return e.elapsed;
    case StateRole: return int(e.state);
    }
    return QVariant();
}

QHash<int, QByteArray> coachmodel::roleNames() const
{
    return {
        {WhenRole, "when"},
        {ServiceRole, "service"},
        {ModelRole, "model"},
        {KindRole, "kind"},
        {QuestionRole, "question"},
        {AnswerRole, "answer"},
        {ElapsedRole, "elapsed"},
        {StateRole, "state"},
    };
}

int coachmodel::addPending(const QString &service, const QString &model, const QString &kind, const QString &question)
{
    beginInsertRows(QModelIndex(), 0, 0); // newest first
    Entry e;
    e.id = m_nNextId++;
    e.when = QDateTime::currentDateTime();
    e.service = service;
    e.model = model;
    e.kind = kind;
    e.question = question;
    m_entries.prepend(e);
    endInsertRows();
    emit countChanged();
    return e.id;
}

int coachmodel::rowOfId(int id) const
{
    for (int r = 0; r < m_entries.size(); r++)
        if (m_entries[r].id == id)
            return r;
    return -1;
}

void coachmodel::update(int row, const QList<int> &roles)
{
    const QModelIndex i = index(row);
    emit dataChanged(i, i, roles);
}

void coachmodel::setDone(int id, const QString &answerHtml, int elapsedMs)
{
    const int row = rowOfId(id);
    if (row < 0)
        return;
    m_entries[row].answer = answerHtml;
    m_entries[row].elapsed = elapsedMs;
    m_entries[row].state = Done;
    update(row, {AnswerRole, ElapsedRole, StateRole});
}

void coachmodel::setFailed(int id, const QString &message, int elapsedMs)
{
    const int row = rowOfId(id);
    if (row < 0)
        return;
    m_entries[row].answer = message.toHtmlEscaped();
    m_entries[row].elapsed = elapsedMs;
    m_entries[row].state = Failed;
    update(row, {AnswerRole, ElapsedRole, StateRole});
}

void coachmodel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}
