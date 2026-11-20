import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// Delivery's node runs with no peers for the moment: the card says Offline,
// and New chat stays enabled.
Item {
    id: stage
    width: 1024
    height: 768

    readonly property MockLogos logos: MockLogos {
        backend.chatStatus: ChatBackend.Offline
        backend.myAddress: "0fa3867b89b7e34d40123456789abcdef0123456789abcd77013cb70f90dc3cb"
        backend.myLabel: "0fa3867b"
        backend.myInitials: "0f"
    }

    ChatView {
        anchors.fill: parent
    }

    Shot {
        name: "view-offline"
        target: stage
    }
}
