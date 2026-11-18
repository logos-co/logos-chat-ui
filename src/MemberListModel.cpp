#include "MemberListModel.h"
#include "Identity.h"

#include <QStringList>
#include <algorithm>

namespace {

QString syncGroup(const MemberItem& item)
{
    if (item.pending) return QStringLiteral("pending");
    if (item.isSelf || item.syncState == QLatin1String("synced")) return QString();
    return item.syncState;
}

// Where a row lists: this account first, then the rest of the top group, then
// each heading in turn. A state this view does not know lists as unconfirmed.
int rank(const MemberItem& item)
{
    static const QStringList order = { QString(), QStringLiteral("forked"), QStringLiteral("behind"),
                                       QStringLiteral("ahead"), QStringLiteral("unconfirmed"),
                                       QStringLiteral("pending") };
    if (item.isSelf && !item.pending) return 0;
    const int group = order.indexOf(syncGroup(item));
    return 1 + (group < 0 ? order.indexOf(QStringLiteral("unconfirmed")) : group);
}

} // namespace

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
    // A member with no confirmed account (empty address) has no identity to
    // shorten, so it is named as such.
    case LabelRole:   return item.address.isEmpty()
                          ? QStringLiteral("unknown_account")
                          : Identity::shortLabel(item.address);
    case IsSelfRole:  return item.isSelf;
    case PendingRole: return item.pending;
    case AvatarInitialsRole: return Identity::initials(item.address);
    case AvatarRampRole:     return Identity::avatarRamp(Identity::shortLabel(item.address));
    case SyncStateRole:      return item.syncState;
    case EpochRole:          return item.epoch;
    case EpochFingerprintRole: return item.epochFingerprint;
    case SyncGroupRole:      return syncGroup(item);
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
        { SyncStateRole,      "syncState" },
        { EpochRole,          "memberEpoch" },
        { EpochFingerprintRole, "memberFingerprint" },
        { SyncGroupRole,      "syncGroup" }
    };
}

void MemberListModel::setMembers(const QVector<MemberItem>& members)
{
    beginResetModel();
    m_items = members;
    std::stable_sort(m_items.begin(), m_items.end(),
                     [](const MemberItem& a, const MemberItem& b) { return rank(a) < rank(b); });
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
