import QtQuick 2.15
import QtQuick.Controls 2.15

// Sidebar list of conversations.
ListView {
    id: root

    property var conversations: []       // array of conversation objects
    property string selectedConvoId: ""

    signal convoSelected(string convoId)
    signal newChatRequested()

    clip: true

    header: Item {
        width: root.width
        height: 48
        Button {
            anchors.centerIn: parent
            text: "+ New Chat"
            onClicked: root.newChatRequested()
        }
    }

    model: root.conversations

    delegate: ItemDelegate {
        width: root.width
        highlighted: modelData.convo_id === root.selectedConvoId

        contentItem: Column {
            spacing: 2
            Label {
                text: modelData.nickname || modelData.convo_id.substring(0, 8)
                font.bold: true
                font.pixelSize: 14
            }
            Label {
                text: modelData.message_count + " message(s)"
                font.pixelSize: 11
                color: "#999"
            }
        }

        onClicked: root.convoSelected(modelData.convo_id)
    }

    ScrollBar.vertical: ScrollBar {}
}
