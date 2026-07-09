import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Standalone group-roster panel: give it a member model and an `online` flag,
// and wire its two signals. It reads no ambient context (no `logos` global, no
// ancestor properties), so it instantiates anywhere.
Rectangle {
    id: pane
    implicitWidth: 220

    // The MemberListModel (roles: address, label, isSelf).
    required property var memberModel
    // Whether the backend is online; gates the add-member controls.
    required property bool online

    // Emitted when the user asks to invite `address` into the group.
    signal addMemberRequested(string address)
    // Emitted when the user asks to reload the roster.
    signal refreshRequested()

    readonly property color bgSecondary: "#111111"
    readonly property color bgPanel:     "#161616"
    readonly property color bgPrimary:   "#0A0A0A"
    readonly property color border:      "#2a2a2a"
    readonly property color textPrimary: "#FAFAFA"
    readonly property color textSecond:  "#6B7280"
    readonly property color textTertiary:"#4B5563"
    readonly property color accent:      "#10B981"
    readonly property color accentHover: "#34D399"
    readonly property color accentPress: "#059669"

    color: bgSecondary

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: pane.bgPanel

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Text {
                    text: "members"
                    color: pane.accent
                    font.family: "JetBrains Mono"
                    font.pixelSize: 13
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "↻"
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 26
                    enabled: pane.online
                    font.pixelSize: 13
                    background: Rectangle {
                        radius: 4
                        color: parent.pressed ? "#333" : parent.hovered ? "#2a2a2a" : pane.bgPanel
                        border.color: pane.border
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? pane.textPrimary : pane.textTertiary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: pane.refreshRequested()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: pane.border
        }

        // Roster
        ListView {
            id: memberList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: pane.memberModel

            delegate: Item {
                width: memberList.width
                height: 40

                HoverHandler { id: rowHover }
                ToolTip.text: model.address
                ToolTip.visible: rowHover.hovered && model.address.length > 0
                ToolTip.delay: 400

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Text {
                        text: model.label || ""
                        color: pane.textPrimary
                        font.family: "JetBrains Mono"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        visible: model.isSelf === true
                        text: "you"
                        color: pane.accent
                        font.family: "JetBrains Mono"
                        font.pixelSize: 10
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: pane.border
                }
            }

            Text {
                anchors.centerIn: parent
                visible: memberList.count === 0
                text: "No members yet"
                color: pane.textTertiary
                font.family: "JetBrains Mono"
                font.pixelSize: 12
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: pane.border
        }

        // Add-member row
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: pane.bgSecondary

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                TextField {
                    id: addrInput
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: pane.online ? "peer address..." : "offline"
                    enabled: pane.online
                    font.family: "JetBrains Mono"
                    font.pixelSize: 12
                    color: pane.textPrimary
                    background: Rectangle {
                        radius: 4
                        color: pane.bgPanel
                        border.color: addrInput.activeFocus ? pane.accent : pane.border
                    }
                    onAccepted: addBtn.submit()
                }

                Button {
                    id: addBtn
                    Layout.preferredWidth: 36
                    Layout.fillHeight: true
                    text: "+"
                    enabled: pane.online && addrInput.text.trim() !== ""
                    font.family: "JetBrains Mono"
                    font.pixelSize: 16
                    font.bold: true

                    function submit() {
                        var address = addrInput.text.trim()
                        if (address === "") return
                        pane.addMemberRequested(address)
                        addrInput.text = ""
                    }

                    background: Rectangle {
                        radius: 4
                        color: parent.enabled
                               ? (parent.pressed ? pane.accentPress
                                  : parent.hovered ? pane.accentHover : pane.accent)
                               : pane.textTertiary
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#000000"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: submit()
                }
            }
        }
    }
}
