#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>

#include <memory>

#include "MessageListModel.h"

// The thread pane over the model the backend fills. Opening a conversation
// replaces that model's rows in one reset, which no QML model stands in for.
class TestThreadPane : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void opensAConversationAtItsNewestMessage();

private:
    QQmlEngine m_engine;
};

namespace {

// `count` messages with `peer`, a minute apart and oldest first, as
// get_messages answers.
QVector<MessageItem> messagesWith(const QString& peer, int count)
{
    QVector<MessageItem> items;
    const QDateTime start = QDateTime::currentDateTime().addSecs(-60 * count);
    for (int i = 0; i < count; ++i)
        items.append({peer, QStringLiteral("message %1").arg(i), start.addSecs(60 * i), i % 3 == 0});
    return items;
}

bool atNewest(const QQuickItem* list)
{
    return list->property("atYEnd").toBool();
}

} // namespace

void TestThreadPane::initTestCase()
{
    m_engine.addImportPath(QStringLiteral(CHAT_UI_QML_DIR));
    m_engine.addImportPath(QStringLiteral(DESIGN_SYSTEM_QML_DIR));
}

void TestThreadPane::opensAConversationAtItsNewestMessage()
{
    MessageListModel messages;
    messages.setMessages(messagesWith(QStringLiteral("Raya"), 40));

    QQmlComponent component(&m_engine);
    component.loadFromModule("ChatUi", "MessageThreadPane");
    std::unique_ptr<QObject> created(component.createWithInitialProperties({
        {QStringLiteral("messageModel"), QVariant::fromValue(&messages)},
        {QStringLiteral("currentIsGroup"), false},
        {QStringLiteral("title"), QStringLiteral("Raya")},
        {QStringLiteral("conversationId"), QStringLiteral("c1")},
        {QStringLiteral("hasConversation"), true},
        {QStringLiteral("online"), true},
        {QStringLiteral("ready"), true},
    }));
    auto* pane = qobject_cast<QQuickItem*>(created.get());
    QVERIFY2(pane, qPrintable(component.errorString()));

    QQuickWindow window;
    window.resize(400, 300);
    pane->setParentItem(window.contentItem());
    pane->setSize(window.size());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* list = pane->findChild<QQuickItem*>(QStringLiteral("threadList"));
    QVERIFY(list);
    QTRY_VERIFY2(atNewest(list), "the first conversation opens at its newest message");

    // Messages that arrive while it is open, each on its own turn of the event
    // loop as events do, then a switch to another conversation.
    for (int i = 0; i < 20; ++i) {
        messages.addMessage(QStringLiteral("Raya"), QStringLiteral("live %1").arg(i),
                            QDateTime::currentDateTime().addSecs(i), false);
        QTest::qWait(10);
    }
    QTRY_VERIFY2(atNewest(list), "the newest message stays in view as messages arrive");
    messages.setMessages(messagesWith(QStringLiteral("Saro"), 40));
    QTRY_VERIFY2(atNewest(list), "the next conversation opens at its newest message");

    // Back to the first, from a thread scrolled into its history.
    QVERIFY(QMetaObject::invokeMethod(list, "positionViewAtIndex", Q_ARG(int, 30), Q_ARG(int, 1)));
    QTRY_VERIFY(!atNewest(list));
    messages.setMessages(messagesWith(QStringLiteral("Raya"), 60));
    QTRY_VERIFY2(atNewest(list), "a conversation reopened from history opens at its newest message");
}

QTEST_MAIN(TestThreadPane)
#include "tst_threadpane.moc"
