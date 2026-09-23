import QtQml

// Stand-in for the replica type the host registers from src/ChatBackend.rep:
// its enum, properties, slots and signals as plain QML a scene sets, and a
// selection that loads at once. tests/cpp's tst_scenemock fails when the two
// surfaces differ.
QtObject {
    enum ChatStatus {
        Stopped,
        Initialising,
        Online,
        Error
    }

    property int chatStatus: ChatBackend.Stopped
    property bool deliveryAdopted: false
    property string deliveryPreset: "logos.test"
    property string myAddress: ""
    property string myLabel: ""
    property string myInitials: ""
    property string currentConversationId: ""
    property string loadedConversationId: ""
    property bool currentIsGroup: false
    property string currentDisplayName: ""
    property string currentDescription: ""
    property string currentAvatarInitials: ""
    property int currentAvatarRamp: 0
    property int memberCount: 0
    property int pendingMemberCount: 0
    property string currentPeerAddress: ""
    property string logDir: ""
    property var logRuns: []
    property var errors: []

    signal sendFailed(string conversationId, string content)
    signal error(string message)

    function createConversation(peerAddress: string) {
    }
    function createGroupConversation(name: string, description: string) {
    }
    function addGroupMember(conversationId: string, peerAddress: string) {
    }
    function sendMessage(conversationId: string, content: string) {
    }
    function selectConversation(conversationId: string) {
        currentConversationId = conversationId;
        loadedConversationId = conversationId;
    }
    function refreshMembers() {
    }
    function refreshSessionLogs() {
    }
}
