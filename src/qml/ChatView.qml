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
    readonly property bool selectedHistoryOnly: root.optimisticSelection ? root.optimisticSelection.historyOnly : store.currentHistoryOnly
    // Whether the models hold the selected conversation's data.
    readonly property bool selectionLoaded: store.loadedConversationId === root.selectedConversationId
    // A group's roster, which a conversation from a previous session does not
    // have.
    readonly property bool rosterShown: root.selectedIsGroup && !root.selectedHistoryOnly

    // Whether the conversation's details panel is showing, toggled from the
    // thread header and left as the user last set it.
    property bool detailsShown: false

    // The newest failure, and how many nobody has looked at yet, which the
    // account card's icon carries. The count stands until the Errors tab is on
    // screen, which is what marks them seen; the failures themselves are
    // retained by the backend either way.
    property string lastError: ""
    property int unseenErrorCount: 0

    // Whether the developer tools are docked open, and how tall.
    property bool devtoolsOpen: false
    property real devtoolsHeight: devToolsPanel.implicitHeight
    readonly property bool errorsOnScreen: root.devtoolsOpen && devToolsPanel.currentTab === 0
    // The log files rotate and get pruned while the app runs, so the list is
    // read each time it comes on screen.
    readonly property bool logsOnScreen: root.devtoolsOpen && devToolsPanel.currentTab === 1

    onErrorsOnScreenChanged: {
        if (root.errorsOnScreen)
            root.unseenErrorCount = 0;
    }
    onLogsOnScreenChanged: {
        if (root.logsOnScreen)
            store.refreshLogRuns();
    }

    ChatStore {
        id: store
        debugDataShown: root.devtoolsOpen && devToolsPanel.conversationTab
    }

    Shortcut {
        sequence: "F12"
        onActivated: root.toggleDevtools()
    }

    Connections {
        target: store
        function onErrorOccurred(message) {
            root.lastError = message;
            if (!root.errorsOnScreen)
                root.unseenErrorCount += 1;
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
        anchors.margins: Theme.spacing.medium
        spacing: Theme.spacing.medium

        // Closing the notice replaces this binding, so the close lasts for this
        // run and is not remembered: a later run on a node Chat did not start
        // says so again.
        DeliveryNotice {
            Layout.fillWidth: true
            shown: store.deliveryAdopted
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing.medium

            ColumnLayout {
                // A layout nested in a layout fills by default, which would hand
                // the sidebar the slack meant for the thread.
                Layout.fillWidth: false
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
                    running: store.running
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
                    deliveryAdopted: store.deliveryAdopted
                    deliveryPreset: store.deliveryPreset
                    devtoolsOpen: root.devtoolsOpen
                    unseenErrors: root.unseenErrorCount
                    newestError: root.lastError
                    onUnknownNetworkRequested: unknownNetworkDialog.open()
                    onDevtoolsRequested: root.toggleDevtools()
                }
            }

            MessageThreadPane {
                id: threadPane
                Layout.fillWidth: true
                // The thread is what the window is for, so a window too narrow for
                // all three columns clips the right one rather than the thread.
                Layout.minimumWidth: 360
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
                running: store.running
                ready: root.selectionLoaded
                historyOnly: root.selectedHistoryOnly
                tracksDelivery: store.currentTracksDelivery
                // To the conversation on screen, which the backend may not have
                // switched to yet.
                onMessageSubmitted: function (text) {
                    store.sendMessage(root.selectedConversationId, text);
                }
                onDetailsRequested: root.detailsShown = !root.detailsShown
            }

            ColumnLayout {
                visible: root.selectedConversationId !== "" && (root.rosterShown || root.detailsShown)
                Layout.fillWidth: false
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
                    historyOnly: root.selectedHistoryOnly
                    protocol: store.currentProtocol
                    onCloseRequested: root.detailsShown = false
                }

                MembersPane {
                    id: membersPane
                    visible: root.rosterShown
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    memberModel: store.memberModel
                    memberCount: store.memberCount
                    running: store.running
                    ready: root.selectionLoaded
                    // Until the user opts out, the delay is explained before
                    // anyone is listed.
                    onAddMemberRequested: chatPrefs.memberAddExplained ? addMemberDialog.open() : memberAddInfoDialog.open()
                }

                // A column with nothing filling it centres what it holds, so
                // this takes the slack when no roster follows the details.
                Item {
                    visible: !membersPane.visible
                    Layout.fillHeight: true
                }
            }
        }

        // The grip fills the gap above the developer tools, and dragging it
        // resizes them: from the height their pages are laid out for, up to
        // half the view, so the chat stays the larger part.
        Item {
            objectName: "devtoolsGrip"
            visible: root.devtoolsOpen
            Layout.fillWidth: true
            Layout.topMargin: -Theme.spacing.medium
            Layout.bottomMargin: -Theme.spacing.medium
            implicitHeight: Theme.spacing.medium

            Rectangle {
                anchors.centerIn: parent
                width: 40
                height: 4
                radius: Theme.spacing.radiusPill
                color: gripHover.hovered || gripDrag.active ? Theme.palette.textTertiary : Theme.palette.borderStrong
            }

            HoverHandler {
                id: gripHover
                cursorShape: Qt.SizeVerCursor
            }

            DragHandler {
                id: gripDrag
                property real startHeight: 0
                target: null
                xAxis.enabled: false
                onActiveChanged: {
                    if (active)
                        startHeight = root.devtoolsHeight;
                }
                onActiveTranslationChanged: root.devtoolsHeight = Math.max(devToolsPanel.implicitHeight, Math.min(root.height / 2, startHeight - activeTranslation.y))
            }
        }

        DevToolsPanel {
            id: devToolsPanel
            visible: root.devtoolsOpen
            Layout.fillWidth: true
            Layout.preferredHeight: root.devtoolsHeight
            errors: store.errors
            runs: store.logRuns
            logDir: store.logDir
            conversationName: root.selectedConversationId !== "" ? root.selectedDisplayName : ""
            historyOnly: root.selectedHistoryOnly
            debugData: store.debugData
            onRefreshRequested: store.refreshDebugData()
            onCloseRequested: root.devtoolsOpen = false
        }
    }

    // Opening lands on Errors while a failure is unseen, which marks it seen,
    // and otherwise where the tools were left.
    function toggleDevtools() {
        if (!root.devtoolsOpen && root.unseenErrorCount > 0)
            devToolsPanel.currentTab = 0;
        root.devtoolsOpen = !root.devtoolsOpen;
    }

    UnknownNetworkDialog {
        id: unknownNetworkDialog
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
        onAddressesEntered: function (addresses) {
            store.addMembers(root.selectedConversationId, addresses);
        }
    }

    MemberAddInfoDialog {
        id: memberAddInfoDialog
        onConfirmed: function (dontShowAgain) {
            if (dontShowAgain)
                chatPrefs.memberAddExplained = true;
            addMemberDialog.open();
        }
    }

    // Persisted UI preferences.
    Settings {
        id: chatPrefs
        category: "chat_ui"
        property bool memberAddExplained: false
    }
}
