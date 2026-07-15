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

    signal conversationSelected(string conversationId)
    signal newConversationRequested
    signal newGroupRequested
    signal showAddressRequested

    // Exposed for the exchange doc-test's inspector hooks.
    property alias count: convList.count

    implicitWidth: 260
    color: Theme.palette.backgroundInset

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
                id: groupButton
                implicitWidth: 68
                implicitHeight: 30
                //: Button that starts a new group conversation
                text: qsTr("Group")
                enabled: root.online
                onClicked: root.newGroupRequested()
                Layout.alignment: Qt.AlignVCenter

                LogosToolTip {
                    text: qsTr("New group conversation")
                    placement: LogosToolTip.Bottom
                    visible: groupButton.hovered
                }
            }
            LogosButton {
                id: newButton
                implicitWidth: 56
                implicitHeight: 30
                //: Button that starts a new one-to-one conversation
                text: qsTr("New")
                enabled: root.online
                onClicked: root.newConversationRequested()
                Layout.alignment: Qt.AlignVCenter

                LogosToolTip {
                    text: qsTr("New conversation")
                    placement: LogosToolTip.Bottom
                    visible: newButton.hovered
                }
            }
        }

        ListView {
            id: convList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            reuseItems: true
            model: root.conversationModel
            currentIndex: -1

            Keys.onReturnPressed: if (currentItem)
                root.conversationSelected((currentItem as ConversationDelegate).conversationId)
            Keys.onEnterPressed: if (currentItem)
                root.conversationSelected((currentItem as ConversationDelegate).conversationId)

            ScrollBar.vertical: LogosScrollBar {}

            delegate: ConversationDelegate {
                width: ListView.view.width
                currentConversationId: root.currentConversationId
                onActivated: function (conversationId) {
                    root.conversationSelected(conversationId);
                }
            }

            EmptyState {
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.spacing.large
                visible: convList.count === 0
                text: root.online ? qsTr("No conversations yet") : qsTr("Waiting for connection...")
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
