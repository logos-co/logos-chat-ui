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
            avatarInitials: "c1"
            avatarRamp: 0
            unreadCount: 0
            lastActivityDisplay: "12:34"
            preview: "Sounds good, talk soon"
            description: ""
        }
        ListElement {
            conversationId: "c2"
            displayName: "Design Team"
            isGroup: true
            avatarInitials: "c2"
            avatarRamp: 3
            unreadCount: 128
            lastActivityDisplay: "Yesterday"
            preview: "Bob: pushed the fix, please review"
            description: "Design reviews and theme work"
        }
        ListElement {
            conversationId: "c3"
            displayName: "Bob"
            isGroup: false
            avatarInitials: "c3"
            avatarRamp: 1
            unreadCount: 3
            lastActivityDisplay: "Mon"
            preview: "Did you get a chance to look?"
            description: ""
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
            avatarInitials: "al"
            avatarRamp: 0
            isSelf: true
            pending: false
        }
        ListElement {
            address: "0xbob"
            label: "Bob"
            avatarInitials: "bo"
            avatarRamp: 2
            isSelf: false
            pending: false
        }
        ListElement {
            address: "0xcarol"
            label: "Carol"
            avatarInitials: "ca"
            avatarRamp: 4
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

            ColumnLayout {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                spacing: 0

                ConversationsPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    conversationModel: conversationsMock
                    currentConversationId: "c2"
                    online: true
                }
                AccountCard {
                    Layout.fillWidth: true
                    Layout.margins: Theme.spacing.small
                    address: "0xdeadbeefcafe0123456789abcdef0123456789abcdef0123456789abcdef"
                    label: "0xdeadbe"
                    initials: "0x"
                    online: true
                    statusLabel: "Online"
                }
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
                ready: true
            }
            MembersPane {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                memberModel: membersMock
                online: true
                ready: true
            }
        }

        StatusBar {
            Layout.fillWidth: true
            statusMessage: "Connected to network"
            statusLabel: "Online"
            online: true
        }
    }
}
