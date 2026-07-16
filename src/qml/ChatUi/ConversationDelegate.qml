import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// A conversation row: name (with a group marker), last-activity time and an
// unread-count badge. Delegate roles bind by name; set currentConversationId and
// connect activated(). Focus ring when keyboard-focused, highlight when current.
LogosItemDelegate {
    id: root

    required property string conversationId
    required property string displayName
    required property bool isGroup
    required property int unreadCount
    required property var lastActivity
    // The open conversation, so this row highlights when it is the current one.
    property string currentConversationId: ""

    signal activated(string conversationId)

    // True while this is the ListView's current item and the list has focus.
    readonly property bool keyboardFocused: ListView.isCurrentItem && ListView.view && ListView.view.activeFocus

    width: 200
    implicitHeight: 56
    radius: 0
    highlighted: conversationId === currentConversationId

    Accessible.role: Accessible.ListItem
    Accessible.name: {
        const base = isGroup ? qsTr("Group %1").arg(displayName) : displayName;
        return unreadCount > 0 ? qsTr("%1, %2 unread").arg(base).arg(unreadCount) : base;
    }
    Accessible.selected: highlighted

    onClicked: root.activated(root.conversationId)

    contentItem: RowLayout {
        spacing: Theme.spacing.small

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            LogosText {
                text: (root.isGroup ? "# " : "") + root.displayName
                textFormat: Text.PlainText
                color: Theme.palette.text
                font.pixelSize: Theme.typography.primaryText
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            LogosText {
                text: root.lastActivity ? Qt.formatTime(root.lastActivity, Locale.ShortFormat) : ""
                color: Theme.palette.textTertiary
                font.pixelSize: Theme.typography.badgeText
            }
        }

        // Unread badge: a pill that widens for multi-digit counts.
        Rectangle {
            visible: root.unreadCount > 0
            implicitWidth: Math.max(20, badgeText.implicitWidth + 12)
            implicitHeight: 20
            radius: height / 2
            color: Theme.palette.primary
            Layout.alignment: Qt.AlignVCenter

            LogosText {
                id: badgeText
                anchors.centerIn: parent
                text: root.unreadCount
                // Dark ink for legible contrast on the light primary fill; the
                // white text token drops below the readable ratio there.
                color: Theme.palette.background
                font.pixelSize: Theme.typography.badgeText
                font.weight: Theme.typography.weightBold
            }
        }
    }

    // Keyboard focus ring, drawn over the row without disturbing its background.
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        visible: root.keyboardFocused
        border.width: 1
        border.color: Theme.palette.focus
    }
}
