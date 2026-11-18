#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "ChatBackend.h"
#include "ConversationListModel.h"
#include "logos_sdk.h"

class TestChatBackend : public QObject
{
    Q_OBJECT

private slots:
    void showsTheAddressWhileDeliveryIsComingUp();
    void showsTheAddressWhenDeliveryComesUpDuringTheFirstSnapshot();
    void reportsTheNodeDeliveryCameUpOn();
    void readsTheNodeFromStatusWhenAlreadyOnline();
    void asksForThePresetTheEnvironmentNames();
    void marksAConversationFromAPreviousSession();
    void listsAPreviousSessionAfterThisOne();
    void previewsAMultiLineMessageOnOneLine();
    void namesAGroupOnOneLine();
    void showsAGroupsProtocolAndEpoch();
    void marksASentMessageDelivered();
    void listsTheMembersByTheirState();
    void reportsAMultiLineFailureOnOneLine();
    void createsAConversationWhileDeliveryIsOffline();
    void countsANewConversationsFirstMessageOnce();
    void leavesTheModuleRunningWhenTheViewCloses();

private:
    QTemporaryDir m_logs;
    // A chat module whose run log lands in this test's own directory.
    void place(LogosModules& modules) const;
};

void TestChatBackend::place(LogosModules& modules) const
{
    modules.chat_module.logPath = m_logs.filePath(QStringLiteral("chat_module_20260921_120000.log"));
}

void TestChatBackend::showsTheAddressWhileDeliveryIsComingUp()
{
    LogosModules modules;
    place(modules);

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);

    QCOMPARE(backend.chatStatus(), ChatBackendSimpleSource::Initialising);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
}

void TestChatBackend::showsTheAddressWhenDeliveryComesUpDuringTheFirstSnapshot()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.whileListingConversations = [&modules] { modules.chat_module.goOnline(); };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);

    QCOMPARE(backend.chatStatus(), ChatBackendSimpleSource::Online);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
}

void TestChatBackend::reportsTheNodeDeliveryCameUpOn()
{
    LogosModules modules;
    place(modules);

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QCOMPARE(backend.deliveryAdopted(), false);

    modules.chat_module.deliveryAdopted = true;
    modules.chat_module.goOnline();

    QCOMPARE(backend.chatStatus(), ChatBackendSimpleSource::Online);
    QCOMPARE(backend.deliveryAdopted(), true);
    QCOMPARE(backend.deliveryPreset(), QStringLiteral("logos.test"));
}

// A view opened on a module that is already up gets no transition to learn the
// node from.
void TestChatBackend::readsTheNodeFromStatusWhenAlreadyOnline()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");
    modules.chat_module.deliveryAdopted = true;

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);

    QCOMPARE(backend.deliveryAdopted(), true);
}

// Both the request and the card follow LOGOS_CHAT_DELIVERY_PRESET.
void TestChatBackend::asksForThePresetTheEnvironmentNames()
{
    LogosModules modules;
    place(modules);

    qputenv("LOGOS_CHAT_DELIVERY_PRESET", "logos.dev");
    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    qunsetenv("LOGOS_CHAT_DELIVERY_PRESET");

    QCOMPARE(modules.chat_module.initialisedPreset, QStringLiteral("logos.dev"));
    QCOMPARE(backend.deliveryPreset(), QStringLiteral("logos.dev"));
}

// The module keeps earlier sessions' conversations for their history, and the
// view has to tell them from the ones it can send into.
void TestChatBackend::marksAConversationFromAPreviousSession()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation live;
    live.convo_id = QStringLiteral("live");
    live.kind = QStringLiteral("direct");
    live.last_activity_ms = 2000;
    ChatModule::Conversation kept;
    kept.convo_id = QStringLiteral("kept");
    kept.kind = QStringLiteral("group");
    kept.last_activity_ms = 1000;
    kept.history_only = true;
    modules.chat_module.conversations = { live, kept };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);

    const QAbstractItemModel* model = backend.conversationModel();
    QCOMPARE(model->rowCount(), 2);
    const auto historyOnly = [model](int row) {
        return model->data(model->index(row, 0), ConversationListModel::HistoryOnlyRole).toBool();
    };
    QCOMPARE(historyOnly(0), false);
    QCOMPARE(historyOnly(1), true);

    backend.selectConversation(QStringLiteral("kept"));
    QCOMPARE(backend.currentHistoryOnly(), true);
    backend.selectConversation(QStringLiteral("live"));
    QCOMPARE(backend.currentHistoryOnly(), false);
}

