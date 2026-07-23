import QtQuick

// ChatBackend is host-registered at runtime and `logos` is host-injected; static
// analysis sees neither, so those import/unqualified warnings are disabled here.
// qmllint disable import unqualified
import Logos.ChatBackend 1.0

// The sole reader of the host `logos` context: resolves the backend and its
// models, exposes the view state as bindings, and wraps the actions. Everything
// else reads from here and stays standalone; tests pass a mock with this surface.
QtObject {
    id: root

    readonly property var backend: typeof logos !== "undefined" && logos ? logos.module("chat_ui") : null
    readonly property var conversationModel: typeof logos !== "undefined" && logos ? logos.model("chat_ui", "conversationModel") : null
    readonly property var messageModel: typeof logos !== "undefined" && logos ? logos.model("chat_ui", "messageModel") : null
    readonly property var memberModel: typeof logos !== "undefined" && logos ? logos.model("chat_ui", "memberModel") : null

    readonly property bool online: backend ? backend.chatStatus === ChatBackend.Online : false
    readonly property bool hasError: backend ? backend.chatStatus === ChatBackend.Error : false
    readonly property string currentConversationId: backend ? backend.currentConversationId : ""
    readonly property string loadedConversationId: backend ? backend.loadedConversationId : ""
    readonly property bool currentIsGroup: backend ? backend.currentIsGroup : false
    readonly property string currentDisplayName: backend ? backend.currentDisplayName : ""
    readonly property string currentDescription: backend ? backend.currentDescription : ""
    readonly property int memberCount: backend ? backend.memberCount : 0
    readonly property int pendingMemberCount: backend ? backend.pendingMemberCount : 0
    readonly property string statusMessage: backend ? backend.statusMessage : qsTr("No backend")
    readonly property string myIdentity: backend ? backend.myIdentity : ""

    // Short connectivity label for the status bar.
    readonly property string statusLabel: {
        if (!backend)
            return qsTr("No backend");
        switch (backend.chatStatus) {
        case ChatBackend.Stopped:
            return qsTr("Stopped");
        case ChatBackend.Initialising:
            return qsTr("Initialising...");
        case ChatBackend.Online:
            return qsTr("Online");
        case ChatBackend.Error:
            return qsTr("Error");
        default:
            return "";
        }
    }

    // Backend one-shot signals, relayed so the view never touches the backend
    // object directly.
    signal addressReady(string address)
    signal errorOccurred(string message)
    signal sendFailed(string conversationId, string content)

    // Actions. Discrete effects, so imperative here is appropriate.
    function selectConversation(conversationId) {
        if (backend)
            backend.selectConversation(conversationId);
    }
    function sendMessage(text) {
        if (backend && currentConversationId !== "")
            backend.sendMessage(currentConversationId, text);
    }
    function createConversation(address) {
        if (backend)
            backend.createConversation(address);
    }
    function createGroup(name, description) {
        if (backend)
            backend.createGroupConversation(name, description);
    }
    function requestMyAddress() {
        if (backend)
            backend.requestMyAddress();
    }
    function addMember(address) {
        if (backend && currentConversationId !== "")
            backend.addGroupMember(currentConversationId, address);
    }

    property Connections _backendSignals: Connections {
        target: root.backend
        function onAddressReady(address) {
            root.addressReady(address);
        }
        function onError(message) {
            root.errorOccurred(message);
        }
        function onSendFailed(conversationId, content) {
            root.sendFailed(conversationId, content);
        }
    }
}
