#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "ChatBackend.h"
#include "ConversationListModel.h"
#include "MemberListModel.h"
#include "logos_sdk.h"

class TestChatBackend : public QObject
{
    Q_OBJECT

private slots:
    void showsTheAddressWhileDeliveryIsComingUp();
    void showsTheAddressWhenDeliveryComesUpDuringTheFirstSnapshot();
    void reportsTheNodeDeliveryCameUpOn();
    void readsTheNodeFromStatusWhenAlreadyOnline();
    void marksAConversationFromAPreviousSession();
    void listsAPreviousSessionAfterThisOne();
    void previewsAMultiLineMessageOnOneLine();
    void namesAGroupOnOneLine();
    void reportsAMultiLineFailureOnOneLine();
    void offersToRemoveOnlyAnotherMemberWhoHasJoined_data();
    void offersToRemoveOnlyAnotherMemberWhoHasJoined();
    void removesAMemberThroughTheModule();
    void refusesARemovalItCannotAskFor_data();
    void refusesARemovalItCannotAskFor();
    void reportsTheModulesReasonForRefusingARemoval();
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

void TestChatBackend::offersToRemoveOnlyAnotherMemberWhoHasJoined_data()
{
    QTest::addColumn<QString>("myAddress");
    QTest::addColumn<QList<bool>>("removable");

    // This account, another member, an invite waiting to join, and a member
    // with no account to name.
    QTest::newRow("own address known") << QStringLiteral("4be1c07a9d22") << QList<bool>{ false, true, false, false };
    QTest::newRow("own address unknown") << QString() << QList<bool>{ false, false, false, false };
}

// A roster row is removable, by the name the view binds it by, only for
// another member who has joined and has an account to name, and none is while
// this account's own address is unknown, since any row could then be its own.
void TestChatBackend::offersToRemoveOnlyAnotherMemberWhoHasJoined()
{
    QFETCH(QString, myAddress);
    QFETCH(QList<bool>, removable);

    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");
    modules.chat_module.address = myAddress;
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    modules.chat_module.conversations = { group };
    modules.chat_module.members = {
        { QStringLiteral("4be1c07a9d22"), false },
        { QStringLiteral("0b2c3d4e5f60"), false },
        { QStringLiteral("f6e5d4c3b2a1"), true },
        { QString(), false },
    };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    backend.selectConversation(QStringLiteral("group"));

    const MemberListModel* roster = backend.memberModel();
    QCOMPARE(roster->rowCount(), removable.size());
    const int role = roster->roleNames().key("removable", -1);
    QVERIFY(role != -1);
    for (int row = 0; row < removable.size(); ++row)
        QCOMPARE(roster->data(roster->index(row), role).toBool(), removable.at(row));
}

// A removal is one call to the module and nothing to report: the group votes on
// it first, and members_changed says when the member has left.
void TestChatBackend::removesAMemberThroughTheModule()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QSignalSpy reported(&backend, &ChatBackend::error);
    backend.removeGroupMember(QStringLiteral("group"), QStringLiteral("pax"));

    QCOMPARE(modules.chat_module.removeGroupMemberCalls,
             QList<QStringList>({ { QStringLiteral("group"), QStringLiteral("pax") } }));
    QCOMPARE(reported.count(), 0);
}

void TestChatBackend::refusesARemovalItCannotAskFor_data()
{
    QTest::addColumn<bool>("online");
    QTest::addColumn<QString>("conversationId");
    QTest::addColumn<QString>("address");
    QTest::addColumn<QString>("report");

    QTest::newRow("offline") << false << QStringLiteral("group") << QStringLiteral("pax")
                             << QStringLiteral("Failed to remove member: chat is not online");
    QTest::newRow("no conversation") << true << QString() << QStringLiteral("pax")
                                     << QStringLiteral("Failed to remove member: no conversation selected");
    QTest::newRow("no address") << true << QStringLiteral("group") << QString()
                                << QStringLiteral("Failed to remove member: address cannot be empty");
}

// A removal the view cannot ask for is reported and never reaches the module.
void TestChatBackend::refusesARemovalItCannotAskFor()
{
    QFETCH(bool, online);
    QFETCH(QString, conversationId);
    QFETCH(QString, address);
    QFETCH(QString, report);

    LogosModules modules;
    place(modules);
    if (online)
        modules.chat_module.deliveryState = QStringLiteral("online");

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QSignalSpy reported(&backend, &ChatBackend::error);
    backend.removeGroupMember(conversationId, address);

    QVERIFY(modules.chat_module.removeGroupMemberCalls.isEmpty());
    QCOMPARE(reported.count(), 1);
    QCOMPARE(reported.first().first().toString(), report);
}

// The module refuses a removal it cannot put to the group, and its reason is
// what reaches the status bar.
void TestChatBackend::reportsTheModulesReasonForRefusingARemoval()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");
    modules.chat_module.removeGroupMemberError = QStringLiteral("no one named is a member of this group");

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QSignalSpy reported(&backend, &ChatBackend::error);
    backend.removeGroupMember(QStringLiteral("group"), QStringLiteral("pax"));

    QCOMPARE(modules.chat_module.removeGroupMemberCalls.size(), 1);
    QCOMPARE(reported.count(), 1);
    QCOMPARE(reported.first().first().toString(),
             QStringLiteral("Failed to remove member: no one named is a member of this group"));
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
