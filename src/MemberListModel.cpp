#include "MemberListModel.h"

MemberListModel::MemberListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int MemberListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant MemberListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const auto& item = m_items.at(index.row());
    switch (role) {
    case AddressRole: return item.address;
    // Short identity string, mirroring ChatBackend::fallbackDisplayName's left(8);
    // a member with no confirmed account (empty address) shows as unknown_account.
    case LabelRole:   return item.address.isEmpty()
                          ? QStringLiteral("unknown_account")
                          : item.address.left(8);
    case IsSelfRole:  return item.isSelf;
    case PendingRole: return item.pending;
    default:          return {};
    }
}

QHash<int, QByteArray> MemberListModel::roleNames() const
{
    return {
        { AddressRole, "address" },
        { LabelRole,   "label" },
        { IsSelfRole,  "isSelf" },
        { PendingRole, "pending" }
    };
}

void MemberListModel::setMembers(const QVector<MemberItem>& members)
{
    beginResetModel();
    m_items = members;
    endResetModel();
}

void MemberListModel::clear()
{
    if (m_items.isEmpty()) return;
    beginResetModel();
    m_items.clear();
    endResetModel();
}

bool MemberListModel::contains(const QString& address) const
{
    for (const auto& item : m_items) {
        if (!item.pending && item.address == address)
            return true;
    }
    return false;
}
