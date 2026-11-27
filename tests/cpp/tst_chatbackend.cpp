#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "ChatBackend.h"
#include "ConversationListModel.h"
#include "Identity.h"
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
    void showsAGroupsProtocol();
    void marksASentMessageDelivered();
    void startsANewRunForAnotherSenderWithTheSameShortAddress();
    void keepsTheSameWordsFromTwoSenders();
    void titlesADirectConversationAfterItsPeer();
    void titlesADirectConversationWithoutANameByItsPeer();
    void namesTheSendersOfAThread();
    void namesTheRosterAndThisAccount();
    void keepsANameWhenAReadFails();
    void savesThisAccountsName();
    void showsWhyANameWasRefused();
    void reportsAMultiLineFailureOnOneLine();
    void createsAConversationWhileDeliveryIsOffline();
    void addsEveryMemberInOneCall();
    void refusesAnAddNamingNoOne();
    void countsANewConversationsFirstMessageOnce();
    void leavesTheModuleRunningWhenTheViewCloses();
    void showsAGroupsDebugDataThisAccountFirst();
    void readsTheDebugDataAgainOnlyWhileShown();
    void loadsTheConversationSelectedWhileAReloadReadsMessages();
    void loadsTheConversationSelectedWhileAReloadReadsMembers();
    void keepsTheNewerThreadWhenAReloadIsRetried();
    void loadsNothingForAConversationDeletedWhileLoading();
    void listsTheMembersOfTheConversationSelectedDuringTheirRead();
    void keepsTheMembersThatChangedDuringARead();

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

// A GroupV3 group names its protocol; a conversation from a previous session
// may not.
void TestChatBackend::showsAGroupsProtocol()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    group.protocol = QStringLiteral("GroupV3");
    ChatModule::Conversation kept;
    kept.convo_id = QStringLiteral("kept");
    kept.kind = QStringLiteral("group");
    kept.history_only = true;
    modules.chat_module.conversations = { group, kept };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);

    backend.selectConversation(QStringLiteral("group"));
    QCOMPARE(backend.currentProtocol(), QStringLiteral("GroupV3"));
    backend.selectConversation(QStringLiteral("kept"));
    QCOMPARE(backend.currentProtocol(), QString());
}

// A sent message reads back as delivered once another member has received it,
// whether the thread loads it so or the module reports it while the thread is
// open.
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

namespace {

// Two accounts whose addresses share the eight characters the thread shows.
const QString kRaya = QStringLiteral("4be1c07a") + QString(56, QLatin1Char('a'));
const QString kPax = QStringLiteral("4be1c07a") + QString(56, QLatin1Char('b'));

ChatModule::Message received(const QString& sender, const QString& content, qlonglong ms)
{
    ChatModule::Message message;
    message.sender = sender;
    message.content = content;
    message.timestamp_ms = ms;
    return message;
}

} // namespace

void TestChatBackend::startsANewRunForAnotherSenderWithTheSameShortAddress()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    modules.chat_module.conversations = { group };
    modules.chat_module.messages = { received(kRaya, QStringLiteral("hi"), 1000),
                                     received(kPax, QStringLiteral("hey"), 2000) };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    backend.selectConversation(QStringLiteral("group"));
    const QAbstractItemModel* model = backend.messageModel();
    const auto data = [model](int row, int role) { return model->data(model->index(row, 0), role); };
    QCOMPARE(data(0, MessageListModel::SenderRole).toString(), QStringLiteral("4be1c07a"));
    QCOMPARE(data(1, MessageListModel::SenderRole).toString(), QStringLiteral("4be1c07a"));
    QCOMPARE(data(0, MessageListModel::SameSenderAsPreviousRole).toBool(), false);

    modules.chat_module.emitEvent(QStringLiteral("message_received"),
                                  { QStringLiteral("group"), QStringLiteral("again"), 3000, kPax });
    QCOMPARE(data(0, MessageListModel::SameSenderAsPreviousRole).toBool(), true);
}

void TestChatBackend::keepsTheSameWordsFromTwoSenders()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    modules.chat_module.conversations = { group };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    backend.selectConversation(QStringLiteral("group"));
    for (const QString& sender : { kRaya, kPax })
        modules.chat_module.emitEvent(QStringLiteral("message_received"),
                                      { QStringLiteral("group"), QStringLiteral("yes"), 1000, sender });

    QCOMPARE(backend.messageModel()->rowCount(), 2);
}

