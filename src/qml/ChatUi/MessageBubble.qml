import QtQuick

import Logos.Theme
import Logos.Controls

// A single chat message: a bubble anchored right for own messages, left for
// peers, with the sender's name above incoming group messages. Roles bind by name.
Item {
    id: root

    required property string content
    required property bool isMe
    required property var timestamp
    required property string sender
    // Whether the thread is a group; drives the sender label on incoming messages.
    property bool groupContext: false

    readonly property bool showSender: groupContext && !isMe

    implicitWidth: 200
    implicitHeight: bubble.y + bubble.height

    LogosText {
        id: senderLabel
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
        y: root.showSender ? senderLabel.height + Theme.spacing.tiny : 0
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

        Column {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacing.medium
            anchors.rightMargin: Theme.spacing.medium
            anchors.topMargin: Theme.spacing.small
            anchors.bottomMargin: Theme.spacing.small
            spacing: Theme.spacing.tiny

            LogosText {
                id: contentText
                width: parent.width
                text: root.content
                textFormat: Text.PlainText
                color: root.isMe ? ChatTheme.bubbleOwnText : ChatTheme.bubblePeerText
                font.pixelSize: Theme.typography.primaryText
                wrapMode: Text.Wrap
            }

            LogosText {
                id: timeText
                width: parent.width
                text: root.timestamp ? Qt.formatTime(root.timestamp, Locale.ShortFormat) : ""
                color: root.isMe ? Theme.colors.getColor(ChatTheme.bubbleOwnText, 0.5) : Theme.palette.textTertiary
                font.pixelSize: Theme.typography.secondaryText
                horizontalAlignment: root.isMe ? Text.AlignRight : Text.AlignLeft
            }
        }
    }
}
