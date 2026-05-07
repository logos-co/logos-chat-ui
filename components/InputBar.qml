import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Composer bar: text field + Send button + /connect paste dialog.
Item {
    id: root

    property bool hasActiveConvo: false

    signal sendMessage(string text)
    signal connectWithBundle(string bundle)

    height: 56

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        TextField {
            id: field
            Layout.fillWidth: true
            placeholderText: root.hasActiveConvo
                ? "Type a message… or /connect <bundle>"
                : "/connect <bundle> to start a chat"
            Keys.onReturnPressed: root._submit()
            Keys.onEnterPressed: root._submit()
        }

        Button {
            text: "Send"
            enabled: field.text.trim().length > 0
            onClicked: root._submit()
        }
    }

    function _submit() {
        var text = field.text.trim()
        if (text.length === 0) return
        if (text.startsWith("/connect ")) {
            var bundle = text.substring(9).trim()
            if (bundle.length > 0) root.connectWithBundle(bundle)
        } else {
            root.sendMessage(text)
        }
        field.text = ""
    }
}
