#ifndef CHAT_BACKEND_H
#define CHAT_BACKEND_H

#include <QObject>
#include <QMap>
#include <QDateTime>
#include <QTimer>
#include "rep_ChatBackend_source.h"
#include "logos_api.h"
#include "logos_sdk.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"

class ChatBackend : public ChatBackendSimpleSource
{
    Q_OBJECT
    Q_PROPERTY(ConversationListModel* conversationModel READ conversationModel CONSTANT)
    Q_PROPERTY(MessageListModel* messageModel READ messageModel CONSTANT)

public:
    explicit ChatBackend(LogosAPI* logosAPI = nullptr, QObject* parent = nullptr);
    ~ChatBackend() override;

    ConversationListModel* conversationModel() const;
    MessageListModel* messageModel() const;

public slots:
    // .rep slot overrides
    void initChat() override;
    void startChat() override;
    void stopChat() override;
    void createConversation(QString introBundle, QString initialMessage) override;
    void requestMyBundle() override;
    void sendMessage(QString conversationId, QString content) override;
    void selectConversation(QString conversationId) override;
    void setMixMode(bool required) override;

private:
    void setupEventHandlers();
    void showConversationMessages(const QString& conversationId);

    // Event handlers for chat_module responses
    void onChatInitResult(const QVariantList& data);
    void onChatStartResult(const QVariantList& data);
    void onChatStopResult(const QVariantList& data);
    void onChatCreateIntroBundleResult(const QVariantList& data);
    void onChatNewMessage(const QVariantList& data);
    void onChatNewConversation(const QVariantList& data);
    void onChatNewPrivateConversationResult(const QVariantList& data);
    void onChatSendMessageResult(const QVariantList& data);
    void onChatGetIdResult(const QVariantList& data);
    void onChatGetMixStatusResult(const QVariantList& data);

    LogosAPI* m_logosAPI;
    LogosModules* m_logos;

    // Polls chat_module.getMixStatus() while running so the UI can reflect the
    // live mix pool size (and grey out sending while it is below the minimum).
    QTimer* m_mixStatusTimer = nullptr;

    ConversationListModel* m_conversationModel;
    MessageListModel* m_messageModel;

    // Per-conversation message storage
    struct MessageInfo {
        QString sender;
        QString content;
        QDateTime timestamp;
        bool isMe;
    };
    QMap<QString, QList<MessageInfo>> m_messages;
    QMap<QString, QString> m_peerIdentities;

    bool m_pendingBundleRequest = false;
    QString m_pendingInitialMessage;
};

#endif
