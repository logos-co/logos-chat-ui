#include "MemberListModel.h"
#include "Identity.h"
#include "NameBook.h"

MemberListModel::MemberListModel(const NameBook& names, QObject* parent)
    : QAbstractListModel(parent)
    , m_names(names)
{
    connect(&names, &NameBook::changed, this, &MemberListModel::nameChanged);
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
    // A member with no confirmed account (empty address) has no identity to
    // shorten, so it is named as such.
    case LabelRole:   return item.address.isEmpty()
                          ? QStringLiteral("unknown_account")
                          : Identity::shortLabel(item.address);
    case IsSelfRole:  return item.isSelf;
    case PendingRole: return item.pending;
    case AvatarInitialsRole: return Identity::initials(item.address);
    case AvatarRampRole:     return Identity::avatarRamp(Identity::shortLabel(item.address));
    case AccountNameRole:    return m_names.name(item.address);
    default:          return {};
    }
}

QHash<int, QByteArray> MemberListModel::roleNames() const
{
    return {
        { AddressRole, "address" },
        { LabelRole,   "label" },
        { IsSelfRole,  "isSelf" },
        { PendingRole, "pending" },
        { AvatarInitialsRole, "avatarInitials" },
        { AvatarRampRole,     "avatarRamp" },
        { AccountNameRole,    "accountName" }
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

void MemberListModel::nameChanged(const QString& address)
{
    for (int row = 0; row < m_items.size(); ++row) {
        if (m_items.at(row).address == address)
            emit dataChanged(index(row), index(row), { AccountNameRole });
    }
}

bool MemberListModel::contains(const QString& address) const
{
    for (const auto& item : m_items) {
        if (!item.pending && item.address == address)
            return true;
    }
    return false;
}
