import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// A group kept from a previous session, with its details: no roster card, and
// the details name the session in place of the member count.
Item {
    id: stage
    width: 1280
    height: 800

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
                sender: "Raya"
                avatarInitials: "ra"
                avatarRamp: 1
                content: "Notes are up for review, comments welcome before Friday"
                timeDisplay: "16:30"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "12 September"
            }
            ListElement {
                sender: "Me"
                avatarInitials: "4b"
                avatarRamp: 0
                content: "Tagged 0.3.0-rc1"
                timeDisplay: "16:02"
                isMe: true
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "12 September"
            }
            ListElement {
                sender: "Pax"
                avatarInitials: "pa"
                avatarRamp: 2
                content: "CI is green on the release branch"
                timeDisplay: "15:47"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: true
                dayLabel: "12 September"
            }
        }
    }

    ChatView {
        anchors.fill: parent
        detailsShown: true
    }

    Shot {
        name: "view-history-group"
        target: stage
        setup: () => {
            const b = stage.logos.backend;
            b.currentIsGroup = true;
            b.currentDisplayName = "Release crew";
            b.currentDescription = "Release planning for 0.3";
            b.currentAvatarInitials = "c4";
            b.currentAvatarRamp = 1;
            b.currentHistoryOnly = true;
            b.selectConversation("c4");
        }
    }
}
