import QtQuick

import Logos.ChatBackend

// The host's `logos` bridge, reduced to what ChatStore asks of it: the chat_ui
// backend and its three models, all plain QML a scene fills in.
QtObject {
    readonly property ChatBackend backend: ChatBackend {}
    property ListModel conversations: ListModel {}
    property ListModel messages: ListModel {}
    property ListModel members: ListModel {}

    function module(name) {
        return name === "chat_ui" ? backend : null;
    }
    function model(module, prop) {
        if (module !== "chat_ui")
            return null;
        return ({
                conversationModel: conversations,
                messageModel: messages,
                memberModel: members
            })[prop] ?? null;
    }
}