namespace {

ChatModule::Conversation direct(const QString& id, const QString& peer)
{
    ChatModule::Conversation conversation;
    conversation.convo_id = id;
    conversation.kind = QStringLiteral("direct");
    if (!peer.isEmpty())
        conversation.peer = peer;
    return conversation;
}

} // namespace

// What a direct conversation's row and header show of the person on the other
// side, and that both follow their name when it changes.
void TestChatBackend::titlesADirectConversationAfterItsPeer()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.conversations = { direct(QStringLiteral("dm"), kRaya) };
    modules.chat_module.displayNames.insert(kRaya, QStringLiteral("Raya"));

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    const QAbstractItemModel* model = backend.conversationModel();
    const auto data = [model](int role) { return model->data(model->index(0, 0), role); };
    QTRY_COMPARE_WITH_TIMEOUT(data(ConversationListModel::DisplayNameRole).toString(), QStringLiteral("Raya"), 1000);
    QCOMPARE(data(ConversationListModel::PeerLabelRole).toString(), QStringLiteral("4be1c07a"));
    QCOMPARE(data(ConversationListModel::AvatarInitialsRole).toString(), QStringLiteral("4b"));

    backend.selectConversation(QStringLiteral("dm"));
    QCOMPARE(backend.currentDisplayName(), QStringLiteral("Raya"));
    QCOMPARE(backend.currentPeerAddress(), kRaya);
    QCOMPARE(backend.currentPeerLabel(), QStringLiteral("4be1c07a"));
    QCOMPARE(backend.currentAvatarInitials(), QStringLiteral("4b"));

    modules.chat_module.emitEvent(QStringLiteral("display_name_changed"),
                                  { kRaya, QStringLiteral("Raya\nof the theme") });
    QCOMPARE(data(ConversationListModel::DisplayNameRole).toString(), QStringLiteral("Raya of the theme"));
    QCOMPARE(backend.currentDisplayName(), QStringLiteral("Raya of the theme"));
}

void TestChatBackend::titlesADirectConversationWithoutANameByItsPeer()
{
    LogosModules modules;
    place(modules);
    // An invite whose first message has not arrived has no peer yet.
    ChatModule::Conversation named = direct(QStringLiteral("named"), kRaya);
    named.last_activity_ms = 2000;
    ChatModule::Conversation invite = direct(QStringLiteral("0123456789abcdef"), QString());
    invite.last_activity_ms = 1000;
    modules.chat_module.conversations = { named, invite };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    const QAbstractItemModel* model = backend.conversationModel();
    const auto title = [model](int row) {
        return model->data(model->index(row, 0), ConversationListModel::DisplayNameRole).toString();
    };
    QCOMPARE(title(0), QStringLiteral("4be1c07a"));
    QCOMPARE(title(1), QStringLiteral("DM 01234567"));

    backend.selectConversation(QStringLiteral("0123456789abcdef"));
    QCOMPARE(backend.currentPeerAddress(), QString());
    QCOMPARE(backend.currentPeerLabel(), QString());
}

void TestChatBackend::namesTheSendersOfAThread()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    modules.chat_module.conversations = { group };
    ChatModule::Message mine;
    mine.from_self = true;
    mine.content = QStringLiteral("mine");
    mine.timestamp_ms = 2000;
    modules.chat_module.messages = { received(kRaya, QStringLiteral("named"), 1000),
                                     received(kRaya, QStringLiteral("again"), 1500), mine };
    modules.chat_module.displayNames.insert(kRaya, QStringLiteral("Raya"));
    modules.chat_module.displayNames.insert(kPax, QStringLiteral("Pax"));
    modules.chat_module.displayNames.insert(modules.chat_module.address, QStringLiteral("Saro"));

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    backend.selectConversation(QStringLiteral("group"));
    const QAbstractItemModel* model = backend.messageModel();
    const auto name = [model](int row) {
        return model->data(model->index(row, 0), MessageListModel::SenderNameRole).toString();
    };
    QTRY_COMPARE_WITH_TIMEOUT(name(2), QStringLiteral("Raya"), 1000);
    QCOMPARE(name(1), QStringLiteral("Raya"));
    QCOMPARE(name(0), QString());

    // Someone first heard after the thread opened is looked up then.
    modules.chat_module.emitEvent(QStringLiteral("message_received"),
                                  { QStringLiteral("group"), QStringLiteral("hello"), 3000, kPax });
    QTRY_COMPARE_WITH_TIMEOUT(name(0), QStringLiteral("Pax"), 1000);

    // A name set since arrives as display_name_changed, and every row of
    // theirs takes it.
    modules.chat_module.emitEvent(QStringLiteral("display_name_changed"),
                                  { kRaya, QStringLiteral("Raya of the theme") });
    QCOMPARE(name(2), QStringLiteral("Raya of the theme"));
    QCOMPARE(name(3), QStringLiteral("Raya of the theme"));
}

