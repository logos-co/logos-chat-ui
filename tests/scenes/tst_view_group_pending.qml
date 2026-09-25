import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// A group just opened, with its details, one member still waiting to join, a
// thread spanning two days, and failures held on the status bar. Opening it is
// what hands the composer the caret.
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
                displayName: "Saro"
                isGroup: false
                avatarInitials: "sa"
                avatarRamp: 0
                unreadCount: 3
                lastActivityDisplay: "12:44"
                preview: "Did you get a chance to look?"
                description: ""
                historyOnly: false
            }
            ListElement {
                conversationId: "c2"
                displayName: "Design Team"
                isGroup: true
                avatarInitials: "c2"
                avatarRamp: 3
                unreadCount: 0
                lastActivityDisplay: "12:41"
                preview: "You: Merged, thanks all"
                description: "Design reviews and theme work, with a description long enough to be clamped to one line in the header"
                historyOnly: false
            }
            ListElement {
                conversationId: "c3"
                displayName: "A group whose name is far too long for one sidebar row"
                isGroup: true
                avatarInitials: "c3"
                avatarRamp: 1
                unreadCount: 128
                lastActivityDisplay: "Yesterday"
                preview: "Raya: the release notes are up for review, comments welcome before Friday"
                description: ""
                historyOnly: false
            }
            ListElement {
                conversationId: "c4"
                displayName: "Pax"
                isGroup: false
                avatarInitials: "pa"
                avatarRamp: 2
                unreadCount: 0
                lastActivityDisplay: "Mon"
                preview: ""
                description: ""
                historyOnly: false
            }
        }
        // Newest first: the thread is bottom-anchored.
        messages: ListModel {
            ListElement {
                sender: "Me"
                avatarInitials: "4b"
                avatarRamp: 0
                content: "Merged, thanks all"
                timeDisplay: "12:41"
                isMe: true
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Today"
            }
            ListElement {
                sender: "Raya"
                avatarInitials: "ra"
                avatarRamp: 1
                content: "Pushed the fix, please review"
                timeDisplay: "12:39"
                isMe: false
                sameSenderAsPrevious: true
                showDaySeparator: false
                dayLabel: "Today"
            }
            ListElement {
                sender: "Raya"
                avatarInitials: "ra"
                avatarRamp: 1
                content: "Found the elide bug in the header"
                timeDisplay: "12:38"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Today"
            }
            ListElement {
                sender: "Saro"
                avatarInitials: "sa"
                avatarRamp: 0
                content: "Morning! Anyone looked at the facepile yet?"
                timeDisplay: "09:02"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: true
                dayLabel: "Today"
            }
            ListElement {
                sender: "Me"
                avatarInitials: "4b"
                avatarRamp: 0
                content: "See you tomorrow"
                timeDisplay: "18:20"
                isMe: true
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Yesterday"
            }
            ListElement {
                sender: "Saro"
                avatarInitials: "sa"
                avatarRamp: 0
                content: "A longer message that wraps across several lines in the bubble, to show where the seventy-percent cap on a bubble's width lands and how the time sits under the last line."
                timeDisplay: "18:10"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: true
                dayLabel: "Yesterday"
            }
        }
        members: ListModel {
            ListElement {
                address: "4be1c07a9d22"
                label: "4be1c07a"
                avatarInitials: "4b"
                avatarRamp: 0
                isSelf: true
                pending: false
            }
            ListElement {
                address: "0b2c3d4e5f60"
                label: "Raya"
                avatarInitials: "ra"
                avatarRamp: 1
                isSelf: false
                pending: false
            }
            ListElement {
                address: "a1b2c3d4e5f6"
                label: "Saro"
                avatarInitials: "sa"
                avatarRamp: 0
                isSelf: false
                pending: false
            }
            ListElement {
                address: "f6e5d4c3b2a1"
                label: "Pax"
                avatarInitials: "pa"
                avatarRamp: 2
                isSelf: false
                pending: true
            }
        }
    }

    ChatView {
        anchors.fill: parent
        detailsShown: true
        lastError: "Could not send to Design Team: delivery is not connected"
        unseenErrorCount: 3
    }

    Shot {
        name: "view-group-pending"
        target: stage
        setup: () => {
            const b = stage.logos.backend;
            b.currentIsGroup = true;
            b.currentDisplayName = "Design Team";
            b.currentDescription = "Design reviews and theme work, with a description long enough to be clamped to one line in the header";
            b.currentAvatarInitials = "c2";
            b.currentAvatarRamp = 3;
            b.memberCount = 3;
            b.pendingMemberCount = 1;
            b.selectConversation("c2");
        }
    }
}
