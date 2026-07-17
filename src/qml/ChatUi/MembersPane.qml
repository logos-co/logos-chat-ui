pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Group-roster panel: give it a member model and an online flag, and connect its
// signals. A row shows the member label and a self marker, reveals the full
// address on hover, and requests a copy on click or Return.
Rectangle {
    id: root

    // The MemberListModel (roles: address, label, isSelf).
    required property var memberModel
    // Whether the backend is online; gates the add-member controls.
    required property bool online

    // Emitted when the user asks to add a member; the caller collects the address.
    signal addMemberRequested

    implicitWidth: 220
    color: Theme.palette.backgroundInset

    // Copy the focused member's address and flash the confirmation on its row, so
    // a keyboard copy gets the same feedback as a click.
    function _copyCurrent() {
        const member = memberList.currentItem as MemberDelegate;
        if (!member)
            return;
        clipboard.copy(member.address);
        member.flashCopied();
    }

    ClipboardProxy {
        id: clipboard
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PaneHeader {
            Layout.fillWidth: true
            title: qsTr("Members")
        }

        ListView {
            id: memberList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            reuseItems: true
            model: root.memberModel

            Keys.onReturnPressed: root._copyCurrent()
            Keys.onEnterPressed: root._copyCurrent()

            ScrollBar.vertical: LogosScrollBar {}

            delegate: MemberDelegate {
                width: ListView.view.width
                onCopyRequested: function (address) {
                    clipboard.copy(address);
                }
            }

            EmptyState {
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.spacing.large
                visible: memberList.count === 0
                text: qsTr("No members yet")
            }
        }

        LogosButton {
            objectName: "addMemberButton"
            Layout.fillWidth: true
            Layout.margins: Theme.spacing.small
            implicitHeight: 40
            //: Button that opens the dialog to invite a new group member
            text: qsTr("Add member")
            enabled: root.online
            onClicked: root.addMemberRequested()
        }
    }
}
