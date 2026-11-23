import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// A hand-off to the Accounts app that ended without an account, back at the
// choice.
Item {
    id: stage
    width: 1024
    height: 768

    readonly property MockLogos logos: MockLogos {
        backend.accountStep: ChatBackend.AccountFailed
        backend.accountProblem: "Accounts did not endorse this installation: the request was cancelled"
    }

    ChatView {
        anchors.fill: parent
    }

    Shot {
        name: "view-account-failed"
        target: stage
    }
}
