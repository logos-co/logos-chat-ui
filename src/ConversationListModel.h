#ifndef CONVERSATION_LIST_MODEL_H
#define CONVERSATION_LIST_MODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>
#include <QVector>

struct ConversationItem {
    QString conversationId;
    QString displayName;
    QString description;
    QDateTime lastActivity;
    int unreadCount = 0;
    bool isGroup = false;
};

class ConversationListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ConversationIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        LastActivityRole,
        UnreadCountRole,
        IsGroupRole,
        // Relative last-activity label ("14:03" today, "Yesterday", else a short
        // date). Computed when the activity changes, so an app left idle keeps
        // yesterday's label until the next activity or a rehydrate; there is no
        // day-tick timer.
        LastActivityDisplayRole
    };

    explicit ConversationListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addConversation(const QString& id, const QString& displayName,
                         const QString& description, const QDateTime& lastActivity, bool isGroup);
    void updateDisplayName(const QString& id, const QString& displayName);
    void updateDescription(const QString& id, const QString& description);
    void updateLastActivity(const QString& id, const QDateTime& lastActivity);
    void incrementUnread(const QString& id);
    void clearUnread(const QString& id);
    void removeConversation(const QString& id);
    void clear();
    bool contains(const QString& id) const;

    int indexOf(const QString& id) const;

    // Display name for a conversation id, or empty if unknown.
    Q_INVOKABLE QString displayNameFor(const QString& id) const;

    // Group description for a conversation id, or empty if unknown or unset.
    Q_INVOKABLE QString descriptionFor(const QString& id) const;

    // Whether a conversation is a group; false for a direct or unknown id.
    Q_INVOKABLE bool isGroupFor(const QString& id) const;

private:
    // Relative label for the last-activity timestamp (see LastActivityDisplayRole).
    QString formatLastActivity(const QDateTime& lastActivity) const;

    QVector<ConversationItem> m_items;
};

#endif