// A kept conversation without messages has no activity to sort by, and still
// lists after this session's, under the one heading the view gives them.
void TestChatBackend::listsAPreviousSessionAfterThisOne()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation live;
    live.convo_id = QStringLiteral("live");
    live.kind = QStringLiteral("direct");
    live.last_activity_ms = 2000;
    ChatModule::Conversation kept;
    kept.convo_id = QStringLiteral("kept");
    kept.kind = QStringLiteral("group");
    kept.history_only = true;
    modules.chat_module.conversations = { kept, live };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);

    const QAbstractItemModel* model = backend.conversationModel();
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->data(model->index(0, 0), ConversationListModel::ConversationIdRole).toString(), QStringLiteral("live"));
    QCOMPARE(model->data(model->index(1, 0), ConversationListModel::ConversationIdRole).toString(), QStringLiteral("kept"));
}

// A last message written over several lines previews on one line, so it stays
// inside its row.
void TestChatBackend::previewsAMultiLineMessageOnOneLine()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation convo;
    convo.convo_id = QStringLiteral("multi");
    convo.kind = QStringLiteral("direct");
    convo.preview = QStringLiteral("first\nline 1\r\nline 2\u2028line 3");
    modules.chat_module.conversations = { convo };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);

    const QAbstractItemModel* model = backend.conversationModel();
    QCOMPARE(model->data(model->index(0, 0), ConversationListModel::PreviewRole).toString(),
             QStringLiteral("first line 1 line 2 line 3"));
}

// A group's name comes from whoever created it, and shows on one line wherever
// the view names the conversation.
void TestChatBackend::namesAGroupOnOneLine()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation convo;
    convo.convo_id = QStringLiteral("group");
    convo.kind = QStringLiteral("group");
    convo.name = QStringLiteral("Book\nClub");
    modules.chat_module.conversations = { convo };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    backend.selectConversation(QStringLiteral("group"));

    const QAbstractItemModel* model = backend.conversationModel();
    QCOMPARE(model->data(model->index(0, 0), ConversationListModel::DisplayNameRole).toString(),
             QStringLiteral("Book Club"));
    QCOMPARE(backend.currentDisplayName(), QStringLiteral("Book Club"));
}

// A GroupV3 group names its protocol, and the epoch and fingerprint its members
// on the same state share.
void TestChatBackend::showsAGroupsProtocolAndEpoch()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    group.protocol = QStringLiteral("GroupV3");
    group.epoch = 4;
    group.epoch_fingerprint = QStringLiteral("a2434d2747f3f5");
    ChatModule::Conversation kept;
    kept.convo_id = QStringLiteral("kept");
    kept.kind = QStringLiteral("group");
    kept.history_only = true;
    modules.chat_module.conversations = { group, kept };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);

    backend.selectConversation(QStringLiteral("group"));
    QCOMPARE(backend.currentProtocol(), QStringLiteral("GroupV3"));
    QCOMPARE(backend.currentEpoch(), 4);
    QCOMPARE(backend.currentEpochFingerprint(), QStringLiteral("a2434d2747f3f5"));
    backend.selectConversation(QStringLiteral("kept"));
    QCOMPARE(backend.currentProtocol(), QString());
    QCOMPARE(backend.currentEpoch(), -1);
    QCOMPARE(backend.currentEpochFingerprint(), QString());
}

// A sent message reads back as delivered once another member holds it, whether
// the thread loads it so or the module reports it while the thread is open.
void TestChatBackend::marksASentMessageDelivered()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    modules.chat_module.conversations = { group };
    ChatModule::Message held;
    held.from_self = true;
    held.content = QStringLiteral("held");
    held.timestamp_ms = 1000;
    held.message_id = QStringLiteral("m1");
    held.delivered = true;
    modules.chat_module.messages = { held };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    backend.selectConversation(QStringLiteral("group"));
    const QAbstractItemModel* model = backend.messageModel();
    const auto delivered = [model](int row) {
        return model->data(model->index(row, 0), MessageListModel::DeliveredRole).toBool();
    };
    QCOMPARE(delivered(0), true);

    modules.chat_module.emitEvent(QStringLiteral("message_sent"),
                                  { QStringLiteral("group"), QStringLiteral("sent"), 2000, QStringLiteral("m2") });
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(delivered(0), false);
    modules.chat_module.emitEvent(QStringLiteral("message_delivered"),
                                  { QStringLiteral("elsewhere"), QStringLiteral("m2") });
    QCOMPARE(delivered(0), false);
    modules.chat_module.emitEvent(QStringLiteral("message_delivered"),
                                  { QStringLiteral("group"), QStringLiteral("m2") });
    QCOMPARE(delivered(0), true);
}

