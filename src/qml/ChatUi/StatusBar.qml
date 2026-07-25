import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Bottom status strip: a free-form status message and a short connectivity
// label. All inputs are properties; standalone.
Rectangle {
    id: root

    property string statusMessage: ""
    property string statusLabel: ""
    property bool online: false
    property bool hasError: false
    // How long a status message stays on the bar. Messages are announcements of
    // something that happened, so they go quiet again; the connectivity label
    // is state and stays.
    property int messageTimeout: 6000

    implicitWidth: 200
    implicitHeight: 28
    color: Theme.palette.backgroundTertiary

    onStatusMessageChanged: messageTimer.restart()

    Timer {
        id: messageTimer
        interval: root.messageTimeout
        running: true
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing.medium
        anchors.rightMargin: Theme.spacing.medium
        spacing: Theme.spacing.small

        LogosText {
            objectName: "statusMessageLabel"
            text: messageTimer.running ? root.statusMessage : ""
            textFormat: Text.PlainText
            color: Theme.palette.textTertiary
            font.pixelSize: Theme.typography.secondaryText
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        LogosText {
            text: root.statusLabel
            color: root.online ? Theme.palette.success : root.hasError ? Theme.palette.error : Theme.palette.textTertiary
            font.pixelSize: Theme.typography.secondaryText
        }
    }
}
