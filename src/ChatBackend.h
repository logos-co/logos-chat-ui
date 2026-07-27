#ifndef CHAT_BACKEND_H
#define CHAT_BACKEND_H

#include <QElapsedTimer>
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
#include "LogFilterModel.h"
#include "SessionLogModel.h"
#include "SessionLogWriter.h"

class ChatBackend : public ChatBackendSimpleSource,
                    public LogosUiPluginContext
{
    Q_OBJECT
    // The conversation model reaches QML through a recency proxy, so the list is
    // sorted newest-first; the base type is what the host remotes to the replica.
    Q_PROPERTY(QAbstractItemModel* conversationModel READ conversationModel CONSTANT)
    Q_PROPERTY(MessageListModel* messageModel READ messageModel CONSTANT)
    Q_PROPERTY(MemberListModel* memberModel READ memberModel CONSTANT)
    // The session log through the console's filters; the unfiltered lines stay
    // here, so only the view the reader asked for crosses to QML.
    Q_PROPERTY(QAbstractItemModel* logModel READ logModel CONSTANT)

public:
    explicit ChatBackend(QObject* parent = nullptr);
    ~ChatBackend() override;

    QAbstractItemModel* conversationModel() const;
    MessageListModel* messageModel() const;
    MemberListModel* memberModel() const;
    QAbstractItemModel* logModel() const;

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
    void setLogFilter(QString text) override;
    void setLogLevelEnabled(int level, bool enabled) override;
    void setLogDomainEnabled(QString domain, bool enabled) override;
    void exportSessionLog() override;
    void exportFilteredLog() override;

private:
    // Reports a failure to the view and records it in the session log. A reason
    // the module could not supply is left off rather than trailing a colon.
    void reportFailure(const QString& what, const QString& reason = QString());
    // Follows and appends to the file the host assigned.
    void openSessionLog();
    void republishLogState();
    // Copies the session file, keeping only the lines the filters show when
    // `filtered`. The file is read rather than the model, which retains less.
    void exportLog(bool filtered);

    void initialiseModule();
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
    SessionLogModel* m_sessionLogModel;
    LogFilterModel* m_logFilter;
    SessionLogWriter m_sessionLogWriter;
    // Since the status bar's resting line last moved.
    QElapsedTimer m_restingLineAge;

    bool m_moduleInitialised = false;
    // Set once the initial snapshot has loaded; gates the reconnect resync in
    // applyDeliveryState so it doesn't fire during initial setup.
    bool m_initialSnapshotDone = false;
};

#endif
