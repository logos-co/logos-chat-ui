#include "ConversationListModel.h"

ConversationListModel::ConversationListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ConversationListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant ConversationListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const auto& item = m_items.at(index.row());
    switch (role) {
    case ConversationIdRole: return item.conversationId;
    case DisplayNameRole:    return item.displayName;
    case LastActivityRole:   return item.lastActivity;
    case UnreadCountRole:    return item.unreadCount;
    default:                 return {};
    }
}

QHash<int, QByteArray> ConversationListModel::roleNames() const
{
    return {
        { ConversationIdRole, "conversationId" },
        { DisplayNameRole,    "displayName" },
        { LastActivityRole,   "lastActivity" },
        { UnreadCountRole,    "unreadCount" }
    };
}

void ConversationListModel::addConversation(const QString& id, const QString& displayName,
                                            const QDateTime& lastActivity)
{
    if (contains(id)) return;

    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append({ id, displayName, lastActivity, 0 });
    endInsertRows();
}

void ConversationListModel::updateDisplayName(const QString& id, const QString& displayName)
{
    int idx = indexOf(id);
    if (idx < 0) return;

    if (m_items[idx].displayName == displayName) return;
    m_items[idx].displayName = displayName;
    emit dataChanged(index(idx), index(idx), { DisplayNameRole });
}

void ConversationListModel::updateLastActivity(const QString& id, const QDateTime& lastActivity)
{
    int idx = indexOf(id);
    if (idx < 0) return;

    m_items[idx].lastActivity = lastActivity;
    emit dataChanged(index(idx), index(idx), { LastActivityRole });
}

void ConversationListModel::incrementUnread(const QString& id)
{
    int idx = indexOf(id);
    if (idx < 0) return;

    m_items[idx].unreadCount++;
    emit dataChanged(index(idx), index(idx), { UnreadCountRole });
}

void ConversationListModel::clearUnread(const QString& id)
{
    int idx = indexOf(id);
    if (idx < 0) return;

    if (m_items[idx].unreadCount == 0) return;
    m_items[idx].unreadCount = 0;
    emit dataChanged(index(idx), index(idx), { UnreadCountRole });
}

void ConversationListModel::removeConversation(const QString& id)
{
    int idx = indexOf(id);
    if (idx < 0) return;

    beginRemoveRows(QModelIndex(), idx, idx);
    m_items.removeAt(idx);
    endRemoveRows();
}

void ConversationListModel::clear()
{
    if (m_items.isEmpty()) return;
    beginResetModel();
    m_items.clear();
    endResetModel();
}

bool ConversationListModel::contains(const QString& id) const
{
    return indexOf(id) >= 0;
}

int ConversationListModel::indexOf(const QString& id) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].conversationId == id)
            return i;
    }
    return -1;
}
