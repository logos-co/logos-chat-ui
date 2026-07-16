import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// A group-roster row: the member label with a self marker, revealing the full
// address on hover and requesting a copy on click. A pending member (invited but
// not yet committed to the roster) dims and shows a busy spinner. Delegate roles
// bind by name.
LogosItemDelegate {
    id: root

    required property string label
    required property string address
    required property bool isSelf
    // Invited but not yet committed into the group's roster.
    required property bool pending

    signal copyRequested(string address)

    // True briefly after a copy, so the tooltip can confirm it.
    readonly property bool copiedFlashing: copiedTimer.running

    width: 200
    implicitHeight: 40
    radius: 0

    Accessible.role: Accessible.ListItem
    Accessible.name: label

    onClicked: {
        root.copyRequested(root.address);
        root.flashCopied();
    }

    // Flash a brief "Copied" confirmation in the tooltip.
    function flashCopied() {
        copiedTimer.restart();
    }

    Timer {
        id: copiedTimer
        interval: 1500
    }

    contentItem: RowLayout {
        spacing: Theme.spacing.small

        LogosText {
            text: root.label
            textFormat: Text.PlainText
            color: root.pending ? Theme.palette.textSecondary : Theme.palette.text
            font.pixelSize: Theme.typography.secondaryText
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        LogosSpinner {
            visible: root.pending
            running: root.pending
            ringColor: Theme.palette.border
            thickness: 2
            dotSize: 5
            Layout.preferredWidth: 14
            Layout.preferredHeight: 14
            Layout.alignment: Qt.AlignVCenter
        }

        LogosText {
            visible: root.isSelf
            //: Badge marking the current user in the member list
            text: qsTr("you")
            color: Theme.palette.primary
            font.pixelSize: Theme.typography.badgeText
            Layout.alignment: Qt.AlignVCenter
        }
    }

    LogosToolTip {
        //: Tooltip on a member still being committed into the group
        text: root.pending ? qsTr("waiting for commit") : (root.copiedFlashing ? qsTr("Copied") : root.address)
        placement: LogosToolTip.Left
        visible: (root.hovered || root.copiedFlashing) && (root.pending || root.address !== "")
    }
}
