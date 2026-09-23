import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// Delivery still coming up: no address yet, nothing to start.
Item {
    id: stage
    width: 1024
    height: 768

    readonly property MockLogos logos: MockLogos {
        backend.chatStatus: ChatBackend.Initialising
    }

    ChatView {
        anchors.fill: parent
    }

    Shot {
        name: "view-initialising"
        target: stage
    }
}
