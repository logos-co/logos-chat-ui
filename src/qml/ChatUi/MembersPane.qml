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
    // Whether the member model already holds this conversation's roster. It does
    // not for as long as a selection is being loaded, and the rows standing in
    // the model meanwhile are the ones left behind by the previous conversation.
    required property bool ready

    // Emitted when the user asks to add a member; the caller collects the address.
    signal addMemberRequested

    // The roster model is a separate replica from the properties carrying the
    // selection, so its rows can land a moment after the conversation counts as
    // loaded. Wait that out before calling a roster empty.
    onReadyChanged: if (root.ready)
        settleTimer.restart()

    Timer {
        id: settleTimer
        interval: 300
    }

    implicitWidth: 220
    color: Theme.palette.backgroundInset

    QtObject {
        id: d

        // Copy the focused member's address and flash the confirmation on its
        // row, so a keyboard copy gets the same feedback as a click.
        function copyCurrent() {
            const member = memberList.currentItem as MemberDelegate;
            if (!member)
                return;
            clipboard.copy(member.address);
            member.flashCopied();
        }
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

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: memberList
                objectName: "memberList"
                anchors.fill: parent
                clip: true
                focus: true
                reuseItems: true
                model: root.memberModel
                visible: root.ready

                Keys.onReturnPressed: d.copyCurrent()
                Keys.onEnterPressed: d.copyCurrent()

                ScrollBar.vertical: LogosScrollBar {}

                delegate: MemberDelegate {
                    width: ListView.view.width
                    onCopyRequested: function (address) {
                        clipboard.copy(address);
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
