import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// The strip across the foot of the canvas, where the app says what it could not
// do. It holds the newest failure until someone deals with it and counts the
// ones queued behind it, so a burst is visible rather than collapsing to
// whichever arrived last. Activating the message is what clears the strip.
// Set the properties; standalone.
Rectangle {
    id: root

    // The newest failure, in prose. Empty leaves the strip resting.
    required property string errorMessage
    // How many failures are waiting, the displayed one included.
    required property int errorCount

    signal errorActivated

    implicitWidth: 480
    implicitHeight: 26
    radius: Theme.spacing.radiusMedium
    color: Theme.palette.backgroundTertiary
    border.width: 1
    border.color: Theme.palette.borderSubtle

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing.medium
        anchors.rightMargin: Theme.spacing.tiny
        spacing: Theme.spacing.small

        Rectangle {
            visible: root.errorCount > 0
            Layout.preferredWidth: 6
            Layout.preferredHeight: 6
            radius: Theme.spacing.radiusPill
            color: Theme.palette.error
        }

        LogosText {
            objectName: "statusMessage"
            Layout.fillWidth: true
            text: root.errorMessage
            textFormat: Text.PlainText
            color: Theme.palette.error
            font.pixelSize: Theme.typography.secondaryText
            elide: Text.ElideRight
            Accessible.role: Accessible.StaticText
            Accessible.name: root.errorMessage

            TapHandler {
                enabled: root.errorCount > 0
                onTapped: root.errorActivated()
            }

            HoverHandler {
                enabled: root.errorCount > 0
                cursorShape: Qt.PointingHandCursor
            }
        }

        LogosText {
            objectName: "statusErrorCount"
            visible: root.errorCount > 1
            //: How many failures are waiting on the status bar, the one shown included.
            text: qsTr("%n errors", "", root.errorCount)
            color: Theme.palette.textTertiary
            font.pixelSize: Theme.typography.secondaryText
        }
    }
}
