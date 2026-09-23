import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// A DM just picked and not loaded yet: the thread holds its skeleton while the
// composer already has the caret.
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
                avatarInitials: "sa"
                avatarRamp: 0
                unreadCount: 0
                lastActivityDisplay: "12:44"
                preview: "Did you get a chance to look?"
                description: ""
            }
        }
    }

    ChatView {
        anchors.fill: parent
    }

    Shot {
        name: "view-loading"
        target: stage
        setup: () => {
            const b = stage.logos.backend;
            b.currentDisplayName = "Saro";
            b.currentAvatarInitials = "sa";
            b.currentConversationId = "c1";
        }
    }
}
