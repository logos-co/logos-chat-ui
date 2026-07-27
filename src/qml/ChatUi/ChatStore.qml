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
    readonly property var logModel: typeof logos !== "undefined" && logos ? logos.model("chat_ui", "logModel") : null

    readonly property bool online: backend ? backend.chatStatus === ChatBackend.Online : false
    readonly property bool hasError: backend ? backend.chatStatus === ChatBackend.Error : false
    readonly property string currentConversationId: backend ? backend.currentConversationId : ""
    readonly property string loadedConversationId: backend ? backend.loadedConversationId : ""
    readonly property bool currentIsGroup: backend ? backend.currentIsGroup : false
    readonly property string currentDisplayName: backend ? backend.currentDisplayName : ""
    readonly property string currentDescription: backend ? backend.currentDescription : ""
    readonly property string currentAvatarInitials: backend ? backend.currentAvatarInitials : ""
    readonly property int currentAvatarRamp: backend ? backend.currentAvatarRamp : 0
    readonly property int memberCount: backend ? backend.memberCount : 0
    readonly property int pendingMemberCount: backend ? backend.pendingMemberCount : 0
    readonly property string currentPeerAddress: backend ? backend.currentPeerAddress : ""
    // This account's own address, empty until the backend is online, and its
    // short form.
    readonly property string myAddress: backend ? backend.myAddress : ""
    readonly property string myLabel: backend ? backend.myLabel : ""
    readonly property string myInitials: backend ? backend.myInitials : ""

    // The session log the host is capturing, and the console's account of it.
    readonly property string sessionLogPath: backend ? backend.sessionLogPath : ""
    readonly property string sessionLogSizeLabel: backend ? backend.sessionLogSizeLabel : ""
    readonly property int logLineCount: backend ? backend.logLineCount : 0
    readonly property int logErrorCount: backend ? backend.logErrorCount : 0
    readonly property int logShownCount: backend ? backend.logShownCount : 0
    readonly property var logDomains: backend ? backend.logDomains : []
    readonly property int logLevels: backend ? backend.logLevels : 0
    readonly property string logFilterText: backend ? backend.logFilterText : ""
    readonly property string logLastLine: backend ? backend.logLastLine : ""

    // The severities the console filters by, paired with the names the backend
    // spells them with. Declared here because this is the one file that sees
    // the backend's enum.
    readonly property var logLevelOptions: [
        {
            label: qsTr("debug"),
            name: "debug",
            value: ChatBackend.LevelDebug
        },
        {
            label: qsTr("info"),
            name: "info",
            value: ChatBackend.LevelInfo
        },
        {
            label: qsTr("warn"),
            name: "warning",
            value: ChatBackend.LevelWarning
        },
        {
            label: qsTr("error"),
            name: "error",
            value: ChatBackend.LevelError
        }
    ]

    // Short connectivity label for the account card.
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
    signal errorOccurred(string message)
    signal sendFailed(string conversationId, string content)
    signal logExported(string path)

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
    function addMember(address) {
        if (backend && currentConversationId !== "")
            backend.addGroupMember(currentConversationId, address);
    }
    function setLogDomainEnabled(domain, enabled) {
        if (backend)
            backend.setLogDomainEnabled(domain, enabled);
    }
    function setLogLevelEnabled(level, enabled) {
        if (backend)
            backend.setLogLevelEnabled(level, enabled);
    }
    function setLogFilterText(text) {
        if (backend)
            backend.setLogFilter(text);
    }
    function exportSessionLog() {
        if (backend)
            backend.exportSessionLog();
    }
    function exportFilteredLog() {
        if (backend)
            backend.exportFilteredLog();
    }

    property Connections _backendSignals: Connections {
        target: root.backend
        function onError(message) {
            root.errorOccurred(message);
        }
        function onSendFailed(conversationId, content) {
            root.sendFailed(conversationId, content);
        }
        function onLogExported(path) {
            root.logExported(path);
        }
    }
}
