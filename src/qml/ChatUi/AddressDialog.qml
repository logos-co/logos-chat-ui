import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Modal dialog that shows this installation's own address for sharing, with a
// one-tap copy. Set addressText, then open(). Standalone.
LogosDialog {
    id: root

    // The address to display; set by the caller before open().
    property alias addressText: addressDisplay.text

    title: qsTr("My Address")
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
            text: qsTr("Share this address with others to start a conversation:")
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing.small

            LogosScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 120

                LogosTextArea {
                    id: addressDisplay
                    readOnly: true
                    color: Theme.palette.primary
                    font.family: ChatTheme.monoFont
                }
            }

            LogosIconButton {
                objectName: "copyMyAddressButton"
                size: 32
                iconSize: 16
                iconSource: Qt.resolvedUrl("icons/copy.png")
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Copy address")
                onClicked: root._copy()
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

    function _copy() {
        clipboard.copy(root.addressText);
        copiedTimer.restart();
    }
}
