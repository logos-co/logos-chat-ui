#ifndef MESSAGE_LIST_MODEL_H
#define MESSAGE_LIST_MODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>
#include <QVector>

struct MessageItem {
    QString sender;
    QString content;
    QDateTime timestamp;
    bool isMe;
};

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SenderRole = Qt::UserRole + 1,
        ContentRole,
        TimestampRole,
        IsMeRole
    };

    explicit MessageListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addMessage(const QString& sender, const QString& content,
                    const QDateTime& timestamp, bool isMe);
    void addMessages(QVector<MessageItem> items);
    void clear();

private:
    QVector<MessageItem> m_items;
};

#endif
