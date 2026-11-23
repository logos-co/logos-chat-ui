import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// Before init: run as the Accounts app's account, or on one of Chat's own.
Item {
    id: stage
    width: 1024
    height: 768

    readonly property MockLogos logos: MockLogos {
        backend.accountStep: ChatBackend.ChoosingAccount
    }

    ChatView {
        anchors.fill: parent
    }

    Shot {
        name: "view-account-choice"
        target: stage
    }
}
