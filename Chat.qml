import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "components"

// Top-level Chat UI App.
//
// Layout: IdentityPanel (top-left) | ConversationList (left) || MessageView (center)
//         InputBar (bottom-center)
//
// Polls chat_module every 200 ms via drain_events_json() for new messages.
// All module calls go through logos.callModule("chat_module", method, args).
//
// LogosQmlBridge (logos-basecamp/docs/project.md) serializes the Logos
// Module's c-ffi return value to JSON before handing it to QML. Concretely:
//   * int methods come back either as a JS number or as the JSON-encoded
//     string ("0", "-1", …). _callInt() handles both.
//   * char* methods come back either as a JS string or as a JSON-encoded
//     string (with surrounding quotes). _callStr() unwraps both.
// Helpers below normalise these so call sites can read the value as the
// expected native type.
//
// Method name mapping (logos-cpp-generator strips the "chat_module_" prefix):
//   chat_module_init                  → module_init
//   chat_module_shutdown              → module_shutdown
//   chat_module_<rest>                → module_<rest>

Item {
    id: root

    // Writable persistence path; set this to the module's instancePersistencePath
    // before init() is called. Defaults to a temporary location for dev/testing.
    property string instancePath: "/tmp/chat_module_default"

    property var conversations: []
    property var messages: []
    property string activeConvoId: ""
    property string installationName: ""
    property string deliveryState: "starting"
    property string deliveryDetail: ""
    property bool initialised: false

    // ── Initialise ───────────────────────────────────────────────────────────
    Component.onCompleted: {
        var rc = root._callInt("module_init", [instancePath, "logos.dev", 60000])
        if (rc !== 0) {
            console.error("chat_module init failed with code:", rc)
            root.deliveryState = "error"
            root.deliveryDetail = "init failed (rc=" + rc + ")"
            return
        }
        root.initialised = true
        root.refreshIdentity()
        root.refreshStatus()
        root.refreshConversations()
        pollTimer.start()
    }

    Component.onDestruction: {
        if (root.initialised) {
            root._callInt("module_shutdown", [])
        }
    }

    // ── Polling timer ─────────────────────────────────────────────────────────
    Timer {
        id: pollTimer
        interval: 200
        repeat: true
        onTriggered: root.processEvents()
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Left panel: identity + conversation list
        Rectangle {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            color: "#f5f5f5"
            border.color: "#ddd"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                IdentityPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    installationName: root.installationName
                    deliveryState: root.deliveryState
                    deliveryDetail: root.deliveryDetail
                    onIntroBundleRequested: root.copyIntroBundle()
                }

                Rectangle { height: 1; color: "#ddd"; Layout.fillWidth: true }

                ConversationList {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    conversations: root.conversations
                    selectedConvoId: root.activeConvoId
                    onConvoSelected: function(id) { root.selectConvo(id) }
                    onNewChatRequested: connectDialog.open()
                }
            }
        }

        // Right panel: messages + input
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            MessageView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                messages: root.messages
            }

            Rectangle { height: 1; color: "#ddd"; Layout.fillWidth: true }

            InputBar {
                Layout.fillWidth: true
                hasActiveConvo: root.activeConvoId !== ""
                onSendMessage: function(text) { root.sendMessage(text) }
                onConnectWithBundle: function(bundle) { root.connectWithBundle(bundle) }
            }
        }
    }

    // ── /connect dialog ───────────────────────────────────────────────────────
    Dialog {
        id: connectDialog
        title: "Connect with a peer"
        modal: true
        anchors.centerIn: parent
        width: 420

        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Label { text: "Paste the peer's intro bundle:" }
            TextArea {
                id: bundleField
                Layout.fillWidth: true
                height: 100
                wrapMode: TextEdit.WrapAnywhere
                placeholderText: "Paste intro bundle here…"
            }
        }

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            var bundle = bundleField.text.trim()
            if (bundle.length > 0) root.connectWithBundle(bundle)
            bundleField.text = ""
        }
    }

    // ── Bridge return helpers ─────────────────────────────────────────────────

    // Coerce a logos.callModule(...) return into a Number.
    // Accepts: number, "0"/"-1"/etc, JSON-encoded string of either.
    function _callInt(method, args) {
        var raw = logos.callModule("chat_module", method, args)
        if (typeof raw === "number") return raw
        if (typeof raw !== "string") return NaN
        if (raw.length === 0) return NaN
        // Strip a single layer of JSON-string wrapping if present.
        if (raw.charAt(0) === '"' && raw.charAt(raw.length - 1) === '"') {
            try { raw = JSON.parse(raw) } catch (e) {}
        }
        var n = parseInt(raw, 10)
        return isNaN(n) ? NaN : n
    }

    // Coerce a logos.callModule(...) return into a String.
    // Returns "" on null/undefined; unwraps JSON-quoted strings.
    function _callStr(method, args) {
        var raw = logos.callModule("chat_module", method, args)
        if (raw === null || raw === undefined) return ""
        if (typeof raw === "number") return String(raw)
        if (typeof raw !== "string") return ""
        if (raw.length === 0) return ""
        if (raw.charAt(0) === '"' && raw.charAt(raw.length - 1) === '"') {
            try { return JSON.parse(raw) } catch (e) { return raw }
        }
        return raw
    }

    // Read a JSON-shaped return (already a JSON string we control). Falls
    // back through the same string-unwrap path before parsing.
    function _callJson(method, args) {
        var s = root._callStr(method, args)
        if (s.length === 0) return null
        try { return JSON.parse(s) } catch (e) {
            console.error("chat_module: bad JSON from", method, ":", s)
            return null
        }
    }

    // ── Logic ─────────────────────────────────────────────────────────────────

    function refreshIdentity() {
        var name = root._callStr("module_get_installation_name", [])
        if (name.length > 0) root.installationName = name
    }

    function refreshConversations() {
        var arr = root._callJson("module_list_conversations_json", [])
        if (arr !== null && Array.isArray(arr)) root.conversations = arr
    }

    function refreshStatus() {
        var st = root._callJson("module_status_json", [])
        if (st !== null) {
            if (st.delivery_state) root.deliveryState = st.delivery_state
            if (typeof st.detail === "string") root.deliveryDetail = st.detail
            if (typeof st.identity === "string" && st.identity.length > 0)
                root.installationName = st.identity
        }
    }

    function selectConvo(id) {
        root.activeConvoId = id
        var arr = root._callJson("module_get_messages_json", [id])
        if (arr !== null && Array.isArray(arr)) root.messages = arr
    }

    function sendMessage(text) {
        if (root.activeConvoId === "") return
        var rc = root._callInt("module_send_message", [root.activeConvoId, text])
        if (rc !== 0) {
            console.error("send_message failed with code:", rc)
            return
        }
        root.selectConvo(root.activeConvoId)
    }

    function connectWithBundle(bundle) {
        var newId = root._callStr("module_create_conversation", [bundle, "Hello!"])
        if (newId.length === 0) {
            console.error("create_conversation failed")
            return
        }
        root.refreshConversations()
        root.selectConvo(newId)
    }

    function copyIntroBundle() {
        var b64 = root._callStr("module_create_intro_bundle_b64", [])
        if (b64.length === 0) {
            console.error("create_intro_bundle failed")
            return
        }
        if (typeof Qt.application !== "undefined"
                && typeof Qt.application.clipboard !== "undefined") {
            Qt.application.clipboard.text = b64
        }
        console.log("Intro bundle:", b64)
    }

    function processEvents() {
        var events = root._callJson("module_drain_events_json", [])
        if (events === null || !Array.isArray(events) || events.length === 0) return

        for (var i = 0; i < events.length; i++) {
            var evt = events[i]
            switch (evt.type) {
            case "messageReceived":
                if (evt.convo_id === root.activeConvoId) {
                    root.selectConvo(root.activeConvoId)
                }
                root.refreshConversations()
                break
            case "conversationCreated":
                root.refreshConversations()
                if (!evt.is_outgoing) {
                    root.selectConvo(evt.convo_id)
                }
                break
            case "conversationUpdated":
                root.refreshConversations()
                break
            case "deliveryStateChanged":
                root.deliveryState = evt.state || "error"
                root.deliveryDetail = evt.detail || ""
                break
            }
        }
    }
}