// The roster leads with this account and the members on its group state, then
// lists the rest under their state, worst first, with the invited last.
void TestChatBackend::listsTheMembersByTheirState()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    modules.chat_module.conversations = { group };
    modules.chat_module.deliveryState = QStringLiteral("online");
    const auto member = [](const char* address, const char* state, bool pending = false) {
        ChatModule::GroupMember m;
        m.address = QString::fromLatin1(address);
        m.sync_state = QString::fromLatin1(state);
        m.pending = pending;
        return m;
    };
    ChatModule::GroupMember saro = member("saro", "behind");
    saro.epoch = 3;
    saro.epoch_fingerprint = QStringLiteral("5e81c09b2a7d44");
    modules.chat_module.members = {
        member("invited", "", true),
        saro,
        member("unheard", "unconfirmed"),
        member("raya", "synced"),
        member("pax", "forked"),
        member(qPrintable(modules.chat_module.address), "synced"),
        member("early", "ahead"),
    };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    backend.selectConversation(QStringLiteral("group"));

    const QAbstractItemModel* model = backend.memberModel();
    QStringList order;
    QStringList groups;
    for (int row = 0; row < model->rowCount(); ++row) {
        order << model->data(model->index(row, 0), MemberListModel::AddressRole).toString();
        groups << model->data(model->index(row, 0), MemberListModel::SyncGroupRole).toString();
    }
    QCOMPARE(order, QStringList({ modules.chat_module.address, QStringLiteral("raya"), QStringLiteral("pax"),
                                  QStringLiteral("saro"), QStringLiteral("early"), QStringLiteral("unheard"),
                                  QStringLiteral("invited") }));
    QCOMPARE(groups, QStringList({ QString(), QString(), QStringLiteral("forked"), QStringLiteral("behind"),
                                   QStringLiteral("ahead"), QStringLiteral("unconfirmed"),
                                   QStringLiteral("pending") }));
    const QModelIndex saroRow = model->index(3, 0);
    QCOMPARE(model->data(saroRow, MemberListModel::EpochRole).toInt(), 3);
    QCOMPARE(model->data(saroRow, MemberListModel::EpochFingerprintRole).toString(),
             QStringLiteral("5e81c09b2a7d44"));
}

// A module failure can carry a server's response body, and reaches the status
// bar and the Errors tab on one line.
void TestChatBackend::reportsAMultiLineFailureOnOneLine()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");
    modules.chat_module.createConversationError =
        QStringLiteral("server returned status 503: <html>\n<body>busy</body>\n</html>");

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QSignalSpy reported(&backend, &ChatBackend::error);
    backend.createConversation(QStringLiteral("peer"));

    const QString line =
        QStringLiteral("Failed to create DM: server returned status 503: <html> <body>busy</body> </html>");
    QCOMPARE(reported.count(), 1);
    QCOMPARE(reported.first().first().toString(), line);
    QCOMPARE(backend.errors().first().toMap().value(QStringLiteral("message")).toString(), line);
}

// A node with no peers for the moment still takes actions, and is no error.
void TestChatBackend::createsAConversationWhileDeliveryIsOffline()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QSignalSpy reported(&backend, &ChatBackend::error);
    modules.chat_module.goOffline();
    QCOMPARE(backend.chatStatus(), ChatBackendSimpleSource::Offline);

    backend.createConversation(QStringLiteral("peer"));

    QCOMPARE(modules.chat_module.conversationsCreated, 1);
    QCOMPARE(reported.count(), 0);
}

// Being invited marks a new conversation unread, and the message the invite
// carries is that same unread one.
void TestChatBackend::countsANewConversationsFirstMessageOnce()
{
    LogosModules modules;
    place(modules);
    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    const QAbstractItemModel* model = backend.conversationModel();
    const auto unread = [model] {
        return model->data(model->index(0, 0), ConversationListModel::UnreadCountRole).toInt();
    };

    modules.chat_module.emitEvent(QStringLiteral("conversation_created"),
                                  { QStringLiteral("dm"), false, QStringLiteral("peer"), QStringLiteral("direct") });
    QCOMPARE(unread(), 1);
    modules.chat_module.emitEvent(QStringLiteral("message_received"),
                                  { QStringLiteral("dm"), QStringLiteral("hello"), 1000, QStringLiteral("peer") });
    QCOMPARE(unread(), 1);
    modules.chat_module.emitEvent(QStringLiteral("message_received"),
                                  { QStringLiteral("dm"), QStringLiteral("again"), 2000, QStringLiteral("peer") });
    QCOMPARE(unread(), 2);
}

// A host closing the view, as basecamp does with its tab, leaves the module
// running on the same account for the view it opens next.
void TestChatBackend::leavesTheModuleRunningWhenTheViewCloses()
{
    LogosModules modules;
    place(modules);
    {
        ChatBackend backend;
        backend._logosCoreSetLogosModulesPtr_(&modules);
        QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    }

    QCOMPARE(modules.chat_module.get_address(), modules.chat_module.address);
}

QTEST_MAIN(TestChatBackend)
#include "tst_chatbackend.moc"
