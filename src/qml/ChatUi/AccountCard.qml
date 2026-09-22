import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls
import Logos.Icons

// This account's own card: its short identity, its connectivity and the delivery
// node it is on, and the full address with a one-tap copy, so what a peer needs to
// reach you is on screen rather than behind an action. Set the properties;
// standalone.
Rectangle {
    id: root

    // This account's own address. Empty until the module has initialised, which
    // leaves the card showing the connection state alone.
    required property string address
    // Short form of the address, the card's headline identity.
    required property string label
    // Two-letter form of the address, for the avatar.
    required property string initials
    required property bool online
    // Short connectivity label ("Online", "Initialising...").
    required property string statusLabel
    // The delivery node: whether it was already running when Chat opened, and
    // the network Chat asks for when it starts the node itself. Shown after the
    // status once online.
    required property bool deliveryAdopted
    required property string deliveryPreset

    // The unknown-network link after the status was activated.
    signal unknownNetworkRequested

    // True briefly after a copy, so the tooltip can confirm it.
    readonly property bool copiedFlashing: copiedTimer.running

    implicitWidth: 260
    implicitHeight: layout.implicitHeight + 2 * Theme.spacing.medium
    radius: Theme.spacing.radiusXlarge
    color: Theme.palette.backgroundTertiary
    border.width: 1
    border.color: Theme.palette.borderSubtle

    ClipboardProxy {
        id: clipboard
    }

    Timer {
        id: copiedTimer
        interval: 2000
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.spacing.medium
        spacing: Theme.spacing.small

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing.small

            Avatar {
                // The ramp is unused for this account, which takes the brand one.
                ramp: 0
                isSelf: true
                initials: root.initials
                showPresence: true
                online: root.online
                Layout.alignment: Qt.AlignVCenter
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                LogosText {
                    objectName: "myLabelText"
                    visible: root.label !== ""
                    text: root.label
                    textFormat: Text.PlainText
                    color: Theme.palette.text
                    font.family: Theme.typography.mono
                    font.pixelSize: Theme.typography.subtitleText
                    font.weight: Theme.typography.weightBold
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: Theme.spacing.tiny
                    Layout.fillWidth: true

                    LogosText {
                        text: root.statusLabel
                        textFormat: Text.PlainText
                        color: root.online ? Theme.palette.success : Theme.palette.textTertiary
                        font.pixelSize: Theme.typography.secondaryText
                        font.weight: Theme.typography.weightMedium
                        elide: Text.ElideRight
                        Layout.fillWidth: !root.online
                    }

                    LogosText {
                        visible: root.online
                        text: "·"
                        color: Theme.palette.textTertiary
                        font.pixelSize: Theme.typography.secondaryText
                    }

                    LogosText {
                        objectName: "deliveryNetworkName"
                        visible: root.online && !root.deliveryAdopted
                        text: root.deliveryPreset
                        textFormat: Text.PlainText
                        color: Theme.palette.textTertiary
                        font.pixelSize: Theme.typography.secondaryText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.maximumWidth: implicitWidth

                        HoverHandler {
                            id: networkNameHover
                        }

                        LogosToolTip {
                            objectName: "deliveryNetworkHint"
                            text: qsTr("Chat started delivery with its defaults (%1). Peers reach you when their delivery is on the same network.").arg(root.deliveryPreset)
                            placement: LogosToolTip.Top
                            visible: networkNameHover.hovered
                        }
                    }

                    LogosIcon {
                        visible: networkLink.visible
                        source: LogosIcons.warning
                        color: Theme.palette.warning
                        // The asset is a dark silhouette, which tints near-black.
                        brightness: 1.0
                        Layout.preferredWidth: 12
                        Layout.preferredHeight: 12
                    }

                    // A node Chat did not start is marked, since its network is
                    // unknown and may not be the one Chat's peers are on.
                    LogosLink {
                        id: networkLink
                        objectName: "deliveryNetworkLink"
                        visible: root.online && root.deliveryAdopted
                        text: qsTr("network unknown")
                        linkColor: Theme.palette.warning
                        hoverColor: Theme.palette.warningHover
                        elide: Text.ElideRight
                        labelItem.font.pixelSize: Theme.typography.secondaryText
                        // As wide as its text, so blank card space takes no clicks.
                        Layout.fillWidth: true
                        Layout.maximumWidth: implicitWidth
                        onActivated: root.unknownNetworkRequested()

                        LogosToolTip {
                            text: qsTr("What this means")
                            placement: LogosToolTip.Top
                            visible: networkLink.hovered
                        }
                    }
                }
            }
        }

        // The address gets a field's surface rather than a label's, because it
        // is a value to be taken rather than a line to be read.
        Rectangle {
            visible: root.address !== ""
            Layout.fillWidth: true
            implicitHeight: 28
            radius: Theme.spacing.radiusMedium
            color: Theme.palette.backgroundMuted
            border.width: 1
            border.color: Theme.palette.borderSubtle

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacing.small
                anchors.rightMargin: Theme.spacing.tiny
                spacing: Theme.spacing.tiny

                // The confirmation lands on the value itself, so a copy reads
                // as done without hovering anything.
                LogosText {
                    objectName: "myAddressText"
                    text: root.copiedFlashing ? qsTr("Copied to clipboard") : root.address
                    textFormat: Text.PlainText
                    color: root.copiedFlashing ? Theme.palette.success : Theme.palette.textTertiary
                    font.family: root.copiedFlashing ? Theme.typography.publicSans : Theme.typography.mono
                    font.pixelSize: Theme.typography.secondaryText
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                ChatIconButton {
                    id: copyButton
                    objectName: "copyMyAddressButton"
                    size: 24
                    iconSize: 12
                    iconSource: Qt.resolvedUrl("icons/copy.png")
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Copy my address")
                    onClicked: {
                        clipboard.copy(root.address);
                        copiedTimer.restart();
                    }
                    Layout.alignment: Qt.AlignVCenter

                    LogosToolTip {
                        text: qsTr("Copy my address")
                        placement: LogosToolTip.Top
                        visible: copyButton.hovered
                    }
                }
            }
        }
    }
}