void TestChatBackend::namesTheRosterAndThisAccount()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    modules.chat_module.conversations = { group };
    modules.chat_module.members = { { modules.chat_module.address, false }, { kRaya, false } };
    modules.chat_module.displayNames.insert(modules.chat_module.address, QStringLiteral("Saro"));
    modules.chat_module.displayNames.insert(kRaya, QStringLiteral("Raya"));

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAccountName(), QStringLiteral("Saro"), 1000);

    backend.selectConversation(QStringLiteral("group"));
    const QAbstractItemModel* model = backend.memberModel();
    const auto name = [model](int row) {
        return model->data(model->index(row, 0), MemberListModel::AccountNameRole).toString();
    };
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(name(0), QStringLiteral("Saro"));
    QTRY_COMPARE_WITH_TIMEOUT(name(1), QStringLiteral("Raya"), 1000);

    modules.chat_module.emitEvent(QStringLiteral("display_name_changed"),
                                  { modules.chat_module.address, QString() });
    QCOMPARE(backend.myAccountName(), QString());
    QCOMPARE(name(0), QString());
}

void TestChatBackend::savesThisAccountsName()
{
    LogosModules modules;
    place(modules);
    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    QSignalSpy saved(&backend, &ChatBackend::myAccountNameSaved);

    backend.saveMyAccountName(QStringLiteral("Saro"));

    QVERIFY(backend.myAccountNameSaving());
    QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 1000);
    QVERIFY(!backend.myAccountNameSaving());
    QCOMPARE(backend.myAccountName(), QStringLiteral("Saro"));
    QCOMPARE(backend.myAccountNameProblem(), QString());
}

// The module's reason stays on show until the next save starts.
void TestChatBackend::showsWhyANameWasRefused()
{
    LogosModules modules;
    place(modules);
    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    QSignalSpy saved(&backend, &ChatBackend::myAccountNameSaved);
    const QString tooLong = QStringLiteral("a display name is limited to 64 bytes of UTF-8, and this one is 66");
    modules.chat_module.setDisplayNameError = tooLong;

    backend.saveMyAccountName(QStringLiteral("é").repeated(33));

    QTRY_COMPARE_WITH_TIMEOUT(backend.myAccountNameProblem(), tooLong, 1000);
    QVERIFY(!backend.myAccountNameSaving());
    QCOMPARE(saved.count(), 0);
    QCOMPARE(backend.myAccountName(), QString());

    modules.chat_module.setDisplayNameError.clear();
    backend.saveMyAccountName(QStringLiteral("Saro"));
    QCOMPARE(backend.myAccountNameProblem(), QString());
    QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 1000);
}

// A read that fails says so, and a name already shown stays rather than reading
// as one the account removed.
void TestChatBackend::keepsANameWhenAReadFails()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.conversations = { direct(QStringLiteral("dm"), kRaya) };
    modules.chat_module.displayNames.insert(kRaya, QStringLiteral("Raya"));

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    backend.selectConversation(QStringLiteral("dm"));
    QTRY_COMPARE_WITH_TIMEOUT(backend.currentDisplayName(), QStringLiteral("Raya"), 1000);

    QSignalSpy reported(&backend, &ChatBackend::error);
    modules.chat_module.displayNameError = "timeout";
    modules.chat_module.emitEvent(QStringLiteral("conversation_updated"), { QStringLiteral("dm") });
    QTRY_VERIFY_WITH_TIMEOUT(reported.count() > 0, 1000);
    QCOMPARE(reported.first().first().toString(), QStringLiteral("Could not read a name: the read failed"));
    QCOMPARE(backend.currentDisplayName(), QStringLiteral("Raya"));
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

