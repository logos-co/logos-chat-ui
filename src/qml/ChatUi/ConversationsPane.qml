pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// The conversation sidebar: a header with a menu of conversations to start, the
// keyboard-navigable conversation list, and a share-my-address action at the
// foot. Data in via properties, intent out via signals.
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

            // The width is explicit because the design system's button keeps a
            // fixed implicit size regardless of its label.
            // TODO(logos-design-system): drop it once buttons size to content.
            LogosButton {
                id: newButton
                objectName: "newMenuButton"
                implicitWidth: 84
                implicitHeight: 30
                //: Button that opens the menu of conversations one can start; the
                //: plus marks it as a create action
                text: qsTr("+ New")
                enabled: root.online
                onClicked: newMenu.popup(0, newButton.height)
                Layout.alignment: Qt.AlignVCenter

                LogosMenu {
                    id: newMenu
                    objectName: "newMenu"

                    LogosMenuItem {
                        objectName: "newDmMenuItem"
                        //: Menu entry that starts a new direct message (1:1) conversation
                        text: qsTr("Direct message")
                        onTriggered: root.newConversationRequested()
                    }
                    LogosMenuItem {
                        objectName: "newGroupMenuItem"
                        //: Menu entry that starts a new group conversation
                        text: qsTr("Group")
                        onTriggered: root.newGroupRequested()
                    }
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
                text: root.online ? qsTr("No conversations yet. Use New to start one, or share your address so someone can reach you.") : qsTr("Waiting for connection...")
            }
        }

        LogosButton {
            objectName: "showAddressButton"
            Layout.fillWidth: true
            Layout.margins: Theme.spacing.small
            implicitHeight: 40
            //: Button that shows this installation's own address for sharing
            text: qsTr("My address")
            enabled: root.online
            onClicked: root.showAddressRequested()
        }
    }
}
