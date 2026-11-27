import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// The first start: an unnamed account typing its name in the account card.
Item {
    id: stage
    width: 1024
    height: 768

    readonly property MockLogos logos: MockLogos {
        backend.chatStatus: ChatBackend.Online
        backend.myAddress: "0fa3867b89b7e34d40123456789abcdef0123456789abcd77013cb70f90dc3cb"
        backend.myLabel: "0fa3867b"
        backend.myInitials: "0f"
    }

    ChatView {
        id: view
        anchors.fill: parent
    }

    Shot {
        id: shot
        name: "view-naming"
        target: stage
        setup: () => {
            shot.mouseClick(shot.findChild(view, "addNameLink"));
            shot.findChild(view, "nameField").text = "Saro";
            shot.mouseMove(stage, -1, -1);
        }
    }
}
