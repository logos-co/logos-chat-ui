import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Modal dialog showing a group's shared details: its name (the title), full
// description, member count, and conversation id with a one-tap copy. Set the
// four properties, then open(). Standalone.
LogosDialog {
    id: root

    property string groupName: ""
    property string description: ""
    property int memberCount: 0
    // The group's conversation id, shown in full and copyable.
    property string conversationId: ""

    title: root.groupName
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    width: Math.min(480, (Overlay.overlay ? Overlay.overlay.width : 480) - 2 * Theme.spacing.large)

    leftActions: [
        LogosButton {
            implicitWidth: 96
            implicitHeight: 36
            text: qsTr("Close")
            onClicked: root.close()
        }
    ]
    rightActions: [
        LogosButton {
            implicitWidth: 120
            implicitHeight: 36
            //: Button that copies the group's conversation id to the clipboard
            text: qsTr("Copy ID")
            onClicked: root._copy()
        }
    ]

    contentItem: ColumnLayout {
        spacing: Theme.spacing.medium

        LogosText {
            //: Header for the group description section
            text: qsTr("Description")
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            Layout.fillWidth: true
        }

        LogosScrollView {
            id: descriptionScroll
            Layout.fillWidth: true
            Layout.preferredHeight: 80

            LogosText {
                width: descriptionScroll.width
                text: root.description !== "" ? root.description : qsTr("No description")
                textFormat: Text.PlainText
                color: root.description !== "" ? Theme.palette.text : Theme.palette.textTertiary
                font.pixelSize: Theme.typography.primaryText
                wrapMode: Text.WordWrap
            }
        }

        LogosText {
            //: Group member count; %n is the number of members
            text: qsTr("%n member(s)", "", root.memberCount)
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            Layout.fillWidth: true
        }

        LogosText {
            //: Label above the group's conversation id
            text: qsTr("Conversation ID")
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            Layout.fillWidth: true
        }

        LogosText {
            text: root.conversationId
            textFormat: Text.PlainText
            color: Theme.palette.text
            font.family: ChatTheme.monoFont
            font.pixelSize: Theme.typography.secondaryText
            elide: Text.ElideMiddle
            Layout.fillWidth: true
        }

        // Copy feedback, kept in the layout at all times (only its opacity
        // toggles) so revealing it never resizes the dialog.
        LogosText {
            id: copiedLabel
            text: qsTr("Copied to clipboard")
            color: Theme.palette.success
            font.pixelSize: Theme.typography.secondaryText
            opacity: copiedTimer.running ? 1 : 0
            Layout.fillWidth: true
        }

        Timer {
            id: copiedTimer
            interval: 2000
        }

        ClipboardProxy {
            id: clipboard
        }
    }

    // A reopened dialog starts without a stale confirmation.
    onOpened: copiedTimer.stop()

    // Copy the conversation id; the confirmation follows from the timer.
    function _copy() {
        clipboard.copy(root.conversationId);
        copiedTimer.restart();
    }
}
