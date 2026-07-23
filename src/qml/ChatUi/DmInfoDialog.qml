import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Modal dialog showing a direct conversation's details: the other person's
// address and the conversation id, each with a copy beside it. Set the
// properties, then open(). Standalone.
LogosDialog {
    id: root

    // The other participant's address. Empty while unknown, in which case the
    // dialog offers only the conversation id.
    property string peerAddress: ""
    // The conversation id, shown in full and copyable.
    property string conversationId: ""

    title: qsTr("Conversation details")
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
            //: Label above the other participant's address
            text: qsTr("Address")
            visible: root.peerAddress !== ""
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            Layout.fillWidth: true
        }

        RowLayout {
            visible: root.peerAddress !== ""
            Layout.fillWidth: true
            spacing: Theme.spacing.small

            LogosText {
                text: root.peerAddress
                textFormat: Text.PlainText
                color: Theme.palette.text
                font.family: ChatTheme.monoFont
                font.pixelSize: Theme.typography.secondaryText
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            LogosIconButton {
                objectName: "copyPeerAddressButton"
                size: 28
                iconSize: 14
                iconSource: Qt.resolvedUrl("icons/copy.png")
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Copy address")
                onClicked: d.copy(root.peerAddress)
                Layout.alignment: Qt.AlignVCenter
            }
        }

        LogosText {
            //: Label above the conversation's id
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
                objectName: "copyDmIdButton"
                size: 28
                iconSize: 14
                iconSource: Qt.resolvedUrl("icons/copy.png")
                Accessible.role: Accessible.Button
                //: Button that copies the conversation id to the clipboard
                Accessible.name: qsTr("Copy conversation ID")
                onClicked: d.copy(root.conversationId)
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

        function copy(value) {
            clipboard.copy(value);
            copiedTimer.restart();
        }
    }
}
