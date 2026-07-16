import QtQuick
import QtCore
import QtQuick.Layouts

import ChatUi

// Entry view (metadata.json "view"). Instantiates the store, which is the sole
// reader of the host `logos` context property, and composes the panes, wiring
// their signals to the store's actions. All UI lives in the ChatUi module; this
// file is composition only.
Item {
    id: root
    implicitWidth: 900
    implicitHeight: 650

    ChatStore {
        id: store
    }

    Connections {
        target: store
        function onAddressReady(address) {
            addressDialog.addressText = address;
            addressDialog.open();
        }
        function onErrorOccurred(message) {
            toast.show(message);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ConversationsPane {
                id: conversationsPane
                Layout.preferredWidth: 260
                Layout.minimumWidth: 200
                Layout.fillHeight: true
                conversationModel: store.conversationModel
                currentConversationId: store.currentConversationId
                online: store.online
                onConversationSelected: function (conversationId) {
                    store.selectConversation(conversationId);
                }
                onNewConversationRequested: newConvDialog.open()
                onNewGroupRequested: newGroupDialog.open()
                onShowAddressRequested: store.requestMyAddress()
            }

            MessageThreadPane {
                id: threadPane
                Layout.fillWidth: true
                Layout.fillHeight: true
                messageModel: store.messageModel
                currentIsGroup: store.currentIsGroup
                title: store.currentDisplayName
                description: store.currentDescription
                conversationId: store.currentConversationId
                hasConversation: store.hasCurrentConversation
                online: store.online
                onMessageSubmitted: function (text) {
                    store.sendMessage(text);
                }
            }

            MembersPane {
                visible: store.currentIsGroup
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                memberModel: store.memberModel
                online: store.online
                onAddMemberRequested: function (address) {
                    // First member add: explain the async commit delay first, then invite.
                    if (chatPrefs.memberAddExplained) {
                        store.addMember(address);
                    } else {
                        memberAddInfoDialog.pendingAddress = address;
                        memberAddInfoDialog.open();
                    }
                }
            }
        }

        StatusBar {
            Layout.fillWidth: true
            statusMessage: store.statusMessage
            statusLabel: store.statusLabel
            online: store.online
            hasError: store.hasError
            identity: store.myIdentity
        }
    }

    NewConversationDialog {
        id: newConvDialog
        onAddressEntered: function (address) {
            store.createConversation(address);
        }
    }

    NewGroupDialog {
        id: newGroupDialog
        onGroupDetailsEntered: function (name, description) {
            store.createGroup(name, description);
        }
    }

    AddressDialog {
        id: addressDialog
    }

    MemberAddInfoDialog {
        id: memberAddInfoDialog
        // The address whose add opened the explainer, applied once confirmed.
        property string pendingAddress: ""
        onConfirmed: function (dontShowAgain) {
            if (dontShowAgain)
                chatPrefs.memberAddExplained = true;
            if (pendingAddress !== "")
                store.addMember(pendingAddress);
            pendingAddress = "";
        }
    }

    Toast {
        id: toast
    }

    // Persisted UI preferences.
    Settings {
        id: chatPrefs
        category: "chat_ui"
        property bool memberAddExplained: false
    }
}
