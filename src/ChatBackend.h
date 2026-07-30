#ifndef CHAT_BACKEND_H
#define CHAT_BACKEND_H

#include <QObject>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <functional>
#include "rep_ChatBackend_source.h"
#include "logos_ui_plugin_context.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"
#include "MemberListModel.h"
#include "ErrorLog.h"
#include "SessionLogFiles.h"

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
    void sendMessage(QString conversationId, QString content) override;
    void selectConversation(QString conversationId) override;
    // Reloads the current conversation's roster into memberModel. A synchronous
    // module read, so never call it from inside a module event callback without
    // deferToEventLoop.
    void refreshMembers() override;
    void refreshSessionLogs() override;

private:
    void initialiseModule();
    // Opens this run's logs: the chat module names the file it is writing, and
    // its directory is where this view writes beside it, for want of one of its
    // own. Reports rather than falls back when there is nowhere to write.
    void openRunLogs();
    // The one way a failure reaches anyone: it joins the retained list, goes into
    // this view's log, and reaches the strip. Every failing path calls this and
    // none of them classifies what it is reporting.
    void report(const QString& message);
    // A failed action and the module's account of it. An empty reason gets words
    // of its own rather than a dangling separator: it is what a call that never
    // reached the module leaves behind, which is the worst moment to say least.
    void reportFailure(const QString& action, const QString& reason);
    // Starts asking the module whether it is still there. Until something asks,
    // a module that died is indistinguishable from an idle one, and the app goes
    // on looking connected until the next thing the user does times out.
    void startHealthProbe();
    // One probe's answer. False is the call failing, not the module saying so:
    // health() has no other return.
    void onHealthAnswer(bool answered);
    void subscribeToEvents();
    void rehydrateConversations();
    // Reads this account's own address into the myAddress property. A
    // synchronous module read, so never call it from inside a module event
    // callback without deferToEventLoop.
    void refreshMyAddress();
    // Pushes the current conversation's group flag, display name, and description
    // as backend properties for the QML view to bind — see the .cpp for why the
    // view can't read them off the model directly.
    void syncCurrentConversationMeta();
    // Loads a conversation's messages into messageModel. False when the module
    // could not be read, leaving the model as it was.
    bool showConversationMessages(const QString& convoId);

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

    // The other participant in a direct conversation's roster; empty for a group
    // or when no other account is on it.
    static QString peerAddressOf(const QVector<MemberItem>& members, bool isGroup);

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

    bool m_moduleInitialised = false;
    // Set once the initial snapshot has loaded; gates the reconnect resync in
    // applyDeliveryState so it doesn't fire during initial setup.
    bool m_initialSnapshotDone = false;

    ErrorLog m_errors;
    // The file each writer announced it is writing. Each fixes the naming its own
    // runs are grouped by, and they share one directory.
    QString m_moduleLogPath;
    QString m_viewLogPath;

    QTimer* m_healthProbe = nullptr;
    // Consecutive unanswered probes. Reset by any answer, so what it counts is a
    // module that has stopped answering rather than one slow reply.
    int m_healthMisses = 0;
};

#endif
