import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// A direct conversation open on a short exchange, with its details beside it
// and no roster below them.
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
                displayName: "Saro"
                isGroup: false
                avatarInitials: "a1"
                avatarRamp: 1
                unreadCount: 0
                lastActivityDisplay: "12:44"
                preview: "Sounds good, talk soon"
                description: ""
                historyOnly: false
                peerLabel: "a1b2c3d4"
            }
            ListElement {
                conversationId: "c2"
                displayName: "Raya"
                isGroup: false
                avatarInitials: "0b"
                avatarRamp: 1
                unreadCount: 2
                lastActivityDisplay: "Mon"
                preview: "Did the build go through?"
                description: ""
                historyOnly: false
                peerLabel: "0b2c3d4e"
            }
        }
        // Newest first: the thread is bottom-anchored.
        messages: ListModel {
            ListElement {
                sender: "a1b2c3d4"
                senderName: "Saro"
                avatarInitials: "a1"
                avatarRamp: 1
                content: "Sounds good, talk soon"
                timeDisplay: "12:44"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Today"
                delivered: false
            }
            ListElement {
                sender: "Me"
                senderName: ""
                avatarInitials: "4b"
                avatarRamp: 0
                content: "I'll send the notes after lunch"
                timeDisplay: "12:40"
                isMe: true
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Today"
                delivered: false
            }
            ListElement {
                sender: "a1b2c3d4"
                senderName: "Saro"
                avatarInitials: "a1"
                avatarRamp: 1
                content: "Hi, did the new theme land?"
                timeDisplay: "12:31"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: true
                dayLabel: "Today"
                delivered: false
            }
        }
        members: ListModel {
            ListElement {
                address: "4be1c07a9d22f0e8b5a6c3d7e9f0a1b2c3d4e5f60718293a4b5c6d7e8f90a1b2"
                label: "4be1c07a"
                accountName: ""
                avatarInitials: "4b"
                avatarRamp: 2
                isSelf: true
                pending: false
            }
            ListElement {
                address: "a1b2c3d4e5f60718293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f9"
                label: "a1b2c3d4"
                accountName: "Saro"
                avatarInitials: "a1"
                avatarRamp: 1
                isSelf: false
                pending: false
            }
        }
    }

    ChatView {
        anchors.fill: parent
        detailsShown: true
    }

    Shot {
        name: "view-direct-details"
        target: stage
        setup: () => {
            const b = stage.logos.backend;
            b.currentDisplayName = "Saro";
            b.currentAvatarInitials = "a1";
            b.currentAvatarRamp = 1;
            b.currentPeerAddress = "a1b2c3d4e5f60718293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f9";
            b.currentPeerLabel = "a1b2c3d4";
            b.memberCount = 2;
            b.currentProtocol = "DirectV1";
            b.selectConversation("c1");
        }
    }
}
