import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// Why the network is unknown, for a node Chat did not start, opened from the
// account card's link.
Item {
    id: stage
    width: 1024
    height: 768

    readonly property MockLogos logos: MockLogos {
        backend.chatStatus: ChatBackend.Online
        backend.deliveryAdopted: true
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
        name: "view-unknown-network-dialog"
        target: stage
        setup: () => {
            shot.mouseClick(shot.findChild(view, "deliveryNetworkLink"));
            shot.mouseMove(stage, -1, -1);
        }
    }
}
