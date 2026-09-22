import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Why Chat does not know the network of a delivery node it did not start, what
// that can mean, and what the user can do about it. open(); standalone.
LogosDialog {
    id: root

    title: qsTr("Delivery network unknown")
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    width: Math.min(480, (Overlay.overlay ? Overlay.overlay.width : 480) - 2 * Theme.spacing.large)

    rightActions: [
        LogosButton {
            implicitWidth: 96
            implicitHeight: 36
            text: qsTr("Close")
            onClicked: root.close()
        }
    ]

    contentItem: ColumnLayout {
        spacing: Theme.spacing.medium

        LogosText {
            text: qsTr("All apps in basecamp share one delivery node, set up by whichever app starts it first. It was running before Chat opened, so Chat is using its settings.")
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        LogosText {
            text: qsTr("If they put it on another network, you and contacts on Chat's network cannot reach each other.")
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        LogosText {
            text: qsTr("To use Chat's network, quit basecamp and open Chat before any other app.")
            color: Theme.palette.text
            font.pixelSize: Theme.typography.secondaryText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
