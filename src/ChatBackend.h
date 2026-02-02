#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_sdk.h"

class ChatBackend : public QObject {
    Q_OBJECT

public:
    enum ChatStatus {
        NotInitialized = 0,
        Initializing,
        Ready,
        Error
    };
    Q_ENUM(ChatStatus)

    Q_PROPERTY(ChatStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString currentChannel READ currentChannel NOTIFY currentChannelChanged)
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)

    explicit ChatBackend(LogosAPI* logosAPI = nullptr, QObject* parent = nullptr);
    ~ChatBackend();

    ChatStatus status() const { return m_status; }
    QString currentChannel() const { return m_currentChannel; }
    QString username() const { return m_username; }
    QVariantList messages() const { return m_messages; }

public slots:
    Q_INVOKABLE void joinChannel(const QString& channelName);
    Q_INVOKABLE void sendMessage(const QString& message);
    Q_INVOKABLE void clearMessages();

signals:
    void statusChanged();
    void currentChannelChanged();
    void usernameChanged();
    void messagesChanged();

private slots:
    void onChatMessage(const QVariantList& data);
    void onHistoryMessage(const QVariantList& data);

private:
    void setStatus(ChatStatus newStatus);
    void setCurrentChannel(const QString& channel);
    void initializeChat();
    void addMessage(const QString& sender, const QString& message, bool isHistory = false);

    ChatStatus m_status;
    QString m_currentChannel;
    QString m_username;
    QVariantList m_messages;
    
    LogosAPI* m_logosAPI;
    LogosModules* m_logos;
};
