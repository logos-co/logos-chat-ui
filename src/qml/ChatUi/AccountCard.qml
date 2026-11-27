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
    // The name this account set; empty when it set none.
    property string accountName: ""
    // A name on its way to the registry, and why the last one did not get there.
    property bool accountNameSaving: false
    property string accountNameProblem: ""
    // The name editor is open. The view closes it once the name is published.
    property bool editingName: false
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
    // A name to publish, trimmed.
    signal accountNameSubmitted(string name)

    // Whether the developer tools are open, and the failures nobody has looked
    // at yet, the newest of which the icon names on hover.
    property bool devtoolsOpen: false
    property int unseenErrors: 0
    property string newestError: ""
    // The developer tools icon was activated.
    signal devtoolsRequested

    // True briefly after a copy, so the tooltip can confirm it.
    readonly property bool copiedFlashing: copiedTimer.running

    // The editor opens on the current name, with no save started from it yet.
    onEditingNameChanged: if (editingName) {
        nameField.text = accountName;
        d.saveStarted = false;
        nameField.forceActiveFocus();
    }
    // Not at the click: the backend clears the last refusal as the save starts.
    onAccountNameSavingChanged: if (accountNameSaving)
        d.saveStarted = true

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

    QtObject {
        id: d

        // A save started from this opening of the editor, so the last refusal
        // is about it rather than about one given up on before.
        property bool saveStarted: false
        readonly property string problem: saveStarted ? root.accountNameProblem : ""
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

                RowLayout {
                    visible: root.label !== ""
                    spacing: Theme.spacing.small
                    Layout.fillWidth: true

                    LogosText {
                        objectName: "myNameText"
                        visible: root.accountName !== ""
                        text: root.accountName
                        textFormat: Text.PlainText
                        color: Theme.palette.text
                        font.pixelSize: Theme.typography.subtitleText
                        font.weight: Theme.typography.weightBold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        // Rounded up: the layout hands out whole pixels, and a text a fraction short elides.
                        Layout.maximumWidth: Math.ceil(implicitWidth)
                    }

                    // The name's place holds the way to set one.
                    LogosLink {
                        objectName: "addNameLink"
                        visible: root.accountName === "" && !root.editingName
                        text: qsTr("Add your name")
                        underline: false
                        labelItem.font.pixelSize: Theme.typography.subtitleText
                        labelItem.font.weight: Theme.typography.weightBold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.maximumWidth: implicitWidth
                        onActivated: root.editingName = true
                    }

                    LogosText {
                        objectName: "myLabelText"
                        text: root.label
                        textFormat: Text.PlainText
                        // Leads the row only while nothing stands before it.
                        readonly property bool leads: root.accountName === "" && root.editingName
                        color: leads ? Theme.palette.text : Theme.palette.textTertiary
                        font.family: Theme.typography.mono
                        font.pixelSize: leads ? Theme.typography.subtitleText : Theme.typography.secondaryText
                        font.weight: leads ? Theme.typography.weightBold : Theme.typography.weightRegular
                        elide: Text.ElideRight
                        Layout.alignment: Qt.AlignBaseline
                        Layout.fillWidth: true
                        // The name gives way first.
                        Layout.minimumWidth: implicitWidth
                    }

                    ChatIconButton {
                        id: editNameButton
                        objectName: "editNameButton"
                        visible: root.accountName !== "" && !root.editingName
                        size: 24
                        iconSize: 12
                        iconSource: Qt.resolvedUrl("icons/edit.png")
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Change my name")
                        onClicked: root.editingName = true
                        Layout.alignment: Qt.AlignVCenter

                        LogosToolTip {
                            text: qsTr("Change my name")
                            placement: LogosToolTip.Top
                            visible: editNameButton.hovered
                        }
                    }
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

            // On this card because the failures it carries belong to the run,
            // not to a conversation.
            ChatIconButton {
                id: devtoolsButton
                objectName: "devtoolsButton"
                Layout.alignment: Qt.AlignVCenter
                iconSize: 18
                lit: root.devtoolsOpen
                iconSource: Qt.resolvedUrl("icons/devtools.png")
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Developer tools")
                onClicked: root.devtoolsRequested()

                Rectangle {
                    objectName: "unseenErrorBadge"
                    visible: root.unseenErrors > 0
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: -4
                    anchors.topMargin: -4
                    width: Math.max(15, badgeLabel.implicitWidth + 8)
                    height: 15
                    radius: Theme.spacing.radiusPill
                    color: Theme.palette.error

                    LogosText {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: root.unseenErrors
                        font.pixelSize: Theme.typography.badgeText
                        font.weight: Theme.typography.weightBold
                        color: Theme.palette.backgroundBlack
                    }
                }

                LogosToolTip {
                    objectName: "devtoolsHint"
                    text: root.unseenErrors > 0 ? root.newestError : qsTr("Developer tools (F12)")
                    placement: LogosToolTip.Top
                    visible: devtoolsButton.hovered
                }
            }
        }

        // The name editor: what was typed, what happens to it, and the two ways out.
        ColumnLayout {
            visible: root.editingName
            // A hidden editor takes no keys.
            enabled: root.editingName
            spacing: Theme.spacing.small
            Layout.fillWidth: true

            LogosTextField {
                id: nameField
                objectName: "nameField"
                placeholderText: qsTr("Your name")
                // Read-only rather than disabled, so the focus survives the save.
                readOnly: root.accountNameSaving
                Layout.fillWidth: true
                Keys.onReturnPressed: if (saveButton.enabled)
                    saveButton.clicked()
                Keys.onEscapePressed: if (cancelButton.enabled)
                    cancelButton.clicked()
            }

            LogosText {
                objectName: "nameHint"
                text: d.problem !== "" ? d.problem : qsTr("Anyone with your address can see this name.")
                textFormat: Text.PlainText
                color: d.problem !== "" ? Theme.palette.warning : Theme.palette.textTertiary
                font.pixelSize: Theme.typography.secondaryText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: Theme.spacing.small
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                LogosButton {
                    id: cancelButton
                    objectName: "cancelNameButton"
                    compact: true
                    text: qsTr("Cancel")
                    enabled: !root.accountNameSaving
                    onClicked: root.editingName = false
                }

                LogosButton {
                    id: saveButton
                    objectName: "saveNameButton"
                    compact: true
                    variant: LogosButton.Variant.Primary
                    text: root.accountNameSaving ? qsTr("Saving...") : qsTr("Save")
                    enabled: !root.accountNameSaving && nameField.text.trim() !== "" && nameField.text.trim() !== root.accountName
                    onClicked: root.accountNameSubmitted(nameField.text.trim())
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