// The addresses go to the module together, so they join through one commit.
void TestChatBackend::addsEveryMemberInOneCall()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QSignalSpy reported(&backend, &ChatBackend::error);

    const QStringList addresses{ QStringLiteral("raya-address"), QStringLiteral("pax-address") };
    backend.addGroupMembers(QStringLiteral("group"), addresses);

    QCOMPARE(modules.chat_module.membersAdded, QList<QStringList>{ addresses });
    QCOMPARE(reported.count(), 0);
}

void TestChatBackend::refusesAnAddNamingNoOne()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QSignalSpy reported(&backend, &ChatBackend::error);

    backend.addGroupMembers(QStringLiteral("group"), {});
    backend.addGroupMembers(QStringLiteral("group"), { QStringLiteral("raya-address"), QString() });

    QCOMPARE(modules.chat_module.membersAdded.count(), 0);
    QCOMPARE(reported.count(), 2);
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

// A GroupV3 group's debug data: its epoch, each member named by account with
// this account first, and a count per state in a fixed order.
void TestChatBackend::showsAGroupsDebugDataThisAccountFirst()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    modules.chat_module.conversations = { group };
    const QString raya = QStringLiteral("757f1dbb07648825b8b55ce8d56279bd8d40d42a8e10713eed631b8d48dbb2cc");
    modules.chat_module.debugData = QVariantMap{
        {QStringLiteral("epoch"), 4},
        {QStringLiteral("epoch_fingerprint"), QStringLiteral("a2434d2747f3f5")},
        {QStringLiteral("members"),
         QVariantList{
             QVariantMap{{QStringLiteral("signer"), QStringLiteral("52c9")},
                         {QStringLiteral("account"), raya},
                         {QStringLiteral("sync"), QStringLiteral("behind")},
                         {QStringLiteral("epoch"), 3},
                         {QStringLiteral("epoch_fingerprint"), QStringLiteral("5e81c09b2a7d44")}},
             QVariantMap{{QStringLiteral("signer"), QStringLiteral("34e8")},
                         {QStringLiteral("sync"), QStringLiteral("unconfirmed")}},
             QVariantMap{{QStringLiteral("signer"), QStringLiteral("1233")},
                         {QStringLiteral("account"), modules.chat_module.address},
                         {QStringLiteral("sync"), QStringLiteral("synced")},
                         {QStringLiteral("epoch"), 4},
                         {QStringLiteral("epoch_fingerprint"), QStringLiteral("a2434d2747f3f5")}},
         }},
    };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    backend.selectConversation(QStringLiteral("group"));
    backend.refreshDebugData();

    const QVariantMap data = backend.debugData();
    QCOMPARE(data.value(QStringLiteral("epoch")).toInt(), 4);
    QCOMPARE(data.value(QStringLiteral("epochFingerprint")).toString(), QStringLiteral("a2434d2747f3f5"));
    const QVariantList members = data.value(QStringLiteral("members")).toList();
    QCOMPARE(members.size(), 3);
    const QVariantMap self = members.at(0).toMap();
    QVERIFY(self.value(QStringLiteral("isSelf")).toBool());
    QCOMPARE(self.value(QStringLiteral("label")).toString(), Identity::shortLabel(modules.chat_module.address));
    const QVariantMap behind = members.at(1).toMap();
    QCOMPARE(behind.value(QStringLiteral("label")).toString(), Identity::shortLabel(raya));
    QCOMPARE(behind.value(QStringLiteral("epoch")).toInt(), 3);
    const QVariantMap unheard = members.at(2).toMap();
    QCOMPARE(unheard.value(QStringLiteral("label")).toString(), QStringLiteral("unknown_account"));
    QCOMPARE(unheard.value(QStringLiteral("epoch")).toInt(), -1);

    QStringList counts;
    for (const QVariant& count : data.value(QStringLiteral("counts")).toList())
        counts << QStringLiteral("%1 %2").arg(count.toMap().value(QStringLiteral("count")).toInt())
                      .arg(count.toMap().value(QStringLiteral("sync")).toString());
    QCOMPARE(counts, QStringList({QStringLiteral("1 synced"), QStringLiteral("1 behind"),
                                  QStringLiteral("1 unconfirmed")}));
    QVERIFY(data.value(QStringLiteral("json")).toString().contains(QStringLiteral("\"epoch_fingerprint\"")));
    QVERIFY(!data.value(QStringLiteral("readAt")).toString().isEmpty());
}

