import QtQuick
import QtCore
import QtQuick.Layouts

import Logos.Theme

import ChatUi

// Entry view (metadata.json "view"). Instantiates the store, which is the sole
// reader of the host `logos` context property, and composes the panes, wiring
// their signals to the store's actions. All UI lives in the ChatUi module; this
// file is composition only.
Rectangle {
    id: root
    implicitWidth: 1000
    implicitHeight: 700
    color: Theme.palette.backgroundInset

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
    readonly property string selectedAvatarInitials: root.optimisticSelection ? root.optimisticSelection.avatarInitials : store.currentAvatarInitials
    readonly property int selectedAvatarRamp: root.optimisticSelection ? root.optimisticSelection.avatarRamp : store.currentAvatarRamp
    // Whether the models hold the selected conversation's data.
    readonly property bool selectionLoaded: store.loadedConversationId === root.selectedConversationId

    // Whether the conversation's details panel is showing, toggled from the
    // thread header and left as the user last set it.
    property bool detailsShown: false

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

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing.medium
        spacing: Theme.spacing.medium

        ColumnLayout {
            Layout.preferredWidth: 320
            Layout.minimumWidth: 260
            Layout.fillHeight: true
            spacing: Theme.spacing.medium

            ConversationsPane {
                id: conversationsPane
                Layout.fillWidth: true
                Layout.fillHeight: true
                conversationModel: store.conversationModel
                currentConversationId: root.selectedConversationId
                online: store.online
                onConversationSelected: function (conversation) {
                    root.pendingSelection = conversation;
                    store.selectConversation(conversation.conversationId);
                }
                onNewConversationRequested: newConvDialog.open()
                onNewGroupRequested: newGroupDialog.open()
            }

            AccountCard {
                Layout.fillWidth: true
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
            avatarInitials: root.selectedAvatarInitials
            avatarRamp: root.selectedAvatarRamp
            conversationId: root.selectedConversationId
            memberModel: store.memberModel
            memberCount: store.memberCount
            detailsShown: root.detailsShown
            hasConversation: root.selectedConversationId !== ""
            hasConversations: conversationsPane.count > 0
            online: store.online
            ready: root.selectionLoaded
            onMessageSubmitted: function (text) {
                store.sendMessage(text);
            }
            onDetailsRequested: root.detailsShown = !root.detailsShown
        }

        ColumnLayout {
            visible: root.selectedConversationId !== "" && (root.selectedIsGroup || root.detailsShown)
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            spacing: Theme.spacing.medium

            DetailsPanel {
                id: detailsPanel
                visible: root.detailsShown
                Layout.fillWidth: true
                isGroup: root.selectedIsGroup
                description: root.selectedDescription
                conversationId: root.selectedConversationId
                peerAddress: store.currentPeerAddress
                memberCount: store.memberCount
                pendingMemberCount: store.pendingMemberCount
                onCloseRequested: root.detailsShown = false
            }

            MembersPane {
                visible: root.selectedIsGroup
                Layout.fillWidth: true
                Layout.fillHeight: true
                memberModel: store.memberModel
                memberCount: store.memberCount
                online: store.online
                ready: root.selectionLoaded
                onAddMemberRequested: addMemberDialog.open()
            }
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
