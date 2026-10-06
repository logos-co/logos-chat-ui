#ifndef CONVERSATION_LIST_MODEL_H
#define CONVERSATION_LIST_MODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QString>
#include <QVector>

struct ConversationItem {
    QString conversationId;
    QString displayName;
    QString description;
    QDateTime lastActivity;
    int unreadCount = 0;
    bool isGroup = false;
    // Truncated last-message content shown as a list preview.
    QString preview;
    // From a previous session, kept for its history only.
    bool historyOnly = false;
    // A group this account was removed from: it reads back, but nothing can be
    // sent into it.
    bool removed = false;
    // The unread count stands for the invite alone, until a message takes it over.
    bool unreadForInvite = false;
};

class ConversationListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    // The numbers are part of the model's interface, so a new role goes last.
    enum Roles {
        ConversationIdRole = Qt::UserRole + 1,
        // On one line, as PreviewRole.
        DisplayNameRole,
        LastActivityRole,
        UnreadCountRole,
        IsGroupRole,
        // Relative last-activity label ("14:03" today, "Yesterday", else a short
        // date). Computed when the activity changes, so an app left idle keeps
        // yesterday's label until the next activity or a rehydrate; there is no
        // day-tick timer.
        LastActivityDisplayRole,
        // Truncated last-message content for the list preview, on one line: its
        // whitespace, line breaks included, reads as single spaces.
        PreviewRole,
        DescriptionRole,
        // Avatar identity, derived from the conversation id rather than the
        // display name: a direct conversation's name is generated from that id
        // too, and a group's avatar carries a glyph instead of initials.
        AvatarInitialsRole,
        AvatarRampRole,
        // From a previous session, kept for its history only.
        HistoryOnlyRole,
        // A group this account was removed from.
        RemovedRole
    };

    explicit ConversationListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addConversation(const QString& id, const QString& displayName,
                         const QString& description, const QDateTime& lastActivity, bool isGroup,
                         const QString& preview, bool historyOnly, bool removed);
    void updateDisplayName(const QString& id, const QString& displayName);
    void updateDescription(const QString& id, const QString& description);
    void updatePreview(const QString& id, const QString& preview);
    void updateLastActivity(const QString& id, const QDateTime& lastActivity);
    void incrementUnread(const QString& id);
    // Marks a row unread for being new; its first message counts as that one
    // rather than a second.
    void markInvited(const QString& id);
    void clearUnread(const QString& id);
    struct Unread {
        int count = 0;
        bool forInvite = false;
    };
    // Unread counts by conversation id, and their restoration onto a rebuilt
    // list: the module does not track them, so a rebuild would drop them.
    QHash<QString, Unread> unreadCounts() const;
    void restoreUnreadCounts(const QHash<QString, Unread>& counts);
    void removeConversation(const QString& id);
    void clear();
    bool contains(const QString& id) const;

    int indexOf(const QString& id) const;

    // Display name for a conversation id, on one line, or empty if unknown.
    Q_INVOKABLE QString displayNameFor(const QString& id) const;

    // Group description for a conversation id, or empty if unknown or unset.
    Q_INVOKABLE QString descriptionFor(const QString& id) const;

    // Whether a conversation is a group; false for a direct or unknown id.
    Q_INVOKABLE bool isGroupFor(const QString& id) const;

    // Whether a conversation is from a previous session; false for one of
    // this session or an unknown id.
    bool historyOnlyFor(const QString& id) const;

    // Whether this account was removed from a group; false for any other
    // conversation or an unknown id.
    bool removedFor(const QString& id) const;

private:
    // Relative label for the last-activity timestamp (see LastActivityDisplayRole).
    QString formatLastActivity(const QDateTime& lastActivity) const;

    QVector<ConversationItem> m_items;
};

#endif
