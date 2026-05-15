#ifndef CHAT_BACKEND_H
#define CHAT_BACKEND_H

#include <QJsonObject>
#include <QObject>
#include <QString>
#include "rep_ChatBackend_source.h"
#include "logos_api.h"
#include "logos_sdk.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"

class QTimer;

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
    void createConversation(QString introBundle, QString initialMessage) override;
    void requestMyBundle() override;
    void sendMessage(QString conversationId, QString content) override;
    void selectConversation(QString conversationId) override;

private:
    // Honours $CHAT_MODULE_INSTANCE_PATH, otherwise QStandardPaths::AppDataLocation.
    // Creates the directory if missing.
    static QString resolveInstancePath();

    // Honours $CHAT_MODULE_DELIVERY_PORT, otherwise the compiled-in default.
    // Lets multiple instances coexist on one host.
    static int resolveDeliveryPort();

    void initialiseModule();
    void rehydrateConversations();
    void showConversationMessages(const QString& convoId);
    void pollEvents();

    void applyDeliveryState(const QString& state, const QString& detail);
    void applyMessageReceived(const QJsonObject& evt);
    void applyMessageSent(const QJsonObject& evt);
    void applyConversationCreated(const QJsonObject& evt);
    void applyConversationUpdated(const QJsonObject& evt);
    void applyConversationDeleted(const QJsonObject& evt);

    static QString fallbackDisplayName(const QString& convoId, const QString& peerLabel = QString());

    LogosAPI* m_logosAPI;
    LogosModules* m_logos;

    ConversationListModel* m_conversationModel;
    MessageListModel* m_messageModel;

    QTimer* m_eventPollTimer = nullptr;
    QString m_instancePath;
    bool m_moduleInitialised = false;
};

#endif
