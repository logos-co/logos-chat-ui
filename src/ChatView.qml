import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ChatBackend 1.0

Rectangle {
    id: root
    color: _d.backgroundColor

    QtObject {
        id: _d

        readonly property color backgroundColor: "#1e1e1e"
        readonly property color surfaceColor: "#2d2d2d"
        readonly property color surfaceDarkColor: "#252525"
        readonly property color borderColor: "#5d5d5d"
        readonly property color textColor: "#ffffff"
        readonly property color textSecondaryColor: "#a0a0a0"
        readonly property color textDisabledColor: "#808080"
        readonly property color buttonColor: "#4d4d4d"
        readonly property color hoverColor: "#3d3d3d"
        readonly property color accentColor: "#e5e5e5"

        readonly property color statusSuccessColor: "#4ade80"  // Green for Ready
        readonly property color statusErrorColor: "#ef4444"    // Red for Error
        readonly property color statusWarningColor: "#fbbf24"  // Amber for Initializing
        readonly property color statusNeutralColor: "#9ca3af"  // Gray for NotInitialized

        readonly property color historyColor: "#c4c4c4"  // Slightly dimmer than accent for history messages

        readonly property int primaryFontSize: 14
        readonly property int smallFontSize: 12

        // Network readiness check: LP peers > 0 AND mix pool size >= 3
        readonly property bool networkReady: backend.lightpushPeersCount > 0 && backend.mixnodePoolSize >= 3

        function getNetworkWarningMessage() {
            if (backend.lightpushPeersCount === 0 && backend.mixnodePoolSize < 3) {
                return "Waiting for network peers (Mix: " + backend.mixnodePoolSize + "/3, LP: " + backend.lightpushPeersCount + ")..."
            } else if (backend.lightpushPeersCount === 0) {
                return "Waiting for lightpush peers..."
            } else if (backend.mixnodePoolSize < 3) {
                return "Waiting for mix nodes (" + backend.mixnodePoolSize + "/3)..."
            }
            return ""
        }

        function getStatusString(status) {
            switch(status) {
            case ChatBackend.NotInitialized: return "Not initialized";
            case ChatBackend.Initializing: return "Initializing chat...";
            case ChatBackend.Ready: return "Ready";
            case ChatBackend.Error: return "Error";
            default: return "Unknown";
            }
        }

        function getStatusColor(status) {
            switch(status) {
            case ChatBackend.Ready: return statusSuccessColor;
            case ChatBackend.Error: return statusErrorColor;
            case ChatBackend.Initializing: return statusWarningColor;
            case ChatBackend.NotInitialized: return statusNeutralColor;
            default: return textSecondaryColor;
            }
        }
    }

    // Restart confirmation dialog
    Dialog {
        id: restartDialog
        title: "Restart Required"
        modal: true
        anchors.centerIn: parent
        width: 400

        background: Rectangle {
            color: _d.surfaceColor
            border.color: _d.borderColor
            border.width: 1
            radius: 8
        }

        contentItem: ColumnLayout {
            spacing: 16

            Text {
                text: "Settings have been saved. A restart is required to apply the changes."
                font.pixelSize: _d.primaryFontSize
                color: _d.textColor
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Text {
                text: "Do you want to restart now?"
                font.pixelSize: _d.primaryFontSize
                color: _d.textSecondaryColor
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    text: "No"
                    onClicked: restartDialog.close()

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: _d.primaryFontSize
                        color: _d.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitWidth: 80
                        implicitHeight: 36
                        color: parent.pressed ? _d.hoverColor : _d.buttonColor
                        radius: 4
                        border.color: _d.borderColor
                        border.width: 1
                    }
                }

                Button {
                    text: "Yes, Restart"
                    onClicked: {
                        restartDialog.close()
                        Qt.quit()
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: _d.primaryFontSize
                        color: _d.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitWidth: 120
                        implicitHeight: 36
                        color: parent.pressed ? _d.statusSuccessColor : "#3d8c40"
                        radius: 4
                        border.color: _d.statusSuccessColor
                        border.width: 1
                    }
                }
            }
        }
    }

    Dialog {
        id: resetPeerIdDialog
        title: "Reset Peer ID"
        modal: true
        anchors.centerIn: parent
        width: 400

        background: Rectangle {
            color: _d.surfaceColor
            border.color: _d.borderColor
            border.width: 1
            radius: 8
        }

        contentItem: ColumnLayout {
            spacing: 16

            Text {
                text: "This will generate a new Peer ID and restart the application."
                font.pixelSize: _d.primaryFontSize
                color: _d.textColor
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Text {
                text: "Are you sure?"
                font.pixelSize: _d.primaryFontSize
                color: _d.textSecondaryColor
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    onClicked: resetPeerIdDialog.close()

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: _d.primaryFontSize
                        color: _d.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitWidth: 80
                        implicitHeight: 36
                        color: parent.pressed ? _d.hoverColor : _d.buttonColor
                        radius: 4
                        border.color: _d.borderColor
                        border.width: 1
                    }
                }

                Button {
                    text: "Reset & Restart"
                    onClicked: {
                        resetPeerIdDialog.close()
                        backend.resetPeerId()
                        Qt.quit()
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: _d.primaryFontSize
                        color: _d.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitWidth: 120
                        implicitHeight: 36
                        color: parent.pressed ? _d.statusErrorColor : "#8c3d3d"
                        radius: 4
                        border.color: _d.statusErrorColor
                        border.width: 1
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        // Tab bar
        TabBar {
            id: tabBar
            Layout.fillWidth: true

            background: Rectangle {
                color: _d.surfaceColor
                radius: 4
            }

            TabButton {
                text: "Chat"
                width: implicitWidth

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: _d.primaryFontSize
                    font.bold: parent.checked
                    color: parent.checked ? _d.accentColor : _d.textSecondaryColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: parent.checked ? _d.hoverColor : _d.surfaceDarkColor
                    radius: 4
                    border.color: parent.checked ? _d.accentColor : _d.borderColor
                    border.width: parent.checked ? 2 : 1

                    // 3D effect with shadow
                    Rectangle {
                        anchors.fill: parent
                        anchors.topMargin: parent.checked ? 0 : 2
                        radius: parent.radius
                        color: "transparent"
                        border.color: parent.checked ? "#ffffff" : "transparent"
                        border.width: 1
                        opacity: parent.checked ? 0.2 : 0
                    }

                    // Bottom shadow for 3D effect
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 3
                        radius: parent.radius
                        color: "#000000"
                        opacity: parent.checked ? 0 : 0.3
                    }
                }
            }

            TabButton {
                visible: showSettings
                text: "Settings"
                width: visible ? implicitWidth : 0

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: _d.primaryFontSize
                    font.bold: parent.checked
                    color: parent.checked ? _d.accentColor : _d.textSecondaryColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: parent.checked ? _d.hoverColor : _d.surfaceDarkColor
                    radius: 4
                    border.color: parent.checked ? _d.accentColor : _d.borderColor
                    border.width: parent.checked ? 2 : 1

                    // 3D effect with shadow
                    Rectangle {
                        anchors.fill: parent
                        anchors.topMargin: parent.checked ? 0 : 2
                        radius: parent.radius
                        color: "transparent"
                        border.color: parent.checked ? "#ffffff" : "transparent"
                        border.width: 1
                        opacity: parent.checked ? 0.2 : 0
                    }

                    // Bottom shadow for 3D effect
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 3
                        radius: parent.radius
                        color: "#000000"
                        opacity: parent.checked ? 0 : 0.3
                    }
                }
            }
        }

        StackLayout {
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Chat Tab Content
            ColumnLayout {
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: _d.surfaceColor
                    radius: 4
                    border.color: _d.borderColor
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 16

                        Text {
                            text: "Status:"
                            font.pixelSize: _d.primaryFontSize
                            color: _d.textColor
                        }

                        Text {
                            text: _d.getStatusString(backend.status)
                            font.pixelSize: _d.primaryFontSize
                            font.bold: true
                            color: _d.getStatusColor(backend.status)
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: _d.borderColor
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            text: "Username:"
                            font.pixelSize: _d.primaryFontSize
                            color: _d.textColor
                        }

                        Text {
                            text: backend.username
                            font.pixelSize: _d.primaryFontSize
                            font.bold: true
                            color: _d.accentColor
                        }

                        Rectangle {
                            width: 1
                            height: parent.height - 8
                            color: _d.borderColor
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            text: "Mix:"
                            font.pixelSize: _d.primaryFontSize
                            color: _d.textColor
                        }

                        Text {
                            text: backend.mixnodePoolSize
                            font.pixelSize: _d.primaryFontSize
                            font.bold: true
                            color: backend.mixnodePoolSize > 0 ? _d.statusSuccessColor : _d.statusWarningColor
                        }

                        Text {
                            text: "LP Peers:"
                            font.pixelSize: _d.primaryFontSize
                            color: _d.textColor
                        }

                        Text {
                            text: backend.lightpushPeersCount
                            font.pixelSize: _d.primaryFontSize
                            font.bold: true
                            color: backend.lightpushPeersCount > 0 ? _d.statusSuccessColor : _d.statusWarningColor
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "Channel:"
                        font.pixelSize: _d.primaryFontSize
                        color: _d.textColor
                        Layout.alignment: Qt.AlignVCenter
                    }

                    TextField {
                        id: channelInput
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        placeholderText: "Enter channel name..."
                        text: backend.currentChannel
                        enabled: backend.status === ChatBackend.Ready
                        font.pixelSize: _d.primaryFontSize
                        color: _d.textColor
                        verticalAlignment: TextInput.AlignVCenter
                        leftPadding: 12
                        rightPadding: 12

                        background: Rectangle {
                            color: parent.enabled ? _d.surfaceColor : _d.surfaceDarkColor
                            radius: 4
                            border.color: parent.activeFocus ? _d.accentColor : _d.borderColor
                            border.width: 1
                        }

                        Keys.onReturnPressed: joinButton.clicked()
                    }

                    Button {
                        id: joinButton
                        text: "Join"
                        enabled: backend.status === ChatBackend.Ready && channelInput.text.trim() !== ""
                        font.pixelSize: _d.primaryFontSize
                        Layout.preferredWidth: 100

                        onClicked: {
                            if (channelInput.text.trim() !== "") {
                                backend.joinChannel(channelInput.text.trim())
                            }
                        }

                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: parent.enabled ? _d.textColor : _d.textDisabledColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitHeight: 36
                            color: parent.enabled ? (parent.pressed ? _d.hoverColor : _d.buttonColor) : _d.surfaceColor
                            radius: 4
                            border.color: parent.enabled ? _d.borderColor : _d.hoverColor
                            border.width: 1
                        }
                    }
                }

                Text {
                    text: "Messages:"
                    font.pixelSize: _d.primaryFontSize
                    color: _d.textColor
                }

                ScrollView {
                    id: messagesScrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    background: Rectangle {
                        color: _d.surfaceColor
                        radius: 4
                        border.color: _d.borderColor
                        border.width: 1
                    }

                    ListView {
                        id: messagesListView
                        anchors.fill: parent
                        anchors.margins: 8
                        model: backend.messages
                        spacing: 2

                        delegate: Item {
                            width: messagesListView.width
                            height: messageColumn.implicitHeight + 8

                            Rectangle {
                                anchors.fill: parent
                                color: modelData.isSystem ? _d.surfaceDarkColor : "transparent"
                                radius: 2
                            }

                            RowLayout {
                                id: messageColumn
                                width: parent.width - 8
                                anchors.left: parent.left
                                anchors.leftMargin: 4
                                anchors.top: parent.top
                                anchors.topMargin: 4
                                spacing: 8

                                Text {
                                    text: modelData.timestamp || ""
                                    font.pixelSize: _d.smallFontSize
                                    color: _d.textSecondaryColor
                                }

                                Text {
                                    text: modelData.sender || ""
                                    font.pixelSize: _d.primaryFontSize
                                    font.bold: true
                                    color: {
                                        if (modelData.isSystem)  return _d.textSecondaryColor
                                        if (modelData.isHistory) return _d.historyColor
                                        return _d.accentColor
                                    }
                                }

                                Text {
                                    text: modelData.message || ""
                                    font.pixelSize: _d.primaryFontSize
                                    color: modelData.isSystem ? _d.textSecondaryColor : _d.textColor
                                    font.italic: modelData.isSystem || false
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        onCountChanged: {
                            Qt.callLater(function() {
                                messagesListView.positionViewAtEnd()
                            })
                        }
                    }
                }

                // Network warning message (shown when network is not ready)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    color: _d.surfaceDarkColor
                    radius: 4
                    border.color: _d.statusWarningColor
                    border.width: 1
                    visible: !_d.networkReady && backend.status === ChatBackend.Ready && backend.currentChannel !== ""

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        Text {
                            text: "\u26A0"  // Warning symbol
                            font.pixelSize: _d.primaryFontSize
                            color: _d.statusWarningColor
                        }

                        Text {
                            text: _d.getNetworkWarningMessage()
                            font.pixelSize: _d.smallFontSize
                            color: _d.statusWarningColor
                            Layout.fillWidth: true
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    TextField {
                        id: messageInput
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        placeholderText: _d.networkReady ? "Type your message here..." : "Waiting for network..."
                        enabled: backend.status === ChatBackend.Ready && backend.currentChannel !== "" && _d.networkReady
                        font.pixelSize: _d.primaryFontSize
                        color: _d.textColor
                        verticalAlignment: TextInput.AlignVCenter
                        leftPadding: 12
                        rightPadding: 12

                        background: Rectangle {
                            color: parent.enabled ? _d.surfaceColor : _d.surfaceDarkColor
                            radius: 4
                            border.color: parent.activeFocus ? _d.accentColor : _d.borderColor
                            border.width: 1
                        }

                        Keys.onReturnPressed: sendButton.clicked()
                    }

                    Button {
                        id: sendButton
                        text: "Send"
                        enabled: backend.status === ChatBackend.Ready &&
                                 backend.currentChannel !== "" &&
                                 messageInput.text.trim() !== "" &&
                                 _d.networkReady
                        font.pixelSize: _d.primaryFontSize
                        Layout.preferredWidth: 100

                        onClicked: {
                            if (messageInput.text.trim() !== "") {
                                backend.sendMessage(messageInput.text.trim())
                                messageInput.clear()
                            }
                        }

                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: parent.enabled ? _d.textColor : _d.textDisabledColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitHeight: 36
                            color: parent.enabled ? (parent.pressed ? _d.hoverColor : _d.buttonColor) : _d.surfaceColor
                            radius: 4
                            border.color: parent.enabled ? _d.borderColor : _d.hoverColor
                            border.width: 1
                        }
                    }
                }
            }

            // Settings Tab Content
            ColumnLayout {
                spacing: 16

                // Discovery Mode Section
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    RowLayout {
                        spacing: 8

                        Text {
                            text: "Discovery Mode"
                            font.pixelSize: _d.primaryFontSize
                            font.bold: true
                            color: _d.textColor
                        }

                        Text {
                            text: "(?)"
                            font.pixelSize: _d.smallFontSize
                            color: _d.textSecondaryColor

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                            }

                            ToolTip.visible: mouseArea.containsMouse
                            ToolTip.delay: 500
                            ToolTip.text: "Select how the node discovers peers in the network"

                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: discoveryModeLayout.implicitHeight + 24
                        color: _d.surfaceColor
                        radius: 4
                        border.color: _d.borderColor
                        border.width: 1

                        RowLayout {
                            id: discoveryModeLayout
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 24

                            RadioButton {
                                id: extKadOnlyRadio
                                text: "ExtKadOnly"
                                checked: backend.discoveryMode === ChatBackend.ExtKadOnly
                                onClicked: backend.discoveryMode = ChatBackend.ExtKadOnly

                                indicator: Rectangle {
                                    implicitWidth: 18
                                    implicitHeight: 18
                                    x: parent.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: 9
                                    border.color: parent.checked ? _d.accentColor : _d.borderColor
                                    color: "transparent"

                                    Rectangle {
                                        width: 10
                                        height: 10
                                        x: 4
                                        y: 4
                                        radius: 5
                                        color: _d.accentColor
                                        visible: parent.parent.checked
                                    }
                                }

                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: _d.primaryFontSize
                                    color: _d.textColor
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: parent.indicator.width + parent.spacing
                                }

                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                                ToolTip.text: "Extended Kademlia Discovery - Uses DHT-based peer discovery. Discovers mixnodes automatically via Kademlia protocol."
                            }

                            RadioButton {
                                id: stdDiscoveryRadio
                                text: "StdDiscovery"
                                checked: backend.discoveryMode === ChatBackend.StdDiscovery
                                onClicked: backend.discoveryMode = ChatBackend.StdDiscovery

                                indicator: Rectangle {
                                    implicitWidth: 18
                                    implicitHeight: 18
                                    x: parent.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: 9
                                    border.color: parent.checked ? _d.accentColor : _d.borderColor
                                    color: "transparent"

                                    Rectangle {
                                        width: 10
                                        height: 10
                                        x: 4
                                        y: 4
                                        radius: 5
                                        color: _d.accentColor
                                        visible: parent.parent.checked
                                    }
                                }

                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: _d.primaryFontSize
                                    color: _d.textColor
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: parent.indicator.width + parent.spacing
                                }

                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                                ToolTip.text: "Standard Discovery - Uses Rendezvous and Peer Exchange protocols to find peers. Requires mixnodes to be configured manually."
                            }

                            RadioButton {
                                id: allDiscoveryRadio
                                text: "All"
                                checked: backend.discoveryMode === ChatBackend.All
                                onClicked: backend.discoveryMode = ChatBackend.All

                                indicator: Rectangle {
                                    implicitWidth: 18
                                    implicitHeight: 18
                                    x: parent.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: 9
                                    border.color: parent.checked ? _d.accentColor : _d.borderColor
                                    color: "transparent"

                                    Rectangle {
                                        width: 10
                                        height: 10
                                        x: 4
                                        y: 4
                                        radius: 5
                                        color: _d.accentColor
                                        visible: parent.parent.checked
                                    }
                                }

                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: _d.primaryFontSize
                                    color: _d.textColor
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: parent.indicator.width + parent.spacing
                                }

                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                                ToolTip.text: "All Discovery Methods - Combines Kademlia, Rendezvous, and Peer Exchange for maximum peer connectivity."
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }
                }

                // Store Node Section
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    RowLayout {
                        spacing: 8

                        Text {
                            text: "Store Node"
                            font.pixelSize: _d.primaryFontSize
                            font.bold: true
                            color: _d.textColor
                        }

                        Text {
                            text: "(?)"
                            font.pixelSize: _d.smallFontSize
                            color: _d.textSecondaryColor

                            ToolTip.visible: storeNodeHelpMouse.containsMouse
                            ToolTip.delay: 500
                            ToolTip.text: "Multiaddress of the store node used for retrieving message history."

                            MouseArea {
                                id: storeNodeHelpMouse
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: storeNodeInput.implicitHeight + 24
                        color: _d.surfaceColor
                        radius: 4
                        border.color: _d.borderColor
                        border.width: 1

                        TextField {
                            id: storeNodeInput
                            anchors.fill: parent
                            anchors.margins: 12
                            text: backend.storeNode
                            placeholderText: "/ip4/<ip>/tcp/<port>/p2p/<peerId>"
                            font.pixelSize: _d.smallFontSize
                            color: _d.textColor
                            verticalAlignment: TextInput.AlignVCenter
                            leftPadding: 8
                            rightPadding: 8

                            background: Rectangle {
                                color: _d.surfaceDarkColor
                                radius: 4
                                border.color: parent.activeFocus ? _d.accentColor : _d.borderColor
                                border.width: 1
                            }

                            onEditingFinished: {
                                backend.storeNode = text
                            }
                        }
                    }
                }

                // Bootstrap Nodes Section
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    RowLayout {
                        spacing: 8

                        Text {
                            text: "Bootstrap Nodes"
                            font.pixelSize: _d.primaryFontSize
                            font.bold: true
                            color: _d.textColor
                        }

                        Text {
                            text: "(?)"
                            font.pixelSize: _d.smallFontSize
                            color: _d.textSecondaryColor

                            ToolTip.visible: bootstrapHelpMouse.containsMouse
                            ToolTip.delay: 500
                            ToolTip.text: "Bootstrap peers to connect to initially. Optionally specify a Mix Public Key to use this node as a mixnode."

                            MouseArea {
                                id: bootstrapHelpMouse
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: _d.surfaceColor
                        radius: 4
                        border.color: _d.borderColor
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            ListView {
                                id: bootstrapListView
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: backend.bootstrapNodes
                                spacing: 6

                                delegate: Rectangle {
                                    width: bootstrapListView.width
                                    height: nodeColumn.implicitHeight + 16
                                    color: nodeMouseArea.containsMouse ? _d.hoverColor : _d.surfaceDarkColor
                                    radius: 4

                                    MouseArea {
                                        id: nodeMouseArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                    }

                                    ColumnLayout {
                                        id: nodeColumn
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 4

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 8

                                            Text {
                                                text: modelData.address || ""
                                                font.pixelSize: _d.smallFontSize
                                                color: _d.textColor
                                                elide: Text.ElideMiddle
                                                Layout.fillWidth: true

                                                ToolTip.visible: nodeTextMouse.containsMouse
                                                ToolTip.delay: 500
                                                ToolTip.text: modelData.address || ""

                                                MouseArea {
                                                    id: nodeTextMouse
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                }
                                            }

                                            Button {
                                                text: "X"
                                                Layout.preferredWidth: 28
                                                Layout.preferredHeight: 28

                                                onClicked: backend.removeBootstrapNode(index)

                                                ToolTip.visible: hovered
                                                ToolTip.delay: 500
                                                ToolTip.text: "Remove this bootstrap node"

                                                contentItem: Text {
                                                    text: parent.text
                                                    font.pixelSize: _d.smallFontSize
                                                    color: _d.statusErrorColor
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }

                                                background: Rectangle {
                                                    color: parent.pressed ? _d.hoverColor : "transparent"
                                                    radius: 4
                                                    border.color: _d.borderColor
                                                    border.width: 1
                                                }
                                            }
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 8

                                            Text {
                                                text: "Mix Key:"
                                                font.pixelSize: _d.smallFontSize
                                                color: _d.textSecondaryColor
                                                Layout.preferredWidth: 55

                                                ToolTip.visible: mixKeyLabelMouse.containsMouse
                                                ToolTip.delay: 500
                                                ToolTip.text: "Optional: Mixnode public key (hex). If provided, this node can be used as a mixnode for anonymous routing."

                                                MouseArea {
                                                    id: mixKeyLabelMouse
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                }
                                            }

                                            TextField {
                                                id: mixKeyInput
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 28
                                                text: modelData.mixPubKey || ""
                                                placeholderText: "(optional) hex public key"
                                                font.pixelSize: _d.smallFontSize - 1
                                                color: _d.textColor
                                                verticalAlignment: TextInput.AlignVCenter
                                                leftPadding: 8
                                                rightPadding: 8

                                                background: Rectangle {
                                                    color: _d.backgroundColor
                                                    radius: 3
                                                    border.color: parent.activeFocus ? _d.accentColor : _d.borderColor
                                                    border.width: 1
                                                }

                                                onEditingFinished: {
                                                    backend.updateBootstrapNodeMixKey(index, text)
                                                }

                                                ToolTip.visible: activeFocus
                                                ToolTip.delay: 1000
                                                ToolTip.text: "Enter the mixnode public key (64-char hex) to use this node for anonymous message routing"
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: _d.borderColor
                            }

                            // Add new node section
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        text: "Address:"
                                        font.pixelSize: _d.smallFontSize
                                        color: _d.textSecondaryColor
                                        Layout.preferredWidth: 55
                                    }

                                    TextField {
                                        id: newNodeInput
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 32
                                        placeholderText: "/ip4/<ip>/tcp/<port>/p2p/<peerId>"
                                        font.pixelSize: _d.smallFontSize
                                        color: _d.textColor
                                        verticalAlignment: TextInput.AlignVCenter
                                        leftPadding: 10
                                        rightPadding: 10

                                        background: Rectangle {
                                            color: _d.surfaceDarkColor
                                            radius: 4
                                            border.color: parent.activeFocus ? _d.accentColor : _d.borderColor
                                            border.width: 1
                                        }

                                        Keys.onReturnPressed: {
                                            if (text.trim() !== "") {
                                                backend.addBootstrapNode(text.trim(), newMixKeyInput.text.trim())
                                                text = ""
                                                newMixKeyInput.text = ""
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        text: "Mix Key:"
                                        font.pixelSize: _d.smallFontSize
                                        color: _d.textSecondaryColor
                                        Layout.preferredWidth: 55

                                        ToolTip.visible: newMixKeyLabelMouse.containsMouse
                                        ToolTip.delay: 500
                                        ToolTip.text: "Optional: Provide a mixnode public key if this node should also be used as a mixnode"

                                        MouseArea {
                                            id: newMixKeyLabelMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                        }
                                    }

                                    TextField {
                                        id: newMixKeyInput
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 32
                                        placeholderText: "(optional) hex public key"
                                        font.pixelSize: _d.smallFontSize
                                        color: _d.textColor
                                        verticalAlignment: TextInput.AlignVCenter
                                        leftPadding: 10
                                        rightPadding: 10

                                        background: Rectangle {
                                            color: _d.surfaceDarkColor
                                            radius: 4
                                            border.color: parent.activeFocus ? _d.accentColor : _d.borderColor
                                            border.width: 1
                                        }

                                        Keys.onReturnPressed: {
                                            if (newNodeInput.text.trim() !== "") {
                                                backend.addBootstrapNode(newNodeInput.text.trim(), text.trim())
                                                newNodeInput.text = ""
                                                text = ""
                                            }
                                        }
                                    }

                                    Button {
                                        text: "+ Add"
                                        Layout.preferredWidth: 70
                                        Layout.preferredHeight: 32
                                        enabled: newNodeInput.text.trim() !== ""

                                        onClicked: {
                                            if (newNodeInput.text.trim() !== "") {
                                                backend.addBootstrapNode(newNodeInput.text.trim(), newMixKeyInput.text.trim())
                                                newNodeInput.text = ""
                                                newMixKeyInput.text = ""
                                            }
                                        }

                                        ToolTip.visible: hovered
                                        ToolTip.delay: 500
                                        ToolTip.text: "Add a new bootstrap node (mix key is optional)"

                                        contentItem: Text {
                                            text: parent.text
                                            font.pixelSize: _d.smallFontSize
                                            color: parent.enabled ? _d.textColor : _d.textDisabledColor
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        background: Rectangle {
                                            color: parent.enabled ? (parent.pressed ? _d.hoverColor : _d.buttonColor) : _d.surfaceColor
                                            radius: 4
                                            border.color: parent.enabled ? _d.borderColor : _d.hoverColor
                                            border.width: 1
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Reset Peer ID Button
                Button {
                    text: "Reset Peer ID"
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 40

                    onClicked: resetPeerIdDialog.open()

                    ToolTip.visible: hovered
                    ToolTip.delay: 500
                    ToolTip.text: "Generate a new Peer ID and restart"

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: _d.primaryFontSize
                        font.bold: true
                        color: _d.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: parent.pressed ? _d.statusErrorColor : "#8c3d3d"
                        radius: 4
                        border.color: _d.statusErrorColor
                        border.width: 1
                    }
                }

                // Save Button
                Button {
                    text: "Save & Restart"
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 40

                    onClicked: {
                        backend.saveSettings()
                        restartDialog.open()
                    }

                    ToolTip.visible: hovered
                    ToolTip.delay: 500
                    ToolTip.text: "Save settings and restart the application to apply changes"

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: _d.primaryFontSize
                        font.bold: true
                        color: _d.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: parent.pressed ? _d.statusSuccessColor : "#3d8c40"
                        radius: 4
                        border.color: _d.statusSuccessColor
                        border.width: 1
                    }
                }
            }
        }
    }
}
