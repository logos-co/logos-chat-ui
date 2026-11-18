#ifndef MEMBER_LIST_MODEL_H
#define MEMBER_LIST_MODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct MemberItem {
    // The member's verified account address, empty when no account is confirmed
    // — used for the tooltip and identity comparisons.
    QString address;
    // True for this installation's own entry in the roster.
    bool isSelf = false;
    // A member invited but not yet committed into the group.
    bool pending = false;
    // How the member's group state compares with this account's, as the chat
    // module words it ("synced", "behind", "ahead", "forked", "unconfirmed");
    // empty where it is not tracked.
    QString syncState;
    // The newest epoch the member was heard at, and their fingerprint of it;
    // -1 and empty until then.
    int epoch = -1;
    QString epochFingerprint;
};

// A group's roster, replaced wholesale on each refresh. `label` is the short
// form of `address` (or "unknown_account" when the address is empty), computed
// here so QML renders a consistent identity string. Rows are ordered by
// `syncGroup`, each group in the order given.
class MemberListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        AddressRole = Qt::UserRole + 1,
        LabelRole,
        IsSelfRole,
        PendingRole,
        // Avatar identity, derived from the address so one account keeps one
        // pair of initials and one colour wherever it is drawn.
        AvatarInitialsRole,
        AvatarRampRole,
        SyncStateRole,
        EpochRole,
        EpochFingerprintRole,
        // The heading a row is listed under, in this order: "" for this
        // account, the members on its group state and an untracked roster,
        // then "forked", "behind", "ahead", "unconfirmed" and "pending".
        SyncGroupRole
    };

    explicit MemberListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMembers(const QVector<MemberItem>& members);
    void clear();
    // True when `address` is a committed member. A pending invite does not
    // count: it cannot take part in the conversation until the group commits it.
    bool contains(const QString& address) const;

private:
    QVector<MemberItem> m_items;
};

#endif
