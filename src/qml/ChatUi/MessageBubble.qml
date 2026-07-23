import QtQuick

import Logos.Theme
import Logos.Controls

// A single chat message: a bubble anchored right for own messages, left for
// peers, with the sender's name above incoming group messages. Roles bind by name.
Item {
    id: root

    required property string content
    required property bool isMe
    // Clock time for the bubble, formatted by the model.
    required property string timeDisplay
    required property string sender
    // Whether the thread is a group; drives the sender label on incoming messages.
    property bool groupContext: false
    // Neighbour-derived roles: suppress a repeated sender label within a run of
    // messages, and head a new calendar day with a separator above the bubble.
    required property bool sameSenderAsPrevious
    required property bool showDaySeparator
    required property string dayLabel

    readonly property bool showSender: groupContext && !isMe && !sameSenderAsPrevious
    // What a copy of this message takes: the selection when the user made one,
    // else the whole message.
    readonly property string copyText: contentText.selectedText !== "" ? contentText.selectedText : root.content

    // Emitted on a right-click, so the thread can offer its message actions.
    signal contextMenuRequested(string text)

    implicitWidth: 200
    implicitHeight: bubble.y + bubble.height

    Accessible.role: Accessible.StaticText
    Accessible.name: root.content
    Accessible.description: qsTr("%1, %2").arg(root.sender).arg(timeText.text)

    Item {
        id: daySeparator
        y: 0
        width: root.width
        visible: root.showDaySeparator
        height: root.showDaySeparator ? dayLabelText.implicitHeight + 2 * Theme.spacing.small : 0

        LogosText {
            id: dayLabelText
            anchors.centerIn: parent
            text: root.dayLabel
            textFormat: Text.PlainText
            color: Theme.palette.textTertiary
            font.pixelSize: Theme.typography.secondaryText
        }
    }

    LogosText {
        id: senderLabel
        y: daySeparator.height
        visible: root.showSender
        anchors.left: parent.left
        anchors.leftMargin: Theme.spacing.large
        text: root.sender
        textFormat: Text.PlainText
        color: Theme.palette.textSecondary
        font.pixelSize: Theme.typography.secondaryText
    }

    Rectangle {
        id: bubble
        objectName: "bubble"
        y: root.showSender ? senderLabel.y + senderLabel.height + Theme.spacing.tiny : daySeparator.height
        anchors.left: root.isMe ? undefined : parent.left
        anchors.right: root.isMe ? parent.right : undefined
        anchors.leftMargin: Theme.spacing.large
        anchors.rightMargin: Theme.spacing.large

        // Grow to the wider of the content and the timestamp (a short message
        // must not leave the time outside the bubble), capped at 70% of the row.
        width: Math.min(Math.max(contentText.implicitWidth, timeText.implicitWidth) + 2 * Theme.spacing.medium, root.width * 0.7)
        height: contentText.implicitHeight + timeText.implicitHeight + 2 * Theme.spacing.small + Theme.spacing.tiny
        radius: Theme.spacing.radiusLarge
        color: root.isMe ? ChatTheme.bubbleOwn : ChatTheme.bubblePeer
        border.width: root.isMe ? 0 : 1
        border.color: Theme.palette.borderSubtle

        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: root.contextMenuRequested(root.copyText)
        }

        Column {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacing.medium
            anchors.rightMargin: Theme.spacing.medium
            anchors.topMargin: Theme.spacing.small
            anchors.bottomMargin: Theme.spacing.small
            spacing: Theme.spacing.tiny

            SelectableText {
                id: contentText
                objectName: "messageText"
                width: parent.width
                text: root.content
                color: root.isMe ? ChatTheme.bubbleOwnText : ChatTheme.bubblePeerText
                selectionColor: root.isMe ? ChatTheme.bubbleOwnSelection : ChatTheme.bubblePeerSelection
                selectedTextColor: root.isMe ? ChatTheme.bubbleOwnSelectedText : ChatTheme.bubblePeerSelectedText
                font.pixelSize: Theme.typography.primaryText
            }

            LogosText {
                id: timeText
                width: parent.width
                text: root.timeDisplay
                color: root.isMe ? Theme.colors.getColor(ChatTheme.bubbleOwnText, 0.5) : Theme.palette.textTertiary
                font.pixelSize: Theme.typography.secondaryText
                horizontalAlignment: root.isMe ? Text.AlignRight : Text.AlignLeft
            }
        }
    }
}
