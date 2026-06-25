#ifndef CHAT_BACKEND_H
#define CHAT_BACKEND_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <functional>
#include "rep_ChatBackend_source.h"
#include "logos_ui_plugin_context.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"

class ChatBackend : public ChatBackendSimpleSource,
                    public LogosUiPluginContext
{
    Q_OBJECT
    Q_PROPERTY(ConversationListModel* conversationModel READ conversationModel CONSTANT)
    Q_PROPERTY(MessageListModel* messageModel READ messageModel CONSTANT)

public:
    explicit ChatBackend(QObject* parent = nullptr);
    ~ChatBackend() override;

    ConversationListModel* conversationModel() const;
    MessageListModel* messageModel() const;

    // Fires once the generated plugin glue has wired modules(); the typed
    // chat_module surface is live, so init + event subscriptions happen here.
    void onContextReady() override;

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
    void subscribeToEvents();
    void rehydrateConversations();
    void showConversationMessages(const QString& convoId);

    // Runs `work` on the next event-loop turn. A module read (list_conversations/
    // get_messages) is a synchronous QtRO call; issuing one from inside a module
    // event callback re-enters the replica's socket-read handler while its read
    // notifier is disabled, so the reply only lands after the call's ~20s timeout
    // and the UI thread stalls. Deferring lets the callback return first.
    void deferToEventLoop(std::function<void()> work);

    // Event handlers. Each receives the event's positional argument list, in
    // the order declared in chat_module.lidl.
    void applyDeliveryState(const QString& state, const QString& detail);
    void applyMessageReceived(const QVariantList& args);
    void applyMessageSent(const QVariantList& args);
    void applyConversationCreated(const QVariantList& args);
    void applyConversationUpdated(const QVariantList& args);
    void applyConversationDeleted(const QVariantList& args);

    static QString fallbackDisplayName(const QString& convoId, const QString& peerLabel = QString());

    ConversationListModel* m_conversationModel;
    MessageListModel* m_messageModel;

    QString m_instancePath;
    bool m_moduleInitialised = false;
    // Set once the initial snapshot has loaded; gates the reconnect resync in
    // applyDeliveryState so it doesn't fire during initial setup.
    bool m_initialSnapshotDone = false;
};

#endif
