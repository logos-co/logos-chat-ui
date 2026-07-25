import QtQuick
import QtCore
import QtQuick.Layouts

import Logos.Theme

import ChatUi

// Entry view (metadata.json "view"). Instantiates the store, which is the sole
// reader of the host `logos` context property, and composes the panes, wiring
// their signals to the store's actions. All UI lives in the ChatUi module; this
// file is composition only.
Item {
    id: root
    implicitWidth: 900
    implicitHeight: 650

    // The row the user just picked, carrying its own data so the sidebar and the
    // header move in the same frame as the click instead of waiting for the
    // backend. It stops applying as soon as the backend reports that
    // conversation loaded, from when on the store is the truth again.
    property var pendingSelection: null

    readonly property var optimisticSelection: root.pendingSelection && store.loadedConversationId !== root.pendingSelection.conversationId ? root.pendingSelection : null

    readonly property string selectedConversationId: root.optimisticSelection ? root.optimisticSelection.conversationId : store.currentConversationId
    readonly property string selectedDisplayName: root.optimisticSelection ? root.optimisticSelection.displayName : store.currentDisplayName
    readonly property string selectedDescription: root.optimisticSelection ? root.optimisticSelection.description : store.currentDescription
    readonly property bool selectedIsGroup: root.optimisticSelection ? root.optimisticSelection.isGroup : store.currentIsGroup
    // Whether the models hold the selected conversation's data.
    readonly property bool selectionLoaded: store.loadedConversationId === root.selectedConversationId

    ChatStore {
        id: store
    }

    Connections {
        target: store
        function onErrorOccurred(message) {
            toast.show(message);
        }
        function onSendFailed(conversationId, content) {
            threadPane.restoreFailedSend(conversationId, content);
        }
        // The backend switched somewhere else (a new conversation opening, the
        // selected one going away), which retires the pending row.
        function onCurrentConversationIdChanged() {
            if (root.pendingSelection && store.currentConversationId !== root.pendingSelection.conversationId)
                root.pendingSelection = null;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ColumnLayout {
                Layout.preferredWidth: 260
                Layout.minimumWidth: 200
                Layout.fillHeight: true
                spacing: 0

                ConversationsPane {
                    id: conversationsPane
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    conversationModel: store.conversationModel
                    currentConversationId: root.selectedConversationId
                    online: store.online
                    onConversationSelected: function (conversationId, displayName, isGroup, description) {
                        root.pendingSelection = {
                            conversationId: conversationId,
                            displayName: displayName,
                            isGroup: isGroup,
                            description: description
                        };
                        store.selectConversation(conversationId);
                    }
                    onNewConversationRequested: newConvDialog.open()
                    onNewGroupRequested: newGroupDialog.open()
                }

                AccountCard {
                    Layout.fillWidth: true
                    Layout.margins: Theme.spacing.small
                    address: store.myAddress
                    label: store.myLabel
                    initials: store.myInitials
                    online: store.online
                    statusLabel: store.statusLabel
                }
            }

            MessageThreadPane {
                id: threadPane
                Layout.fillWidth: true
                Layout.fillHeight: true
                messageModel: store.messageModel
                currentIsGroup: root.selectedIsGroup
                title: root.selectedDisplayName
                description: root.selectedDescription
                conversationId: root.selectedConversationId
                hasConversation: root.selectedConversationId !== ""
                hasConversations: conversationsPane.count > 0
                online: store.online
                ready: root.selectionLoaded
                onMessageSubmitted: function (text) {
                    store.sendMessage(text);
                }
                onDetailsRequested: {
                    if (root.selectedIsGroup)
                        groupInfoDialog.open();
                    else
                        dmInfoDialog.open();
                }
            }

            MembersPane {
                visible: root.selectedIsGroup
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                memberModel: store.memberModel
                online: store.online
                ready: root.selectionLoaded
                onAddMemberRequested: addMemberDialog.open()
            }
        }

        StatusBar {
            Layout.fillWidth: true
            statusMessage: store.statusMessage
            statusLabel: store.statusLabel
            online: store.online
            hasError: store.hasError
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

    GroupInfoDialog {
        id: groupInfoDialog
        groupName: store.currentDisplayName
        description: store.currentDescription
        memberCount: store.memberCount
        pendingMemberCount: store.pendingMemberCount
        conversationId: store.currentConversationId
    }

    DmInfoDialog {
        id: dmInfoDialog
        peerAddress: store.currentPeerAddress
        conversationId: store.currentConversationId
    }

    AddMemberDialog {
        id: addMemberDialog
        onAddressEntered: function (address) {
            // First member add: explain the async commit delay first, then invite.
            if (chatPrefs.memberAddExplained) {
                store.addMember(address);
            } else {
                memberAddInfoDialog.pendingAddress = address;
                memberAddInfoDialog.open();
            }
        }
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
