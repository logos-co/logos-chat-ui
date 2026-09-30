#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlListReference>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <memory>

// The whole view on the scene catalog's stand-in for the host's `logos` bridge,
// injected where the host puts it: what ChatView wires between its panes, its
// dialogs and the backend, which no single component carries.
class TestChatView : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void removesTheMemberARowAsksFor();
    void hidesTheRosterOfAGroupThisAccountWasRemovedFrom();
    void marksTheThreadAndDetailsOfAGroupThisAccountWasRemovedFrom();

private:
    // ChatView in a window over groupBridge, in the order that tears the view
    // down first.
    struct Shown {
        std::unique_ptr<QObject> logos;
        std::unique_ptr<QQuickWindow> window;
        std::unique_ptr<QObject> view;
    };
    void show(Shown& shown);

    QQmlEngine m_engine;
};

namespace {

// An online group on screen, with another member and this account on its
// roster, over a backend that tells which removals the view asks for.
const char* const groupBridge = R"(
import QtQuick
import Logos.ChatBackend
import ChatUiScenes

MockLogos {
    backend: ChatBackend {
        signal removalAsked(string conversationId, string address)

        function removeGroupMember(conversationId: string, peerAddress: string) {
            removalAsked(conversationId, peerAddress);
        }

        chatStatus: ChatBackend.Online
        currentConversationId: "group"
        loadedConversationId: "group"
        currentIsGroup: true
        memberCount: 2
    }
    members: ListModel {
        ListElement {
            address: "0b2c3d4e5f60"
            label: "0b2c3d4e"
            avatarInitials: "0b"
            avatarRamp: 1
            isSelf: false
            pending: false
            removable: true
        }
        ListElement {
            address: "4be1c07a9d22"
            label: "4be1c07a"
            avatarInitials: "4b"
            avatarRamp: 0
            isSelf: true
            pending: false
            removable: false
        }
    }
}
)";

} // namespace

void TestChatView::initTestCase()
{
    m_engine.addImportPath(QStringLiteral(SCENE_MOCKS_DIR));
    m_engine.addImportPath(QStringLiteral(CHAT_UI_QML_DIR));
    m_engine.addImportPath(QStringLiteral(DESIGN_SYSTEM_QML_DIR));
}

void TestChatView::show(Shown& shown)
{
    QQmlComponent logos(&m_engine);
    logos.setData(groupBridge, QUrl());
    shown.logos.reset(logos.create());
    QVERIFY2(shown.logos, qPrintable(logos.errorString()));
    m_engine.rootContext()->setContextProperty(QStringLiteral("logos"), shown.logos.get());

    QQmlComponent component(&m_engine, QUrl::fromLocalFile(QStringLiteral(CHAT_UI_QML_DIR "/ChatView.qml")));
    shown.view.reset(component.create());
    auto* view = qobject_cast<QQuickItem*>(shown.view.get());
    QVERIFY2(view, qPrintable(component.errorString()));

    shown.window = std::make_unique<QQuickWindow>();
    shown.window->resize(1280, 800);
    view->setParentItem(shown.window->contentItem());
    view->setSize(shown.window->size());
    shown.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(shown.window.get()));
}

// Remove on a member's row, once confirmed, asks the backend to remove that
// member from the group on screen.
void TestChatView::removesTheMemberARowAsksFor()
{
    Shown shown;
    show(shown);
    if (QTest::currentTestFailed())
        return;
    QSignalSpy asked(shown.logos->property("backend").value<QObject*>(), SIGNAL(removalAsked(QString, QString)));
    QVERIFY(asked.isValid());

    auto* roster = shown.view->findChild<QQuickItem*>(QStringLiteral("memberList"));
    QVERIFY(roster);
    QQuickItem* row = nullptr;
    QTRY_VERIFY(QMetaObject::invokeMethod(roster, "itemAtIndex", Q_RETURN_ARG(QQuickItem*, row), Q_ARG(int, 0)) && row);
    QMetaObject::invokeMethod(row, "contextMenuRequested", Q_ARG(QString, row->property("address").toString()));
    auto* remove = shown.view->findChild<QObject*>(QStringLiteral("removeMemberMenuItem"));
    QVERIFY(remove);
    QMetaObject::invokeMethod(remove, "triggered");

    auto* dialog = shown.view->findChild<QObject*>(QStringLiteral("removeMemberDialog"));
    QVERIFY(dialog);
    QTRY_VERIFY2(dialog->property("opened").toBool(), "Remove asks for a confirmation");
    QCOMPARE(dialog->property("title").toString(), QStringLiteral("Remove 0b2c3d4e?"));
    QCOMPARE(dialog->property("memberCount").toInt(), 2);
    QCOMPARE(asked.count(), 0);

    QMetaObject::invokeMethod(QQmlListReference(dialog, "rightActions").at(0), "clicked");
    QCOMPARE(asked.count(), 1);
    QCOMPARE(asked.first(), (QVariantList{ QStringLiteral("group"), QStringLiteral("0b2c3d4e5f60") }));
}

// The roster change that removes this account can reach the view before the
// mark does, so the roster can still hold rows when the mark lands. The mark
// alone hides the roster, and its faces in the header.
void TestChatView::hidesTheRosterOfAGroupThisAccountWasRemovedFrom()
{
    Shown shown;
    show(shown);
    if (QTest::currentTestFailed())
        return;
    auto* roster = shown.view->findChild<QQuickItem*>(QStringLiteral("memberList"));
    auto* faces = shown.view->findChild<QQuickItem*>(QStringLiteral("facepile"));
    QVERIFY(roster && faces);
    QVERIFY2(roster->isVisible(), "a group's roster shows");
    QVERIFY2(faces->isVisible(), "and its faces in the header");

    shown.logos->property("backend").value<QObject*>()->setProperty("currentRemoved", true);
    QCOMPARE(roster->property("count").toInt(), 2);
    QVERIFY2(!roster->isVisible(), "a group this account was removed from shows no roster");
    QVERIFY2(!faces->isVisible(), "nor its faces");
}

// A group this account was removed from says so where the composer was, and
// its details name the membership instead of counting members.
void TestChatView::marksTheThreadAndDetailsOfAGroupThisAccountWasRemovedFrom()
{
    Shown shown;
    show(shown);
    if (QTest::currentTestFailed())
        return;
    shown.view->setProperty("detailsShown", true);
    auto* composer = shown.view->findChild<QQuickItem*>(QStringLiteral("composer"));
    auto* notice = shown.view->findChild<QQuickItem*>(QStringLiteral("removedNotice"));
    auto* membership = shown.view->findChild<QQuickItem*>(QStringLiteral("membershipRow"));
    auto* members = shown.view->findChild<QQuickItem*>(QStringLiteral("membersRow"));
    QVERIFY(composer && notice && membership && members);
    QVERIFY2(composer->isVisible(), "a live group takes messages");
    QVERIFY2(!notice->property("shown").toBool(), "with no notice");
    QVERIFY2(members->isVisible() && !membership->isVisible(), "and its details count its members");

    shown.logos->property("backend").value<QObject*>()->setProperty("currentRemoved", true);
    QVERIFY2(!composer->isVisible(), "a group this account was removed from takes none");
    QVERIFY2(notice->property("shown").toBool(), "and a notice says why");
    QVERIFY2(membership->isVisible() && !members->isVisible(), "its details name the membership instead");
}

QTEST_MAIN(TestChatView)
#include "tst_chatview.moc"
