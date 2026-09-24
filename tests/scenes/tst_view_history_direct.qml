import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// A direct conversation kept from a previous session, under its heading in the
// list: it reads back, and a notice stands where the composer was.
Item {
    id: stage
    width: 1024
    height: 768

    readonly property MockLogos logos: MockLogos {
        backend.chatStatus: ChatBackend.Online
        backend.myAddress: "4be1c07a9d22f0e8b5a6c3d7e9f0a1b2c3d4e5f60718293a4b5c6d7e8f90a1b2"
        backend.myLabel: "4be1c07a"
        backend.myInitials: "4b"

        conversations: ListModel {
            ListElement {
                conversationId: "c1"
                displayName: "Pax"
                isGroup: false
                avatarInitials: "pa"
                avatarRamp: 2
                unreadCount: 1
                lastActivityDisplay: "14:02"
                preview: "Here's my new address"
                description: ""
                historyOnly: false
            }
            ListElement {
                conversationId: "c2"
                displayName: "Theme review"
                isGroup: true
                avatarInitials: "c2"
                avatarRamp: 3
                unreadCount: 0
                lastActivityDisplay: "13:40"
                preview: "Raya: the new tokens look right"
                description: ""
                historyOnly: false
            }
            ListElement {
                conversationId: "c3"
                displayName: "Saro"
                isGroup: false
                avatarInitials: "sa"
                avatarRamp: 0
                unreadCount: 0
                lastActivityDisplay: "Mon"
                preview: "Sounds good, talk soon"
                description: ""
                historyOnly: true
            }
            ListElement {
                conversationId: "c4"
                displayName: "Release crew"
                isGroup: true
                avatarInitials: "c4"
                avatarRamp: 1
                unreadCount: 0
                lastActivityDisplay: "12 Sep"
                preview: "Raya: notes are up for review, comments welcome before Friday"
                description: "Release planning for 0.3"
                historyOnly: true
            }
            ListElement {
                conversationId: "c5"
                displayName: "Raya"
                isGroup: false
                avatarInitials: "ra"
                avatarRamp: 1
                unreadCount: 0
                lastActivityDisplay: "10 Sep"
                preview: "Did the build go through?"
                description: ""
                historyOnly: true
            }
        }
        // Newest first: the thread is bottom-anchored.
        messages: ListModel {
            ListElement {
                sender: "Saro"
                avatarInitials: "sa"
                avatarRamp: 0
                content: "Sounds good, talk soon"
                timeDisplay: "18:12"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Monday"
            }
            ListElement {
                sender: "Me"
                avatarInitials: "4b"
                avatarRamp: 0
                content: "I'll send the notes after lunch"
                timeDisplay: "18:05"
                isMe: true
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Monday"
            }
            ListElement {
                sender: "Saro"
                avatarInitials: "sa"
                avatarRamp: 0
                content: "Hi, did the new theme land?"
                timeDisplay: "17:58"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: true
                dayLabel: "Monday"
            }
        }
    }

    ChatView {
        anchors.fill: parent
    }

    Shot {
        name: "view-history-direct"
        target: stage
        setup: () => {
            const b = stage.logos.backend;
            b.currentDisplayName = "Saro";
            b.currentAvatarInitials = "sa";
            b.currentHistoryOnly = true;
            b.selectConversation("c3");
        }
    }
}
