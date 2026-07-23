import QtQuick
import QtQuick.Layouts

import Logos.Theme

import ChatUi

// A dev harness that renders the ChatUi panes side by side with mock data and no
// host `logos` context, doubling as a visual standalone-instantiability proof.
// Load it with the design system on the import path, e.g.
//   qml -I src/qml -I <app>/lib tests/storybook/Storybook.qml
Rectangle {
    width: 900
    height: 650
    color: Theme.palette.background

    ListModel {
        id: conversationsMock
        ListElement {
            conversationId: "c1"
            displayName: "Alice"
            isGroup: false
            unreadCount: 0
            lastActivityDisplay: "12:34"
            preview: "Sounds good, talk soon"
        }
        ListElement {
            conversationId: "c2"
            displayName: "Design Team"
            isGroup: true
            unreadCount: 128
            lastActivityDisplay: "Yesterday"
            preview: "Bob: pushed the fix, please review"
        }
        ListElement {
            conversationId: "c3"
            displayName: "Bob"
            isGroup: false
            unreadCount: 3
            lastActivityDisplay: "Mon"
            preview: "Did you get a chance to look?"
        }
    }
    ListModel {
        id: messagesMock
        ListElement {
            sender: "Alice"
            content: "Hey, did you see the new theme?"
            timeDisplay: "12:34"
            isMe: false
            sameSenderAsPrevious: false
            showDaySeparator: true
            dayLabel: "Today"
        }
        ListElement {
            sender: "Me"
            content: "Yes, it looks great"
            timeDisplay: "12:34"
            isMe: true
            sameSenderAsPrevious: false
            showDaySeparator: false
            dayLabel: "Today"
        }
    }
    ListModel {
        id: membersMock
        ListElement {
            address: "0xalice"
            label: "Alice"
            isSelf: true
            pending: false
        }
        ListElement {
            address: "0xbob"
            label: "Bob"
            isSelf: false
            pending: false
        }
        ListElement {
            address: "0xcarol"
            label: "Carol"
            isSelf: false
            pending: true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ConversationsPane {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                conversationModel: conversationsMock
                currentConversationId: "c2"
                online: true
            }
            MessageThreadPane {
                Layout.fillWidth: true
                Layout.fillHeight: true
                messageModel: messagesMock
                currentIsGroup: true
                title: "Design Team"
                conversationId: "c2"
                hasConversation: true
                online: true
            }
            MembersPane {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                memberModel: membersMock
                online: true
            }
        }

        StatusBar {
            Layout.fillWidth: true
            statusMessage: "Connected to network"
            statusLabel: "Online"
            online: true
            identity: "0xdeadbeefcafe0123"
        }
    }
}
