pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// The conversation sidebar: a header with new-chat and new-group actions plus an
// online indicator, the keyboard-navigable conversation list, and a
// show-my-address action. Data in via properties, intent out via signals.
Rectangle {
    id: root

    required property var conversationModel
    required property string currentConversationId
    required property bool online

    // Carries the selected row's own data, so a view can render the selection
    // before the backend has switched to it.
    signal conversationSelected(string conversationId, string displayName, bool isGroup, string description)
    signal newConversationRequested
    signal newGroupRequested
    signal showAddressRequested

    // Exposed for the exchange doc-test's inspector hooks.
    property alias count: convList.count

    implicitWidth: 260
    color: Theme.palette.backgroundInset

    QtObject {
        id: d

        // Select the keyboard-focused row, giving it the same effect as a click.
        function activateCurrent() {
            const row = convList.currentItem as ConversationDelegate;
            if (row)
                root.conversationSelected(row.conversationId, row.displayName, row.isGroup, row.description);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PaneHeader {
            Layout.fillWidth: true
            title: qsTr("Chat")

            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: root.online ? Theme.palette.success : Theme.palette.textMuted
                Layout.alignment: Qt.AlignVCenter
            }
            LogosButton {
                id: newButton
                implicitWidth: 84
                implicitHeight: 30
                //: Button that starts a new direct message (1:1) conversation
                text: qsTr("New DM")
                enabled: root.online
                onClicked: root.newConversationRequested()
                Layout.alignment: Qt.AlignVCenter

                LogosToolTip {
                    text: qsTr("New direct message")
                    placement: LogosToolTip.Bottom
                    visible: newButton.hovered
                }
            }
            LogosButton {
                id: groupButton
                implicitWidth: 84
                implicitHeight: 30
                //: Button that starts a new group conversation
                text: qsTr("New group")
                enabled: root.online
                onClicked: root.newGroupRequested()
                Layout.alignment: Qt.AlignVCenter

                LogosToolTip {
                    text: qsTr("New group conversation")
                    placement: LogosToolTip.Bottom
                    visible: groupButton.hovered
                }
            }
        }

        ListView {
            id: convList
            objectName: "conversationList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            reuseItems: true
            model: root.conversationModel
            currentIndex: -1

            Keys.onReturnPressed: d.activateCurrent()
            Keys.onEnterPressed: d.activateCurrent()

            ScrollBar.vertical: LogosScrollBar {}

            delegate: ConversationDelegate {
                width: ListView.view.width
                currentConversationId: root.currentConversationId
                onActivated: function (conversationId, displayName, isGroup, description) {
                    root.conversationSelected(conversationId, displayName, isGroup, description);
                }
            }

            EmptyState {
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.spacing.large
                visible: convList.count === 0
                text: root.online ? qsTr("No conversations yet. Start one with New DM, or share your address so someone can reach you.") : qsTr("Waiting for connection...")
            }
        }

        LogosButton {
            Layout.fillWidth: true
            Layout.margins: Theme.spacing.small
            implicitHeight: 40
            text: qsTr("Show My Address")
            enabled: root.online
            onClicked: root.showAddressRequested()
        }
    }
}