// The debug data is read while the tools show it, again on the conversation's
// events, and not at all while they are closed.
void TestChatBackend::readsTheDebugDataAgainOnlyWhileShown()
{
    LogosModules modules;
    place(modules);
    ChatModule::Conversation group;
    group.convo_id = QStringLiteral("group");
    group.kind = QStringLiteral("group");
    ChatModule::Conversation other;
    other.convo_id = QStringLiteral("other");
    other.kind = QStringLiteral("group");
    modules.chat_module.conversations = { group, other };
    modules.chat_module.debugData = QVariantMap{{QStringLiteral("epoch"), 4}};

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    backend.selectConversation(QStringLiteral("group"));
    const QVariantList sent = {QStringLiteral("group"), QStringLiteral("hi"), 1, QStringLiteral("m1")};
    modules.chat_module.emitEvent(QStringLiteral("message_sent"), sent);
    QTest::qWait(50);
    QCOMPARE(modules.chat_module.debugDataReads, 0);

    backend.setDebugDataShown(true);
    QCOMPARE(modules.chat_module.debugDataReads, 1);
    modules.chat_module.emitEvent(QStringLiteral("message_sent"), sent);
    QTRY_COMPARE_WITH_TIMEOUT(modules.chat_module.debugDataReads, 2, 1000);
    modules.chat_module.emitEvent(QStringLiteral("members_changed"), {QStringLiteral("other")});
    QTest::qWait(50);
    QCOMPARE(modules.chat_module.debugDataReads, 2);

    backend.selectConversation(QStringLiteral("other"));
    QCOMPARE(modules.chat_module.debugDataReads, 3);

    backend.setDebugDataShown(false);
    backend.selectConversation(QStringLiteral("group"));
    QVERIFY(backend.debugData().isEmpty());
    QCOMPARE(modules.chat_module.debugDataReads, 3);
}

namespace {

ChatModule::Conversation groupConversation(const QString& id)
{
    ChatModule::Conversation conversation;
    conversation.convo_id = id;
    conversation.kind = QStringLiteral("group");
    return conversation;
}

QString content(const QAbstractItemModel* thread, int row)
{
    return thread->data(thread->index(row, 0), MessageListModel::ContentRole).toString();
}

} // namespace

void TestChatBackend::loadsTheConversationSelectedWhileAReloadReadsMessages()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.conversations = { groupConversation(QStringLiteral("first")),
                                          groupConversation(QStringLiteral("second")) };
    modules.chat_module.messages = { received(kRaya, QStringLiteral("in first"), 1000) };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    backend.selectConversation(QStringLiteral("first"));

    // Delivery comes back, and the second conversation is selected while the
    // reload waits for the first one's messages.
    bool selected = false;
    modules.chat_module.whileReadingMessages = [&] {
        if (selected)
            return;
        selected = true;
        modules.chat_module.messages = { received(kPax, QStringLiteral("in second"), 2000) };
        backend.selectConversation(QStringLiteral("second"));
    };
    modules.chat_module.goOnline();
    QTRY_VERIFY_WITH_TIMEOUT(selected, 1000);

    QCOMPARE(backend.currentConversationId(), QStringLiteral("second"));
    QCOMPARE(backend.loadedConversationId(), QStringLiteral("second"));
    QCOMPARE(backend.messageModel()->rowCount(), 1);
    QCOMPARE(content(backend.messageModel(), 0), QStringLiteral("in second"));
}

void TestChatBackend::loadsTheConversationSelectedWhileAReloadReadsMembers()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.conversations = { groupConversation(QStringLiteral("first")),
                                          groupConversation(QStringLiteral("second")) };
    modules.chat_module.messages = { received(kRaya, QStringLiteral("in first"), 1000) };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    backend.selectConversation(QStringLiteral("first"));

    bool selected = false;
    modules.chat_module.whileListingMembers = [&] {
        if (selected)
            return;
        selected = true;
        modules.chat_module.messages = { received(kPax, QStringLiteral("in second"), 2000) };
        backend.selectConversation(QStringLiteral("second"));
    };
    modules.chat_module.goOnline();
    QTRY_VERIFY_WITH_TIMEOUT(selected, 1000);

    QCOMPARE(backend.currentConversationId(), QStringLiteral("second"));
    QCOMPARE(backend.loadedConversationId(), QStringLiteral("second"));
    QCOMPARE(content(backend.messageModel(), 0), QStringLiteral("in second"));
}

