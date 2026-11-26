pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Modal dialog that collects the peer addresses to invite into the current group
// and emits them together, so they join through one commit. Return or + moves the
// typed address onto the list, and × drops one again. The field and the list
// persist across a close, so a dismissed dialog reopens with the same draft, and
// clear once the addresses are emitted. Standalone: instantiate with no context,
// open(), and read the signal.
LogosDialog {
    id: root

    // Emitted with the listed addresses, and the typed one if it is not listed
    // yet, when the user confirms at least one.
    signal addressesEntered(list<string> addresses)

    title: qsTr("Add members")
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: Overlay.overlay
    width: Math.min(480, (Overlay.overlay ? Overlay.overlay.width : 480) - 2 * Theme.spacing.large)

    leftActions: [
        LogosButton {
            implicitWidth: 96
            implicitHeight: 36
            text: qsTr("Cancel")
            onClicked: root.close()
        }
    ]
    rightActions: [
        LogosButton {
            objectName: "addMembersButton"
            implicitWidth: Math.max(96, implicitContentWidth + leftPadding + rightPadding)
            implicitHeight: 36
            //: Button that invites every listed address into the group
            text: d.invitees.length > 1 ? qsTr("Add %1 members").arg(d.invitees.length) : qsTr("Add")
            enabled: d.invitees.length > 0
            onClicked: d.accept()
        }
    ]

    onOpened: addressField.forceActiveFocus()

    contentItem: ColumnLayout {
        spacing: Theme.spacing.medium

        LogosText {
            text: qsTr("Paste the address of each person to invite, pressing Return after each:")
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: Theme.spacing.small
            Layout.fillWidth: true

            LogosTextField {
                id: addressField
                objectName: "addMemberField"
                placeholderText: qsTr("peer address (hex)...")
                textInput.font.family: Theme.typography.mono
                Layout.fillWidth: true
            }

            ChatIconButton {
                objectName: "listAddressButton"
                size: addressField.implicitHeight
                iconSize: 14
                iconSource: Qt.resolvedUrl("icons/plus.png")
                enabled: d.listable
                Accessible.role: Accessible.Button
                //: Button that moves the typed address onto the list of people to invite
                Accessible.name: qsTr("Add to the list")
                onClicked: d.list()
            }
        }

        LogosText {
            objectName: "alreadyListedHint"
            visible: d.duplicate
            //: Shown under the address field when the typed address is already on the list
            text: qsTr("Already on the list.")
            color: Theme.palette.textTertiary
            font.pixelSize: Theme.typography.secondaryText
            Layout.fillWidth: true
        }

        LogosListView {
            objectName: "inviteeList"
            model: d.listed
            visible: count > 0
            implicitHeight: Math.min(contentHeight, 200)
            Layout.fillWidth: true

            delegate: RowLayout {
                id: invitee

                required property string modelData
                required property int index

                width: ListView.view.width
                spacing: Theme.spacing.small

                LogosText {
                    objectName: "inviteeAddress"
                    text: invitee.modelData
                    font.family: Theme.typography.mono
                    font.pixelSize: Theme.typography.secondaryText
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                ChatIconButton {
                    objectName: "dropAddressButton"
                    size: 24
                    iconSize: 12
                    iconSource: Qt.resolvedUrl("icons/close.png")
                    Accessible.role: Accessible.Button
                    //: Button that takes an address off the list of people to invite
                    Accessible.name: qsTr("Remove from the list")
                    onClicked: d.drop(invitee.index)
                }
            }
        }
    }

    Connections {
        target: addressField.textInput
        function onAccepted() {
            d.list();
        }
    }

    QtObject {
        id: d

        // The addresses listed so far, in the order they were listed.
        property list<string> listed: []
        readonly property string draft: addressField.text.trim()
        readonly property bool duplicate: draft !== "" && listed.includes(draft)
        readonly property bool listable: draft !== "" && !duplicate
        // Who Add invites: the list, and the typed address if it is not on it.
        readonly property list<string> invitees: listable ? listed.concat([draft]) : listed

        // Both end with the field focused: the icon buttons take focus on press.
        function list() {
            if (!listable)
                return;
            listed = listed.concat([draft]);
            addressField.text = "";
            addressField.forceActiveFocus();
        }

        function drop(index) {
            listed = listed.filter((_, i) => i !== index);
            addressField.forceActiveFocus();
        }

        // Emit the invitees, start over, and close. No-op with no one to invite.
        function accept() {
            if (invitees.length === 0)
                return;
            root.addressesEntered(invitees);
            listed = [];
            addressField.text = "";
            root.close();
        }
    }
}
