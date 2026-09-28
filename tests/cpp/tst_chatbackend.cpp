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
    void marksAConversationFromAPreviousSession();
    void listsAPreviousSessionAfterThisOne();
    void previewsAMultiLineMessageOnOneLine();
    void namesAGroupOnOneLine();
    void reportsAMultiLineFailureOnOneLine();
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
