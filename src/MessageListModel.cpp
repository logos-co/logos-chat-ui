#include "MessageListModel.h"

MessageListModel::MessageListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int MessageListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant MessageListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const auto& item = m_items.at(index.row());
    switch (role) {
    case SenderRole:    return item.sender;
    case ContentRole:   return item.content;
    case TimestampRole: return item.timestamp;
    case IsMeRole:      return item.isMe;
    default:            return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const
{
    return {
        { SenderRole,    "sender" },
        { ContentRole,   "content" },
        { TimestampRole, "timestamp" },
        { IsMeRole,      "isMe" }
    };
}

void MessageListModel::addMessage(const QString& sender, const QString& content,
                                  const QDateTime& timestamp, bool isMe)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append({ sender, content, timestamp, isMe });
    endInsertRows();
}

void MessageListModel::addMessages(QVector<MessageItem> items)
{
    const int n = items.size();
    if (n == 0) return;

    const int firstRow = m_items.size();
    beginInsertRows(QModelIndex(), firstRow, firstRow + n - 1);
    m_items += std::move(items);
    endInsertRows();
}

void MessageListModel::clear()
{
    if (m_items.isEmpty()) return;
    beginResetModel();
    m_items.clear();
    endResetModel();
}
