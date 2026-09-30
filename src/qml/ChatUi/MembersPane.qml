pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// The group's roster card: who is in it, with the one action that grows it
// pinned to the foot so it stays reachable however long the list gets. Give it
// a member model and an online flag, and connect its signals.
Rectangle {
    id: root

    // The MemberListModel (roles: address, label, avatarInitials, avatarRamp,
    // isSelf, pending, removable).
    required property var memberModel
    // The roster's size, taken as a property because the model reaches the view
    // as a replica whose row count a non-view caller cannot read.
    property int memberCount: 0
    // Whether the backend is online; gates the add-member control and the
    // Remove entry.
    required property bool online
    // Whether the member model already holds this conversation's roster. It does
    // not for as long as a selection is being loaded, and the rows standing in
    // the model meanwhile are the ones left behind by the previous conversation.
    required property bool ready

    // Emitted when the user asks to add a member; the caller collects the address.
    signal addMemberRequested
    // Emitted when the user asks to remove a member, with the address and the
    // label of the row the menu was opened on.
    signal removeMemberRequested(string address, string label)

    // The roster model is a separate replica from the properties carrying the
    // selection, so its rows can land a moment after the conversation counts as
    // loaded. Wait that out before calling a roster empty.
    onReadyChanged: if (root.ready)
        settleTimer.restart()

    Timer {
        id: settleTimer
        interval: 300
    }

    implicitWidth: 280
    implicitHeight: 400
    radius: Theme.spacing.radiusXlarge
    color: Theme.palette.backgroundTertiary
    border.width: 1
    border.color: Theme.palette.borderSubtle

    QtObject {
        id: d

        // Copy an address and confirm it on the row that offered it, while that
        // row still shows it.
        function copy(row, address) {
            clipboard.copy(address);
            if (row && row.address === address)
                row.flashCopied();
        }

        // Copy the focused member's address, so a keyboard copy gets the same
        // feedback as the menu.
        function copyCurrent() {
            const member = memberList.currentItem as MemberDelegate;
            if (member)
                d.copy(member, member.address);
        }
    }

    ClipboardProxy {
        id: clipboard
    }

    // One menu serves every row: a roster holds far more members than the menu
    // has entries.
    LogosMenu {
        id: memberMenu
        objectName: "memberMenu"

        // The row that asked for the menu, so its copy confirms where it was
        // asked for.
        property MemberDelegate row: null
        // The member the menu acts on, taken when it opens: by the time an
        // entry is chosen, a reused delegate can be showing someone else.
        property string address: ""
        property string label: ""
        property bool removable: false

        LogosMenuItem {
            objectName: "copyMemberAddressMenuItem"
            //: Menu entry that copies a member's address
            text: qsTr("Copy address")
            onTriggered: d.copy(memberMenu.row, memberMenu.address)
        }

        LogosMenuItem {
            objectName: "removeMemberMenuItem"
            // A hidden entry still takes its row in the menu, and the keyboard
            // still stops on it unless it is disabled too.
            visible: memberMenu.removable
            height: visible ? implicitHeight : 0
            enabled: root.online && memberMenu.removable
            //: Menu entry that asks the group to remove a member
            text: qsTr("Remove")
            onTriggered: root.removeMemberRequested(memberMenu.address, memberMenu.label)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.medium
        anchors.bottomMargin: Theme.spacing.medium
        anchors.leftMargin: Theme.spacing.large
        anchors.rightMargin: Theme.spacing.large
        spacing: Theme.spacing.small

        PanelHeader {
            Layout.fillWidth: true
            title: qsTr("Members")
            count: root.memberCount
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: memberList
                objectName: "memberList"
                anchors.fill: parent
                // Bled past the card's padding, so a row's hover fill has air
                // around its text instead of being cropped to it.
                anchors.leftMargin: -Theme.spacing.small
                anchors.rightMargin: -Theme.spacing.small
                clip: true
                focus: true
                reuseItems: true
                model: root.memberModel
                visible: root.ready

                Keys.onReturnPressed: d.copyCurrent()
                Keys.onEnterPressed: d.copyCurrent()

                ScrollBar.vertical: LogosScrollBar {}

                delegate: MemberDelegate {
                    id: memberRow
                    width: ListView.view.width
                    onContextMenuRequested: {
                        memberMenu.row = memberRow;
                        memberMenu.address = memberRow.address;
                        memberMenu.label = memberRow.label;
                        memberMenu.removable = memberRow.removable;
                        memberMenu.popup();
                    }
                }
            }

            EmptyState {
                objectName: "memberEmptyState"
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.spacing.large
                visible: root.ready && memberList.count === 0 && !settleTimer.running
                text: qsTr("No members yet")
            }
        }

        // A rule the full width of the card, so the action reads as pinned to
        // its foot rather than as the list's last row.
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: -Theme.spacing.large
            Layout.rightMargin: -Theme.spacing.large
            Layout.topMargin: Theme.spacing.tiny
            implicitHeight: 1
            color: Theme.palette.borderSubtle
        }

        LogosButton {
            objectName: "addMemberButton"
            //: Button that opens the dialog to invite a new group member
            text: qsTr("Add member")
            radius: Theme.spacing.radiusLarge
            font.pixelSize: Theme.typography.primaryText
            leadingIcon.source: Qt.resolvedUrl("icons/add-member.png")
            leadingIcon.size: 18
            leadingIcon.color: Theme.palette.text
            enabled: root.online
            onClicked: root.addMemberRequested()
            Layout.fillWidth: true
            Layout.preferredHeight: 40
        }
    }
}
