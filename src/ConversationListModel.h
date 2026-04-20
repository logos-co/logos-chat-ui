#ifndef CONVERSATION_LIST_MODEL_H
#define CONVERSATION_LIST_MODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>
#include <QVector>

struct ConversationItem {
    QString conversationId;
    QString displayName;
    QString peerId;
    QDateTime lastActivity;
    int unreadCount = 0;
};

class ConversationListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ConversationIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        PeerIdRole,
        LastActivityRole,
        UnreadCountRole
    };

    explicit ConversationListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addConversation(const QString& id, const QString& displayName,
                         const QString& peerId, const QDateTime& lastActivity);
    void updateLastActivity(const QString& id, const QDateTime& lastActivity);
    void incrementUnread(const QString& id);
    void clearUnread(const QString& id);
    bool contains(const QString& id) const;

    int indexOf(const QString& id) const;

private:
    QVector<ConversationItem> m_items;
};

#endif
