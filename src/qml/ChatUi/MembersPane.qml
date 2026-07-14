pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls
import Logos.Icons

// Group-roster panel: give it a member model and an online flag, and connect its
// signals. A row shows the member label and a self marker, reveals the full
// address on hover, and requests a copy on click or Return.
Rectangle {
    id: root

    // The MemberListModel (roles: address, label, isSelf).
    required property var memberModel
    // Whether the backend is online; gates the add-member controls.
    required property bool online

    // Emitted when the user asks to invite `address` into the group.
    signal addMemberRequested(string address)
    // Emitted when the user asks to reload the roster.
    signal refreshRequested

    implicitWidth: 220
    color: Theme.palette.backgroundInset

    ClipboardProxy {
        id: clipboard
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PaneHeader {
            Layout.fillWidth: true
            title: qsTr("Members")

            LogosIconButton {
                id: refreshButton
                size: 30
                iconSize: 16
                iconSource: LogosIcons.refresh
                enabled: root.online
                onClicked: root.refreshRequested()
                Layout.alignment: Qt.AlignVCenter

                LogosToolTip {
                    text: qsTr("Reload roster")
                    placement: LogosToolTip.Bottom
                    visible: refreshButton.hovered
                }
            }
        }

        ListView {
            id: memberList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            reuseItems: true
            model: root.memberModel

            Keys.onReturnPressed: if (currentItem)
                clipboard.copy((currentItem as MemberDelegate).address)
            Keys.onEnterPressed: if (currentItem)
                clipboard.copy((currentItem as MemberDelegate).address)

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

        SubmitRow {
            Layout.fillWidth: true
            placeholder: qsTr("peer address...")
            disabledPlaceholder: qsTr("offline")
            //: Button that invites the entered address into the group
            buttonText: qsTr("Add")
            submitEnabled: root.online
            onSubmitted: function (address) {
                root.addMemberRequested(address);
            }
        }
    }
}
