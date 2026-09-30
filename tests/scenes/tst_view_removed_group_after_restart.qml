import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// The same group after a restart, from a previous session as well: listed
// under the previous sessions' heading, with the removal's notice in place of
// the previous session's, and details naming both.
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
                conversationId: "c7"
                displayName: "Raya"
                isGroup: false
                avatarInitials: "ra"
                avatarRamp: 1
                unreadCount: 1
                lastActivityDisplay: "09:14"
                preview: "Here's my new address"
                description: ""
                historyOnly: false
                removed: false
            }
            ListElement {
                conversationId: "c1"
                displayName: "Theme review"
                isGroup: true
                avatarInitials: "c1"
                avatarRamp: 3
                unreadCount: 0
                lastActivityDisplay: "Yesterday"
                preview: "Raya: the new tokens look right"
                description: ""
                historyOnly: true
                removed: false
            }
            ListElement {
                conversationId: "c2"
                displayName: "Release crew"
                isGroup: true
                avatarInitials: "c2"
                avatarRamp: 1
                unreadCount: 0
                lastActivityDisplay: "Yesterday"
                preview: "Saro: tagged 0.3.0-rc1"
                description: "Release planning for 0.3"
                historyOnly: true
                removed: true
            }
            ListElement {
                conversationId: "c4"
                displayName: "Saro"
                isGroup: false
                avatarInitials: "sa"
                avatarRamp: 0
                unreadCount: 0
                lastActivityDisplay: "Mon"
                preview: "Sounds good, talk soon"
                description: ""
                historyOnly: true
                removed: false
            }
        }
        // Newest first: the thread is bottom-anchored.
        messages: ListModel {
            ListElement {
                sender: "Saro"
                avatarInitials: "sa"
                avatarRamp: 0
                content: "Tagged 0.3.0-rc1"
                timeDisplay: "13:52"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Yesterday"
            }
            ListElement {
                sender: "Raya"
                avatarInitials: "ra"
                avatarRamp: 1
                content: "Notes are up for review, comments welcome before Friday"
                timeDisplay: "13:40"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Yesterday"
            }
            ListElement {
                sender: "Me"
                avatarInitials: "4b"
                avatarRamp: 0
                content: "CI is green on the release branch"
                timeDisplay: "13:12"
                isMe: true
                sameSenderAsPrevious: false
                showDaySeparator: true
                dayLabel: "Yesterday"
            }
        }
    }

    ChatView {
        anchors.fill: parent
        detailsShown: true
    }

    Shot {
        name: "view-removed-group-after-restart"
        target: stage
        setup: () => {
            const b = stage.logos.backend;
            b.currentIsGroup = true;
            b.currentDisplayName = "Release crew";
            b.currentDescription = "Release planning for 0.3";
            b.currentAvatarInitials = "c2";
            b.currentAvatarRamp = 1;
            b.currentHistoryOnly = true;
            b.currentRemoved = true;
            b.selectConversation("c2");
        }
    }
}
