import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// The logs dialog over the view, opened the way the status bar opens it.
Item {
    id: stage
    width: 1024
    height: 768

    readonly property MockLogos logos: MockLogos {
        backend.chatStatus: ChatBackend.Online
        backend.myAddress: "4be1c07a9d22f0e8b5a6c3d7e9f0a1b2c3d4e5f60718293a4b5c6d7e8f90a1b2"
        backend.myLabel: "4be1c07a"
        backend.myInitials: "4b"
        backend.logDir: "/home/saro/.local/share/logos/chat_module"
        backend.errors: [
            {
                when: "14:32:07",
                message: "Could not send to Design Team: delivery is not connected",
                count: 3
            },
            {
                when: "14:29:02",
                message: "Could not add a member: address is not an account key",
                count: 1
            }
        ]
    }

    ChatView {
        id: view
        anchors.fill: parent
    }

    Shot {
        name: "view-logs-dialog"
        target: stage
        setup: () => view.showLogs()
    }
}
