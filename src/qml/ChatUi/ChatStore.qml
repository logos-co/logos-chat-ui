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
    // Delivery's node runs, online or offline, so the actions work.
    readonly property bool running: online || (backend ? backend.chatStatus === ChatBackend.Offline : false)
    readonly property bool hasError: backend ? backend.chatStatus === ChatBackend.Error : false
    // Choosing the account Chat runs as, before init, as the account card
    // draws it: "choose", "waiting" for Accounts, "failed" with accountProblem,
    // "own" once Chat runs on an account of its own for want of an Accounts
    // app, and empty once settled.
    readonly property string accountStep: {
        if (!backend)
            return "";
        switch (backend.accountStep) {
        case ChatBackend.ChoosingAccount:
            return "choose";
        case ChatBackend.WaitingForAccounts:
            return "waiting";
        case ChatBackend.AccountFailed:
            return "failed";
        case ChatBackend.RunningWithoutAccounts:
            return "own";
        default:
            return "";
        }
    }
    readonly property string accountProblem: backend ? backend.accountProblem : ""
    // The delivery node: whether it was already running when Chat opened, and
    // the network Chat asks for when it starts the node itself.
    readonly property bool deliveryAdopted: backend ? backend.deliveryAdopted : false
    readonly property string deliveryPreset: backend ? backend.deliveryPreset : ""
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
    readonly property string currentPeerLabel: backend ? backend.currentPeerLabel : ""
    // The current conversation is from a previous session, kept for its
    // history only.
    readonly property bool currentHistoryOnly: backend ? backend.currentHistoryOnly : false
    // The protocol the current conversation runs ("GroupV3").
    readonly property string currentProtocol: backend ? backend.currentProtocol : ""
    // Only GroupV3 reports when another member received a message.
    readonly property bool currentTracksDelivery: currentProtocol === "GroupV3"
    // This account's own address, empty until the module has initialised, and
    // its short form.
    readonly property string myAddress: backend ? backend.myAddress : ""
    readonly property string myLabel: backend ? backend.myLabel : ""
    readonly property string myInitials: backend ? backend.myInitials : ""
    // The name this account set; empty when it set none.
    readonly property string myAccountName: backend ? backend.myAccountName : ""

    // The run's logs: every failure it reported, every run each writer kept, and
    // the directory they share.
    readonly property var errors: backend ? backend.errors : []
    readonly property var logRuns: backend ? backend.logRuns : []
    readonly property string logDir: backend ? backend.logDir : ""

    // Short connectivity label for the account card.
    readonly property string statusLabel: {
        if (!backend)
            return qsTr("No backend");
        if (accountStep === "waiting")
            return qsTr("Waiting for Accounts...");
        if (accountStep === "choose" || accountStep === "failed")
            return qsTr("Not started");
        switch (backend.chatStatus) {
        case ChatBackend.Stopped:
            return qsTr("Stopped");
        case ChatBackend.Initialising:
            return qsTr("Initialising...");
        case ChatBackend.Online:
            return qsTr("Online");
        case ChatBackend.Offline:
            return qsTr("Offline");
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

    // Actions. Discrete effects, so imperative here is appropriate.

    // Asks the Accounts app for the account to run as, by endorsing the key this
    // run's installation signs under; the backend reads the answer.
    function useAccount() {
        if (!backend)
            return;
        backend.awaitAccount();
        logos.request("accounts.signer.endorse", {
            key: backend.installationKey
        }, function (answer) {
            const address = answer.ok && answer.data && answer.data.address ? String(answer.data.address) : "";
            backend.settleAccount(address, answer.ok ? "" : answer.error);
        });
    }
    function continueWithoutAccount() {
        if (backend)
            backend.continueWithoutAccount();
    }
    function selectConversation(conversationId) {
        if (backend)
            backend.selectConversation(conversationId);
    }
    function sendMessage(conversationId, text) {
        if (backend && conversationId !== "")
            backend.sendMessage(conversationId, text);
    }
    function createConversation(address) {
        if (backend)
            backend.createConversation(address);
    }
    function createGroup(name, description) {
        if (backend)
            backend.createGroupConversation(name, description);
    }
    function addMember(conversationId, address) {
        if (backend && conversationId !== "")
            backend.addGroupMember(conversationId, address);
    }
    function refreshLogRuns() {
        if (backend)
            backend.refreshSessionLogs();
    }

    property Connections _backendSignals: Connections {
        target: root.backend
        function onError(message) {
            root.errorOccurred(message);
        }
        function onSendFailed(conversationId, content) {
            root.sendFailed(conversationId, content);
        }
    }
}
