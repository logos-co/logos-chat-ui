#ifndef CHAT_BACKEND_H
#define CHAT_BACKEND_H

#include <QObject>
#include <QSortFilterProxyModel>
#include <QString>
#include <QVariantList>
#include <functional>
#include "rep_ChatBackend_source.h"
#include "logos_ui_plugin_context.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"
#include "MemberListModel.h"

class ChatBackend : public ChatBackendSimpleSource,
                    public LogosUiPluginContext
{
    Q_OBJECT
    // The conversation model reaches QML through a recency proxy, so the list is
    // sorted newest-first; the base type is what the host remotes to the replica.
    Q_PROPERTY(QAbstractItemModel* conversationModel READ conversationModel CONSTANT)
    Q_PROPERTY(MessageListModel* messageModel READ messageModel CONSTANT)
    Q_PROPERTY(MemberListModel* memberModel READ memberModel CONSTANT)

public:
    explicit ChatBackend(QObject* parent = nullptr);
    ~ChatBackend() override;

    QAbstractItemModel* conversationModel() const;
    MessageListModel* messageModel() const;
    MemberListModel* memberModel() const;

    // Fires once the generated plugin glue has wired modules(); the typed
    // chat_module surface is live, so init + event subscriptions happen here.
    void onContextReady() override;

public slots:
    void createConversation(QString peerAddress) override;
    void createGroupConversation(QString name, QString description) override;
    void addGroupMember(QString conversationId, QString peerAddress) override;
    void requestMyAddress() override;
    void sendMessage(QString conversationId, QString content) override;
    void selectConversation(QString conversationId) override;
    // Reloads the current group's roster into memberModel; clears it for a
    // direct conversation. A synchronous module read, so never call it from
    // inside a module event callback without deferToEventLoop.
    void refreshMembers() override;

private:
    void initialiseModule();
    void subscribeToEvents();
    void rehydrateConversations();
    // Pushes the current conversation's group flag, display name, and description
    // as backend properties for the QML view to bind — see the .cpp for why the
    // view can't read them off the model directly.
    void syncCurrentConversationMeta();
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
    void applyMembersChanged(const QVariantList& args);
    void applyConversationDeleted(const QVariantList& args);

    static QString fallbackDisplayName(const QString& convoId, const QString& peerLabel = QString(),
                                       bool isGroup = false);

    // Short display form of a message sender; "Peer" when the sender is empty.
    static QString shortSenderLabel(const QString& sender);

    ConversationListModel* m_conversationModel;
    // Sorts m_conversationModel newest-first for the view; the source keeps
    // insertion order and every internal edit stays on the source.
    QSortFilterProxyModel* m_conversationProxy;
    MessageListModel* m_messageModel;
    MemberListModel* m_memberModel;

    // This installation's own address, cached lazily on first roster refresh to
    // mark the self entry.
    QString m_myAddress;
    bool m_moduleInitialised = false;
    // Set once the initial snapshot has loaded; gates the reconnect resync in
    // applyDeliveryState so it doesn't fire during initial setup.
    bool m_initialSnapshotDone = false;
};

#endif
