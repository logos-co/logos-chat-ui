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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: messageInput
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                placeholderText: "Type your message here..."
                enabled: backend.status === ChatBackend.Ready && backend.currentChannel !== ""
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
                         messageInput.text.trim() !== ""
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
}
