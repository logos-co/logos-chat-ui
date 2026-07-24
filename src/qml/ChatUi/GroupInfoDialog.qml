import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Modal dialog showing a group's shared details: its name (the title), full
// description, member count, and conversation id with a one-tap copy. Set the
// properties, then open(). Standalone.
LogosDialog {
    id: root

    property string groupName: ""
    property string description: ""
    property int memberCount: 0
    // Members invited but not yet on the roster.
    property int pendingMemberCount: 0
    // The group's conversation id, shown in full and copyable.
    property string conversationId: ""

    // Roster size, with outstanding invitations named separately: an invited
    // member does not count towards the group until they join.
    readonly property string memberSummary: {
        const members = root.memberCount === 1 ? qsTr("1 member") : qsTr("%1 members").arg(root.memberCount);
        if (root.pendingMemberCount === 0)
            return members;
        const invited = root.pendingMemberCount === 1 ? qsTr("1 invited") : qsTr("%1 invited").arg(root.pendingMemberCount);
        return qsTr("%1, %2").arg(members).arg(invited);
    }

    title: root.groupName
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    width: Math.min(480, (Overlay.overlay ? Overlay.overlay.width : 480) - 2 * Theme.spacing.large)

    rightActions: [
        LogosButton {
            implicitWidth: 96
            implicitHeight: 36
            text: qsTr("Close")
            onClicked: root.close()
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

        LogosText {
            objectName: "descriptionText"
            Layout.fillWidth: true
            // A height-capped bare Text that elides; a scroll view here would
            // clip to its content size and shear the label below it.
            text: root.description !== "" ? root.description : qsTr("No description")
            textFormat: Text.PlainText
            color: root.description !== "" ? Theme.palette.text : Theme.palette.textTertiary
            font.pixelSize: Theme.typography.primaryText
            wrapMode: Text.WordWrap
            maximumLineCount: 5
            elide: Text.ElideRight
        }

        LogosText {
            text: root.memberSummary
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

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing.small

            LogosText {
                text: root.conversationId
                textFormat: Text.PlainText
                color: Theme.palette.text
                font.family: ChatTheme.monoFont
                font.pixelSize: Theme.typography.secondaryText
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            LogosIconButton {
                objectName: "copyGroupIdButton"
                size: 28
                iconSize: 14
                iconSource: Qt.resolvedUrl("icons/copy.png")
                Accessible.role: Accessible.Button
                //: Button that copies the group's conversation id to the clipboard
                Accessible.name: qsTr("Copy conversation ID")
                onClicked: d.copy()
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // Copy feedback, kept in the layout at all times (only its opacity
        // toggles) so revealing it never resizes the dialog.
        LogosText {
            id: copiedLabel
            objectName: "copiedLabel"
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

    QtObject {
        id: d

        function copy() {
            clipboard.copy(root.conversationId);
            copiedTimer.restart();
        }
    }
}
