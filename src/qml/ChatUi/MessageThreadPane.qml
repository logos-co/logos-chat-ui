pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// The message thread: a header naming the conversation, the message list (newest
// at the bottom, natively bottom-anchored) and the composer. Data in via
// properties, the composed message out via messageSubmitted.
Rectangle {
    id: root

    required property var messageModel
    required property bool currentIsGroup
    required property string title
    required property string conversationId
    required property bool hasConversation
    required property bool online

    signal messageSubmitted(string text)

    // Per-conversation composer drafts, restored on switch so an unsent message
    // is never carried into (or sent to) a different conversation.
    property var _drafts: ({})
    onConversationIdChanged: composer.text = root._drafts[root.conversationId] || ""

    // Exposed for the exchange doc-test's inspector hooks.
    property alias messageCount: threadList.count

    implicitWidth: 400
    color: Theme.palette.background

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PaneHeader {
            Layout.fillWidth: true
            visible: root.hasConversation
            title: root.title
        }

        ListView {
            id: threadList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            reuseItems: true
            model: root.messageModel
            spacing: Theme.spacing.small
            verticalLayoutDirection: ListView.BottomToTop

            header: Item {
                height: Theme.spacing.medium
            }
            footer: Item {
                height: Theme.spacing.medium
            }

            ScrollBar.vertical: LogosScrollBar {}

            delegate: MessageBubble {
                width: ListView.view.width
                groupContext: root.currentIsGroup
            }

            EmptyState {
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.spacing.large
                visible: threadList.count === 0
                text: root.hasConversation ? qsTr("No messages yet") : qsTr("Select a conversation to start chatting")
            }
        }

        SubmitRow {
            id: composer
            Layout.fillWidth: true
            placeholder: qsTr("Type a message...")
            disabledPlaceholder: qsTr("Chat not connected")
            buttonText: qsTr("Send")
            submitEnabled: root.online && root.hasConversation
            onSubmitted: function (text) {
                root.messageSubmitted(text);
            }
            // Persist the in-progress text (and clear it after a send, when the
            // field empties) against the current conversation.
            onTextChanged: if (root.conversationId !== "")
                root._drafts[root.conversationId] = composer.text
        }
    }
}
