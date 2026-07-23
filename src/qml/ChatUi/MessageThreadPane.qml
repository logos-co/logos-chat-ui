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
    // Group description shown under the title; empty for direct conversations
    // and unnamed groups.
    property string description: ""
    required property string conversationId
    required property bool hasConversation
    // Whether any conversation exists at all. Optional so the pane stands alone;
    // drives the empty-thread guidance when nothing is selected.
    property bool hasConversations: true
    required property bool online
    // Whether the message model already holds this conversation's messages. It
    // does not for as long as a selection is being loaded, and the rows standing
    // in the model meanwhile are the ones left behind by the previous
    // conversation.
    required property bool ready

    signal messageSubmitted(string text)
    // Requests the group-details dialog; emitted from the header's Details button.
    signal groupInfoRequested

    // Whether the thread may render its rows.
    readonly property bool threadReady: root.hasConversation && root.ready

    // Per-conversation composer drafts, restored on switch so an unsent message
    // is never carried into (or sent to) a different conversation.
    property var _drafts: ({})
    onConversationIdChanged: composer.text = root._drafts[root.conversationId] || ""
    // Any wait for the thread gets the grace before a placeholder, and any
    // arrival gets the settle window, whether it came from a switch or from the
    // models being refetched under the same conversation.
    onThreadReadyChanged: {
        if (root.threadReady)
            settleTimer.restart();
        else
            graceTimer.restart();
    }

    // Put a message the backend refused back in the composer, unless the user
    // has already started typing the next one.
    function restoreFailedSend(conversationId, content) {
        if (conversationId === root.conversationId && composer.text === "")
            composer.text = content;
    }

    // Exposed for the exchange doc-test's inspector hooks.
    property alias messageCount: threadList.count

    implicitWidth: 400
    color: Theme.palette.background

    // A switch that resolves quickly should not flash a placeholder, so the
    // thread stays blank until this elapses.
    Timer {
        id: graceTimer
        interval: 150
    }

    // The message model is a separate replica from the properties carrying the
    // selection, so its rows can land a moment after the conversation counts as
    // loaded. Wait that out before calling a thread empty.
    Timer {
        id: settleTimer
        interval: 300
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PaneHeader {
            Layout.fillWidth: true
            visible: root.hasConversation
            title: root.title
            subtitle: root.description

            LogosButton {
                objectName: "detailsButton"
                implicitWidth: 84
                implicitHeight: 30
                //: Button that opens the group's details dialog
                text: qsTr("Details")
                visible: root.currentIsGroup
                onClicked: root.groupInfoRequested()
                Layout.alignment: Qt.AlignVCenter
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: threadList
                objectName: "threadList"
                anchors.fill: parent
                clip: true
                reuseItems: true
                model: root.messageModel
                spacing: Theme.spacing.small
                verticalLayoutDirection: ListView.BottomToTop
                visible: root.threadReady

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
            }

            MessageSkeleton {
                objectName: "threadSkeleton"
                anchors.fill: parent
                visible: root.hasConversation && !root.ready && !graceTimer.running
            }

            EmptyState {
                objectName: "threadEmptyState"
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.spacing.large
                visible: threadList.count === 0 && (!root.hasConversation || (root.threadReady && !settleTimer.running))
                text: root.hasConversation ? qsTr("No messages yet") : root.hasConversations ? qsTr("Select a conversation to start chatting") : qsTr("No conversations yet. Use New in the sidebar to start one.")
            }
        }

        MessageComposer {
            id: composer
            objectName: "composer"
            Layout.fillWidth: true
            placeholder: qsTr("Type a message...")
            // Name the reason the composer is closed: being connected with
            // nothing selected is not the same as having no connection.
            disabledPlaceholder: root.online ? qsTr("Select a conversation to start chatting") : qsTr("Chat not connected")
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