void TestChatBackend::keepsTheNewerThreadWhenAReloadIsRetried()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.conversations = { groupConversation(QStringLiteral("group")) };
    modules.chat_module.messages = { received(kRaya, QStringLiteral("first"), 1000) };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    backend.selectConversation(QStringLiteral("group"));

    // While the reload waits, a message arrives and the conversation is
    // selected again, which reads it with the message.
    bool retried = false;
    modules.chat_module.whileReadingMessages = [&] {
        if (retried)
            return;
        retried = true;
        modules.chat_module.messages = { received(kRaya, QStringLiteral("first"), 1000),
                                         received(kRaya, QStringLiteral("second"), 2000) };
        backend.selectConversation(QStringLiteral("group"));
    };
    modules.chat_module.goOnline();
    QTRY_VERIFY_WITH_TIMEOUT(retried, 1000);

    QCOMPARE(backend.loadedConversationId(), QStringLiteral("group"));
    QCOMPARE(backend.messageModel()->rowCount(), 2);
}

void TestChatBackend::loadsNothingForAConversationDeletedWhileLoading()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.conversations = { groupConversation(QStringLiteral("group")) };
    modules.chat_module.messages = { received(kRaya, QStringLiteral("hi"), 1000) };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    modules.chat_module.whileReadingMessages = [&modules] {
        modules.chat_module.emitEvent(QStringLiteral("conversation_deleted"), { QStringLiteral("group") });
    };
    backend.selectConversation(QStringLiteral("group"));

    QCOMPARE(backend.currentConversationId(), QString());
    QCOMPARE(backend.loadedConversationId(), QString());
    QCOMPARE(backend.messageModel()->rowCount(), 0);
}

void TestChatBackend::listsTheMembersOfTheConversationSelectedDuringTheirRead()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");
    modules.chat_module.conversations = { groupConversation(QStringLiteral("first")),
                                          groupConversation(QStringLiteral("second")) };
    modules.chat_module.members = { { kRaya, false } };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    bool selected = false;
    modules.chat_module.whileListingMembers = [&] {
        if (selected)
            return;
        selected = true;
        modules.chat_module.members = { { kPax, false } };
        backend.selectConversation(QStringLiteral("second"));
    };
    backend.selectConversation(QStringLiteral("first"));

    const QAbstractItemModel* members = backend.memberModel();
    QCOMPARE(members->rowCount(), 1);
    QCOMPARE(members->data(members->index(0, 0), MemberListModel::AddressRole).toString(), kPax);
}

void TestChatBackend::keepsTheMembersThatChangedDuringARead()
{
    LogosModules modules;
    place(modules);
    modules.chat_module.deliveryState = QStringLiteral("online");
    modules.chat_module.conversations = { groupConversation(QStringLiteral("group")) };
    modules.chat_module.members = { { kRaya, false } };

    ChatBackend backend;
    backend._logosCoreSetLogosModulesPtr_(&modules);
    QTRY_COMPARE_WITH_TIMEOUT(backend.myAddress(), modules.chat_module.address, 1000);
    backend.selectConversation(QStringLiteral("group"));
    QCOMPARE(backend.memberModel()->rowCount(), 1);

    // While a refresh waits, Pax joins, and the refetch that change asks for
    // runs in the wait's event loop.
    bool joined = false;
    modules.chat_module.whileListingMembers = [&] {
        if (joined)
            return;
        joined = true;
        modules.chat_module.members = { { kRaya, false }, { kPax, false } };
        modules.chat_module.emitEvent(QStringLiteral("members_changed"), { QStringLiteral("group") });
        QCoreApplication::processEvents();
    };
    modules.chat_module.emitEvent(QStringLiteral("members_changed"), { QStringLiteral("group") });
    QTRY_VERIFY_WITH_TIMEOUT(joined, 1000);

    QCOMPARE(backend.memberModel()->rowCount(), 2);
}

QTEST_MAIN(TestChatBackend)
#include "tst_chatbackend.moc"
