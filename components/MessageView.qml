import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Scrollable message bubble view for the active conversation.
Item {
    id: root

    property var messages: []   // array of {from_self, content, timestamp} objects

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6
        clip: true

        model: root.messages

        delegate: Item {
            width: list.width
            height: bubble.implicitHeight + 8

            readonly property bool isSelf: modelData.from_self

            Rectangle {
                id: bubble
                color: isSelf ? "#0078d7" : "#f0f0f0"
                radius: 10
                width: Math.min(msgText.implicitWidth + 20, list.width * 0.75)
                implicitHeight: msgText.implicitHeight + 12

                anchors.right: isSelf ? parent.right : undefined
                anchors.left: isSelf ? undefined : parent.left

                Text {
                    id: msgText
                    anchors {
                        left: parent.left; right: parent.right
                        top: parent.top; bottom: parent.bottom
                        margins: 6
                    }
                    text: modelData.content
                    color: isSelf ? "#fff" : "#222"
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    font.pixelSize: 14
                }
            }
        }

        onCountChanged: positionViewAtEnd()

        ScrollBar.vertical: ScrollBar {}
    }
}
