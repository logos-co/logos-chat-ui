import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Logos.ChatBackend 1.0

Item {
    id: root
    implicitWidth: 900
    implicitHeight: 650
    Layout.fillWidth: true
    Layout.fillHeight: true

    readonly property color bgPrimary:   "#0A0A0A"
    readonly property color bgSecondary: "#111111"
    readonly property color bgPanel:     "#161616"
    readonly property color border:      "#2a2a2a"
    readonly property color textPrimary: "#FAFAFA"
    readonly property color textSecond:  "#6B7280"
    readonly property color textTertiary:"#4B5563"
    readonly property color accent:      "#10B981"
    readonly property color accentHover: "#34D399"
    readonly property color accentPress: "#059669"
    readonly property color unreadBadge: "#EF4444"
    readonly property color bubblePeer:  "#1F1F1F"

    QtObject {
        id: d
        readonly property var backend: typeof logos !== "undefined" && logos
                                       ? logos.module("chat_ui") : null
        readonly property var convModel: typeof logos !== "undefined" && logos
                                         ? logos.model("chat_ui", "conversationModel") : null
        readonly property var msgModel:  typeof logos !== "undefined" && logos
                                         ? logos.model("chat_ui", "messageModel") : null

        function statusText() {
            if (!backend) return "No backend"
            switch (backend.chatStatus) {
            case ChatBackend.Disconnected:  return "Disconnected"
            case ChatBackend.Initializing:  return "Initializing..."
            case ChatBackend.Initialized:   return "Initialized"
            case ChatBackend.Starting:      return "Starting..."
            case ChatBackend.Running:       return "Connected"
            case ChatBackend.Stopping:      return "Stopping..."
            default: return ""
            }
        }

        function isRunning() {
            return backend && backend.chatStatus === ChatBackend.Running
        }
    }

    // ── Backend signal handlers ──────────────────────────────────────────
    Connections {
        target: d.backend
        ignoreUnknownSignals: true

        function onBundleReady(bundle) {
            bundleDialog.bundleText = bundle
            bundleDialog.open()
        }

        function onError(message) {
            errorToast.text = message
            errorToast.visible = true
            errorToastTimer.restart()
        }
    }

    // ── Main layout ─────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: root.bgPrimary

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // ── Left panel: Conversations ────────────────────────
                Rectangle {
                    Layout.preferredWidth: 260
                    Layout.minimumWidth: 200
                    Layout.fillHeight: true
                    color: root.bgSecondary
                    border.color: root.border
                    border.width: 0

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 0
                        spacing: 0

                        // Header
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            color: root.bgPanel

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12

                                Text {
                                    text: "> \u03BB chat"
                                    color: root.accent
                                    font.family: "JetBrains Mono"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                // Status dot
                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: d.isRunning() ? root.accent : root.textTertiary
                                }
                                Item { Layout.fillWidth: true }
                                Button {
                                    Layout.preferredWidth: 50
                                    text: "+ new"
                                    enabled: d.isRunning()
                                    font.family: "JetBrains Mono"
                                    font.pixelSize: 12

                                    background: Rectangle {
                                        radius: 4
                                        color: parent.enabled
                                            ? (parent.pressed ? root.accentPress
                                                : parent.hovered ? root.accentHover : root.accent)
                                            : root.textTertiary
                                    }
                                    contentItem: Text {
                                        text: parent.text
                                        color: parent.enabled ? "#000000" : root.textSecond
                                        font: parent.font
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    onClicked: newConvDialog.open()
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.border
                        }

                        // Conversation list
                        ListView {
                            id: convList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: d.convModel

                            delegate: Rectangle {
                                width: convList.width
                                height: 56
                                color: model.conversationId === (d.backend ? d.backend.currentConversationId : "")
                                       ? Qt.rgba(16/255, 185/255, 129/255, 0.1)
                                       : mouseArea.containsMouse ? Qt.rgba(255,255,255,0.03) : "transparent"

                                MouseArea {
                                    id: mouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (d.backend)
                                            d.backend.selectConversation(model.conversationId)
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 8

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text {
                                            text: model.displayName || ""
                                            color: root.textPrimary
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 13
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: model.lastActivity ? Qt.formatTime(model.lastActivity, "hh:mm") : ""
                                            color: root.textTertiary
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 10
                                        }
                                    }

                                    // Unread badge
                                    Rectangle {
                                        visible: (model.unreadCount || 0) > 0
                                        width: 20; height: 20; radius: 10
                                        color: root.unreadBadge
                                        Text {
                                            anchors.centerIn: parent
                                            text: model.unreadCount || ""
                                            color: "white"
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }
                                }

                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width
                                    height: 1
                                    color: root.border
                                }
                            }

                            // Empty state
                            Text {
                                anchors.centerIn: parent
                                visible: convList.count === 0
                                text: d.isRunning() ? "No conversations yet" : "Waiting for connection..."
                                color: root.textTertiary
                                font.family: "JetBrains Mono"
                                font.pixelSize: 12
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.border
                        }

                        // Bottom buttons
                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            text: "Get Intro Bundle"
                            enabled: d.isRunning()
                            font.family: "JetBrains Mono"
                            font.pixelSize: 12

                            background: Rectangle {
                                radius: 4
                                color: parent.pressed ? "#333" : parent.hovered ? "#2a2a2a" : root.bgPanel
                                border.color: root.border
                            }
                            contentItem: Text {
                                text: parent.text
                                color: parent.enabled ? root.textPrimary : root.textTertiary
                                font: parent.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                if (d.backend) d.backend.requestMyBundle()
                            }
                        }
                    }
                }

                // Vertical separator
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: root.border
                }

                // ── Right panel: Messages ────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: root.bgPrimary

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Chat header
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            color: root.bgPanel
                            visible: d.backend && d.backend.currentConversationId !== ""

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16

                                Text {
                                    text: {
                                        if (!d.convModel || !d.backend) return ""
                                        // We don't have a direct name lookup, show conversation ID
                                        return d.backend.currentConversationId
                                               ? "Conversation active" : ""
                                    }
                                    color: root.textPrimary
                                    font.family: "JetBrains Mono"
                                    font.pixelSize: 13
                                }
                                Item { Layout.fillWidth: true }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.border
                            visible: d.backend && d.backend.currentConversationId !== ""
                        }

                        // Messages area
                        ListView {
                            id: msgList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: d.msgModel
                            spacing: 8
                            verticalLayoutDirection: ListView.TopToBottom

                            onCountChanged: {
                                Qt.callLater(function() {
                                    msgList.positionViewAtEnd()
                                })
                            }

                            header: Item { height: 12 }
                            footer: Item { height: 12 }

                            delegate: Item {
                                width: msgList.width
                                height: bubble.height + 4
                                readonly property bool alignRight: model.isMe === true

                                Rectangle {
                                    id: bubble
                                    x: alignRight ? parent.width - width - 16 : 16
                                    width: Math.min(msgContent.implicitWidth + 24,
                                                    msgList.width * 0.7)
                                    height: msgContent.height + tsLabel.height + 20
                                    radius: 8
                                    color: alignRight ? root.accent : root.bubblePeer
                                    border.color: alignRight ? "transparent" : root.border
                                    border.width: alignRight ? 0 : 1

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 4

                                        Text {
                                            id: msgContent
                                            text: model.content || ""
                                            color: alignRight ? "#000000" : root.textPrimary
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 13
                                            wrapMode: Text.Wrap
                                            Layout.fillWidth: true
                                        }

                                        Text {
                                            id: tsLabel
                                            text: model.timestamp
                                                  ? Qt.formatTime(model.timestamp, "hh:mm")
                                                  : ""
                                            color: alignRight ? Qt.rgba(0,0,0,0.5)
                                                              : root.textTertiary
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 10
                                            horizontalAlignment: alignRight
                                                                 ? Text.AlignRight
                                                                 : Text.AlignLeft
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }

                            // Empty state
                            Text {
                                anchors.centerIn: parent
                                visible: msgList.count === 0
                                text: d.backend && d.backend.currentConversationId !== ""
                                      ? "No messages yet"
                                      : "Select a conversation to start chatting"
                                color: root.textTertiary
                                font.family: "JetBrains Mono"
                                font.pixelSize: 13
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.border
                        }

                        // Message input
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52
                            color: root.bgSecondary

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 8

                                TextField {
                                    id: msgInput
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    placeholderText: d.isRunning() ? "Type a message..."
                                                                   : "Chat not connected"
                                    enabled: d.isRunning()
                                             && d.backend
                                             && d.backend.currentConversationId !== ""
                                    font.family: "JetBrains Mono"
                                    font.pixelSize: 13
                                    color: root.textPrimary

                                    background: Rectangle {
                                        radius: 4
                                        color: root.bgPanel
                                        border.color: msgInput.activeFocus ? root.accent : root.border
                                    }

                                    onAccepted: sendBtn.sendMessage()
                                }

                                Button {
                                    id: sendBtn
                                    Layout.preferredWidth: 44
                                    Layout.fillHeight: true
                                    text: ">>"
                                    enabled: msgInput.enabled && msgInput.text.trim() !== ""
                                    font.family: "JetBrains Mono"
                                    font.pixelSize: 14
                                    font.bold: true

                                    function sendMessage() {
                                        var text = msgInput.text.trim()
                                        if (text === "" || !d.backend) return
                                        d.backend.sendMessage(
                                            d.backend.currentConversationId, text)
                                        msgInput.text = ""
                                    }

                                    background: Rectangle {
                                        radius: 4
                                        color: parent.enabled
                                               ? (parent.pressed ? root.accentPress
                                                  : parent.hovered ? root.accentHover
                                                  : root.accent)
                                               : root.textTertiary
                                    }
                                    contentItem: Text {
                                        text: parent.text
                                        color: "#000000"
                                        font: parent.font
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: sendMessage()
                                }
                            }
                        }
                    }
                }
            }

            // ── Status bar ───────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                color: root.bgPanel

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    Text {
                        text: d.backend ? d.backend.statusMessage : "No backend"
                        color: root.textTertiary
                        font.family: "JetBrains Mono"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: d.statusText()
                        color: d.isRunning() ? root.accent : root.textTertiary
                        font.family: "JetBrains Mono"
                        font.pixelSize: 11
                    }

                    Rectangle { width: 8; height: 1; color: "transparent" }

                    Text {
                        text: d.backend && d.backend.myIdentity
                              ? "ID: " + d.backend.myIdentity
                              : "ID: ..."
                        color: root.textTertiary
                        font.family: "JetBrains Mono"
                        font.pixelSize: 11
                    }
                }
            }
        }
    }

    // ── New Conversation dialog ──────────────────────────────────────────
    Dialog {
        id: newConvDialog
        anchors.centerIn: parent
        width: 480
        modal: true
        padding: 10
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color: root.bgPanel
            border.color: root.border
            radius: 8
        }
        header: Rectangle {
            color: "transparent"
            height: 44
            Text {
                anchors.centerIn: parent
                text: "New Conversation"
                color: root.textPrimary
                font.family: "JetBrains Mono"
                font.pixelSize: 14
                font.bold: true
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: root.border
            }
        }

        footer: DialogButtonBox {
            background: Rectangle { color: root.bgPanel }
            Component.onCompleted: {
                var okBtn = newConvDialog.standardButton(Dialog.Ok)
                if (okBtn) {
                    okBtn.text = "Create"
                    okBtn.font.family = "JetBrains Mono"
                    okBtn.font.pixelSize = 12
                    okBtn.palette.button = root.accent
                    okBtn.palette.buttonText = "#000000"
                }
                var cancelBtn = newConvDialog.standardButton(Dialog.Cancel)
                if (cancelBtn) {
                    cancelBtn.font.family = "JetBrains Mono"
                    cancelBtn.font.pixelSize = 12
                    cancelBtn.palette.button = root.bgPanel
                    cancelBtn.palette.buttonText = root.textPrimary
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Text {
                text: "Paste the other user's intro bundle:"
                color: root.textSecond
                font.family: "JetBrains Mono"
                font.pixelSize: 12
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                implicitHeight: 100
                TextArea {
                    id: bundleInput
                    placeholderText: "logos_chatintro..."
                    font.family: "JetBrains Mono"
                    font.pixelSize: 12
                    color: root.textPrimary
                    wrapMode: TextEdit.Wrap
                    background: Rectangle {
                        color: root.bgPrimary
                        border.color: root.border
                        radius: 4
                    }
                }
            }

            Text {
                text: "Intro message:"
                color: root.textSecond
                font.family: "JetBrains Mono"
                font.pixelSize: 12
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                implicitHeight: 80
                TextArea {
                    id: introMsgInput
                    text: "Hello!"
                    placeholderText: "Type your first message"
                    placeholderTextColor: root.textTertiary
                    font.family: "JetBrains Mono"
                    font.pixelSize: 12
                    color: root.textPrimary
                    wrapMode: TextEdit.Wrap
                    background: Rectangle {
                        color: root.bgPrimary
                        border.color: introMsgInput.activeFocus ? root.accent : root.border
                        radius: 4
                    }
                }
            }
        }

        onAccepted: {
            var bundle = bundleInput.text.trim()
            var msg = introMsgInput.text.trim()
            if (bundle !== "" && msg !== "" && d.backend) {
                d.backend.createConversation(bundle, msg)
            }
            bundleInput.text = ""
            introMsgInput.text = "Hello!"
        }
        onRejected: {
            bundleInput.text = ""
            introMsgInput.text = "Hello!"
        }
    }

    // ── Bundle display dialog ────────────────────────────────────────────
    Dialog {
        id: bundleDialog
        anchors.centerIn: parent
        width: 480
        modal: true

        property string bundleText: ""

        background: Rectangle {
            color: root.bgPanel
            border.color: root.border
            radius: 8
        }
        header: Rectangle {
            color: "transparent"
            height: 44
            Text {
                anchors.centerIn: parent
                text: "My Bundle"
                color: root.textPrimary
                font.family: "JetBrains Mono"
                font.pixelSize: 14
                font.bold: true
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: root.border
            }
        }

        footer: DialogButtonBox {
            background: Rectangle { color: root.bgPanel }
            Button {
                text: "Copy to Clipboard"
                font.family: "JetBrains Mono"; font.pixelSize: 12
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                palette.button: root.accent
                palette.buttonText: "#000000"
                onClicked: {
                    bundleDisplay.selectAll()
                    bundleDisplay.copy()
                    bundleDisplay.deselect()
                    copiedLabel.visible = true
                    copiedTimer.restart()
                }
            }
            Button {
                text: "Close"
                font.family: "JetBrains Mono"; font.pixelSize: 12
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                palette.button: root.bgPanel
                palette.buttonText: root.textPrimary
            }
            onRejected: bundleDialog.close()
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Text {
                text: "Share this bundle with others to start a conversation:"
                color: root.textSecond
                font.family: "JetBrains Mono"
                font.pixelSize: 12
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                TextArea {
                    id: bundleDisplay
                    text: bundleDialog.bundleText
                    readOnly: true
                    font.family: "JetBrains Mono"
                    font.pixelSize: 11
                    color: root.accent
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    background: Rectangle {
                        color: root.bgPrimary
                        border.color: root.border
                        radius: 4
                    }
                }
            }

            Text {
                id: copiedLabel
                visible: false
                text: "Copied to clipboard!"
                color: root.accent
                font.family: "JetBrains Mono"
                font.pixelSize: 11
                Timer {
                    id: copiedTimer
                    interval: 2000
                    onTriggered: copiedLabel.visible = false
                }
            }
        }
    }

    // ── Error toast ──────────────────────────────────────────────────────
    Rectangle {
        id: errorToast
        visible: false
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter
        width: toastText.implicitWidth + 32
        height: 36
        radius: 6
        color: "#5c1a1a"
        border.color: "#C62828"

        property alias text: toastText.text

        Text {
            id: toastText
            anchors.centerIn: parent
            color: "#EF4444"
            font.family: "JetBrains Mono"
            font.pixelSize: 12
        }

        Timer {
            id: errorToastTimer
            interval: 4000
            onTriggered: errorToast.visible = false
        }
    }
}
